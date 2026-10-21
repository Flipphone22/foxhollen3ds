#include "fh_graphics_options.h"
#include "fh_prepared_movie.h"
#include "fh_movie_lz4.h"
#include "fh_movie_crc.h"
#include <zlib.h>
#include <dolphin/types.h>
#include <dolphin/thp/THPAudio.h>
#include <dolphin/thp/THPDecode.h>
#include "main/dll/FRONT/attract_movie.h"
#include "foxhollow_compat.h"

#include <setjmp.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <jpeglib.h>

typedef struct THPJpegError
{
    struct jpeg_error_mgr base;
    jmp_buf jump;
} THPJpegError;

static u8* sDecodedFrame;
static size_t sDecodedFrameSize;
static u8* sJpegFrame;
static size_t sJpegFrameSize;
static size_t sCompressedFrameSize;

typedef struct THPRGBTexture
{
    u8* rgba;
    size_t size;
    unsigned format,width,height;
} THPRGBTexture;

static THPRGBTexture sRGBTextures[3];
unsigned fhTHPVideoGetNativeFormat(const void *pixels) {
    for(unsigned i=0;i<3;i++)if(pixels && pixels==sRGBTextures[i].rgba)return sRGBTextures[i].format;
    return 0x46;
}
void fhTHPVideoGetTextureSize(const void *pixels,unsigned *width,unsigned *height) {
    for(unsigned i=0;i<3;i++)if(pixels && pixels==sRGBTextures[i].rgba){*width=sRGBTextures[i].width;*height=sRGBTextures[i].height;return;}
}

static u32 thpReadU32(const void* ptr)
{
    u32 value;
    memcpy(&value, ptr, sizeof(value));
    return fhSwap32(value);
}

static void thpJpegErrorExit(j_common_ptr info)
{
    THPJpegError* error = (THPJpegError*)info->err;
    longjmp(error->jump, 1);
}

void fhTHPVideoSetCompressedSize(u32 size)
{
    sCompressedFrameSize = size;
}

void* fhTHPVideoGetRGB(const void* yTexture)
{
    u32 i;

    for (i = 0; i < 3; i++)
    {
        if (gAttractMoviePlayer.textureSet[i].yTexture == yTexture)
        {
            return sRGBTextures[i].rgba;
        }
    }
    return NULL;
}

static THPRGBTexture* thpGetRGBTexture(const void* yTexture, size_t size)
{
    THPRGBTexture* texture = NULL;
    u32 i;

    for (i = 0; i < 3; i++)
    {
        if (gAttractMoviePlayer.textureSet[i].yTexture == yTexture)
        {
            texture = &sRGBTextures[i];
            break;
        }
    }
    if (texture == NULL)
    {
        return NULL;
    }
    if (size > texture->size)
    {
        u8* resized = realloc(texture->rgba, size);
        if (resized == NULL)
        {
            return NULL;
        }
        texture->rgba = resized;
        texture->size = size;
    }
    return texture;
}

static u8 thpClampColor(s32 value)
{
    if (value < 0)
    {
        return 0;
    }
    if (value > 255)
    {
        return 255;
    }
    return (u8)value;
}

static size_t thpBuildJpeg(const u8* source)
{
    size_t sourceOffset = 2;
    size_t scanOffset = 0;
    size_t outputOffset;
    size_t needed;

    while (sourceOffset + 4 <= sCompressedFrameSize)
    {
        size_t markerLength;
        if (source[sourceOffset] != 0xff)
        {
            return 0;
        }
        while (sourceOffset < sCompressedFrameSize && source[sourceOffset] == 0xff)
        {
            sourceOffset++;
        }
        if (sourceOffset >= sCompressedFrameSize)
        {
            return 0;
        }
        if (source[sourceOffset] == 0xda)
        {
            markerLength = ((size_t)source[sourceOffset + 1] << 8) | source[sourceOffset + 2];
            scanOffset = sourceOffset + 1 + markerLength;
            break;
        }
        if (source[sourceOffset] == 0xd8 || (source[sourceOffset] >= 0xd0 && source[sourceOffset] <= 0xd7))
        {
            sourceOffset++;
            continue;
        }
        markerLength = ((size_t)source[sourceOffset + 1] << 8) | source[sourceOffset + 2];
        if (markerLength < 2)
        {
            return 0;
        }
        sourceOffset += 1 + markerLength;
    }

    if (scanOffset == 0 || scanOffset > sCompressedFrameSize)
    {
        return 0;
    }

    needed = sCompressedFrameSize * 2 + 2;
    if (needed > sJpegFrameSize)
    {
        u8* resized = realloc(sJpegFrame, needed);
        if (resized == NULL)
        {
            return 0;
        }
        sJpegFrame = resized;
        sJpegFrameSize = needed;
    }

    memcpy(sJpegFrame, source, scanOffset);
    outputOffset = scanOffset;
    for (sourceOffset = scanOffset; sourceOffset < sCompressedFrameSize; sourceOffset++)
    {
        sJpegFrame[outputOffset++] = source[sourceOffset];
        if (source[sourceOffset] == 0xff)
        {
            sJpegFrame[outputOffset++] = 0;
        }
    }
    sJpegFrame[outputOffset++] = 0xff;
    sJpegFrame[outputOffset++] = 0xd9;
    return outputOffset;
}

static void thpJpegEmitMessage(j_common_ptr info, int level)
{
    (void)info;
    (void)level;
}

static size_t thpTileOffset(u32 width, u32 x, u32 y)
{
    return (((size_t)(y >> 2) * (width >> 3) + (x >> 3)) << 5) + ((y & 3) << 3) + (x & 7);
}

BOOL THPInit(void)
{
    return TRUE;
}

s32 THPVideoDecode(void* file, void* tileY, void* tileU, void* tileV, void* work)
{
    struct jpeg_decompress_struct info;
    THPJpegError error;
    const u8* source = file;
    size_t sourceSize;
    size_t decodedSize;
    THPRGBTexture* rgbTexture;
    u32 width;
    u32 height;
    u32 x;
    u32 y;

    (void)work;
    if (source == NULL || tileY == NULL || tileU == NULL || tileV == NULL)
    {
        return 27;
    }

    unsigned nativeFormat=fh_movie_video_format(source,sCompressedFrameSize);
    if(nativeFormat) {
        size_t nativeBytes=fh_movie_video_bytes(source,sCompressedFrameSize,
            gAttractMoviePlayer.videoInfo.xSize,gAttractMoviePlayer.videoInfo.ySize);
        if(!nativeBytes)return 19;
#ifdef FOXHOLLOW_N3DS
        extern void *fh_pica_movie_begin_format(unsigned,unsigned,unsigned,unsigned);
        extern void fh_pica_movie_commit(void*);
        unsigned slot=0;
        while(slot<3 && gAttractMoviePlayer.textureSet[slot].yTexture!=tileY)slot++;
        if(slot==3)return 6;
        rgbTexture=sRGBTextures+slot;
        if(rgbTexture->rgba && rgbTexture->format!=FH_TEXTURE_PICA_ETC1&&rgbTexture->format!=FH_TEXTURE_PICA_RGB8)free(rgbTexture->rgba);
        rgbTexture->rgba=NULL;rgbTexture->size=0;
        rgbTexture->format=nativeFormat;
        /* Decode is a visual consumer. The player still advances its original
         * frame queue and audio. Hide this slot until a newly decoded frame is
         * published after re-enable; never expose old pixels as current video.
         * The PICA owner retains existing allocations and their GPU fences. */
        if(fh_graphics_options&(FH_GRAPHICS_NO_RENDER|FH_GRAPHICS_NO_ACTORS|FH_GRAPHICS_NO_MOVIES))return 0;
        rgbTexture->rgba=fh_pica_movie_begin_format(slot,fh_movie_le32(source+8),fh_movie_le32(source+12),nativeFormat);
        if(!rgbTexture->rgba)return 6;
#else
        rgbTexture=thpGetRGBTexture(tileY,nativeBytes);
        if(!rgbTexture)return 6;
#endif
        if(fh_movie_le32(source+28)==2) {
            if(!fh_movie_lz4_block(rgbTexture->rgba,nativeBytes,source+32,sCompressedFrameSize-36) ||
                fh_movie_crc32(0,rgbTexture->rgba,nativeBytes)!=fh_movie_le32(source+sCompressedFrameSize-4))return 3;
        }else if(fh_movie_le32(source+28)) {
            uLongf outputBytes=nativeBytes;uLong inputBytes=sCompressedFrameSize-32;
            if(uncompress2(rgbTexture->rgba,&outputBytes,source+32,&inputBytes)!=Z_OK || outputBytes!=nativeBytes || inputBytes!=sCompressedFrameSize-32)return 3;
        }else memcpy(rgbTexture->rgba,source+32,nativeBytes);
        rgbTexture->format=nativeFormat;
        rgbTexture->width=fh_movie_le32(source+8);rgbTexture->height=fh_movie_le32(source+12);
#ifdef FOXHOLLOW_N3DS
        fh_pica_movie_commit(rgbTexture->rgba);
#endif
        return 0;
    }
#ifdef FOXHOLLOW_N3DS
    /* A prepared stream cannot switch its GPU-owned slots to realloc-backed
     * JPEG storage. Mixed/corrupt streams fail before touching those owners. */
    for(unsigned i=0;i<3;i++)if(sRGBTextures[i].format==FH_TEXTURE_PICA_ETC1||sRGBTextures[i].format==FH_TEXTURE_PICA_RGB8)return 19;
#endif
    sourceSize = thpBuildJpeg(source);
    if (sourceSize == 0)
    {
        return 3;
    }

    info.err = jpeg_std_error(&error.base);
    error.base.error_exit = thpJpegErrorExit;
    error.base.emit_message = thpJpegEmitMessage;
    if (setjmp(error.jump) != 0)
    {
        jpeg_destroy_decompress(&info);
        return 3;
    }

    jpeg_create_decompress(&info);
    jpeg_mem_src(&info, sJpegFrame, sourceSize);
    jpeg_read_header(&info, TRUE);
    info.out_color_space = JCS_EXT_RGBA;
    jpeg_start_decompress(&info);

    width = info.output_width;
    height = info.output_height;
    if (info.output_components != 4 || width != gAttractMoviePlayer.videoInfo.xSize ||
        height != gAttractMoviePlayer.videoInfo.ySize || (width & 15) != 0 || (height & 7) != 0)
    {
        jpeg_destroy_decompress(&info);
        return 19;
    }

    rgbTexture = thpGetRGBTexture(tileY, (size_t)width * height * 4);
    if (rgbTexture == NULL) { jpeg_destroy_decompress(&info); return 6; }
    rgbTexture->format=0x46;
    rgbTexture->width=width;rgbTexture->height=height;
    while (info.output_scanline < height) {
        JSAMPROW row = rgbTexture->rgba + (size_t)info.output_scanline * width * 4;
        if (jpeg_read_scanlines(&info, &row, 1) != 1) {
            jpeg_destroy_decompress(&info); return 3;
        }
    }
    jpeg_finish_decompress(&info);
    jpeg_destroy_decompress(&info);

    return 0;
}

typedef struct THPAudioCursor
{
    const u8* data;
    u32 nibble;
    u8 predictor;
    u8 scale;
} THPAudioCursor;

static void thpAudioCursorInit(THPAudioCursor* cursor, const u8* data)
{
    cursor->data = data + 1;
    cursor->nibble = 2;
    cursor->predictor = (data[0] >> 4) & 7;
    cursor->scale = data[0] & 15;
}

static s32 thpAudioNextSample(THPAudioCursor* cursor)
{
    s32 sample;

    if ((cursor->nibble & 15) == 0)
    {
        cursor->predictor = (cursor->data[0] >> 4) & 7;
        cursor->scale = cursor->data[0] & 15;
        cursor->data++;
        cursor->nibble += 2;
    }
    if ((cursor->nibble & 1) != 0)
    {
        sample = (s8)(cursor->data[0] << 4) >> 4;
        cursor->data++;
    }
    else
    {
        sample = (s8)cursor->data[0] >> 4;
    }
    cursor->nibble++;
    return sample;
}

static s16 thpAudioDecodeSample(THPAudioCursor* cursor, const u8* coefficients, s16* yn1, s16* yn2)
{
    s64 value = (s64)fhReadBES16(coefficients + (cursor->predictor * 4) + 2) * *yn2;
    value += (s64)fhReadBES16(coefficients + (cursor->predictor * 4)) * *yn1;
    value += ((s64)thpAudioNextSample(cursor) << cursor->scale) << 11;
    value = (value + 1024) >> 11;
    if (value > 32767)
    {
        value = 32767;
    }
    else if (value < -32768)
    {
        value = -32768;
    }
    *yn2 = *yn1;
    *yn1 = (s16)value;
    return (s16)value;
}

u32 THPAudioDecode(s16* buffer, u8* audioFrame, s32 flag)
{
    u32 channelOffset;
    u32 sampleCount;
    const u8* leftCoefficients;
    const u8* rightCoefficients;
    const u8* leftData;
    const u8* rightData;
    THPAudioCursor leftCursor;
    THPAudioCursor rightCursor;
    s16 leftYn1;
    s16 leftYn2;
    s16 rightYn1;
    s16 rightYn2;
    u32 i;

    if (buffer == NULL || audioFrame == NULL)
    {
        return 0;
    }

    channelOffset = thpReadU32(audioFrame);
    sampleCount = thpReadU32(audioFrame + 4);
    leftCoefficients = audioFrame + 8;
    rightCoefficients = leftCoefficients + 32;
    leftYn1 = fhReadBES16(audioFrame + 72);
    leftYn2 = fhReadBES16(audioFrame + 74);
    rightYn1 = fhReadBES16(audioFrame + 76);
    rightYn2 = fhReadBES16(audioFrame + 78);
    leftData = audioFrame + sizeof(THPAudioRecordHeader);
    rightData = leftData + channelOffset;
    thpAudioCursorInit(&leftCursor, leftData);
    if (channelOffset != 0)
    {
        thpAudioCursorInit(&rightCursor, rightData);
    }

    for (i = 0; i < sampleCount; i++)
    {
        s16 left = thpAudioDecodeSample(&leftCursor, leftCoefficients, &leftYn1, &leftYn2);
        s16 right = left;
        if (channelOffset != 0)
        {
            right = thpAudioDecodeSample(&rightCursor, rightCoefficients, &rightYn1, &rightYn2);
        }
        if (flag == 1)
        {
            buffer[i] = right;
            buffer[sampleCount + i] = left;
        }
        else
        {
            buffer[i * 2] = right;
            buffer[i * 2 + 1] = left;
        }
    }

    return sampleCount;
}
