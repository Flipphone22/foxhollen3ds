#include "fh_pica_stars.h"
#include "fh_pica_hud.h"
#include "fh_pica_viewport.h"
#include "main/sky_interface.h"
#include "main/mm.h"
#include "main/camera.h"
#include "main/texture.h"
#include "main/rcp_dolphin.h"
#include "dolphin/gx/GXEnum.h"
#include "dolphin/os/OSCache.h"
#include "main/sky_state.h"
#include "dolphin/gx/GXCull.h"
#include "dolphin/gx/GXGeometry.h"
#include "dolphin/gx/GXLegacy.h"
#include "dolphin/gx/GXManage.h"
#include "dolphin/gx/GXPixel.h"
#include "dolphin/gx/GXTransform.h"
#include "dolphin/mtx.h"
#include "main/lightmap.h"
#include "main/vecmath.h"
#include "dolphin/math.h"
#include "dolphin/mtx/vec.h"
#include "dolphin/gx/GXDispList.h"
#include "track/intersect.h"
#include "track/intersect_render_setup.h"
#include "main/newclouds.h"

u8 gNewCloudStarsInitialized;
Texture* gNewCloudStarTextureB;
Texture* gNewCloudStarTextureA;
typedef struct SkyStarAlphaRange
{
    u8 min;
    u8 max;
} SkyStarAlphaRange;

typedef struct SkyStarColorRange
{
    u8 minRed;
    u8 maxRed;
    u8 minGreen;
    u8 maxGreen;
    u8 minBlue;
    u8 maxBlue;
} SkyStarColorRange;

STATIC_ASSERT(sizeof(SkyStarAlphaRange) == 2);
STATIC_ASSERT(sizeof(SkyStarColorRange) == 6);

enum
{
    SKY_STAR_COLOR_RANGE_COUNT = 4,
    SKY_STAR_DISPLAY_LIST_COUNT = 0x5C,
    SKY_STAR_SMALL_DISPLAY_LIST_COUNT = 0x4C,
    SKY_STAR_TEXTURE_B_LIST_INDEX = 0x54,
    SKY_STAR_CONSTELLATION_POINT_COUNT = 0x64,
    SKY_STAR_POINTS_PER_DISPLAY_LIST = 0x32,
    SKY_STAR_DISPLAY_LIST_BUFFER_SIZE = 0x220
};

SkyStarAlphaRange gNewCloudStarAlphaRanges[SKY_STAR_COLOR_RANGE_COUNT] = {
    {0xA0, 0xAA}, {0x82, 0x8C}, {0x64, 0x6E}, {0x50, 0x5A}
};
GXColor gNewCloudStarFogColor = {0};



SkyStarColorRange gNewCloudStarColorRanges[SKY_STAR_COLOR_RANGE_COUNT] = {
    {0xD0, 0xFF, 0x80, 0xA0, 0x80, 0xA0},
    {0x80, 0xA0, 0x80, 0xA0, 0xD0, 0xFF},
    {0xD0, 0xFF, 0xA0, 0xD0, 0x80, 0xA0},
    {0xD0, 0xFF, 0x80, 0xA0, 0xD0, 0xFF},
};



#define NEWCLOUD_TEXTURE_STAR_A 0xc21 /* gNewCloudStarTextureA */
#define NEWCLOUD_TEXTURE_STAR_B 0xc22 /* gNewCloudStarTextureB */

void drawSkyStars(void)
{
    int timeOk;
    int firstDisplayList;
    int i;
    int alpha;
    int pointSizeDivisor;
    int red;
    int green;
    int blue;
    int starAlpha;
    SkyStarColorRange* colorRange;
    SkyStarAlphaRange* alphaRange;
    GXColor color;
    f32 sunTransitionTime;

    timeOk = (*gSkyInterface)->getSunPosition(&sunTransitionTime);
    if (isOvercast() != 0)
    {
        if (timeOk != 0)
        {
            if (sunTransitionTime > 4000.0f)
            {
                alpha = 0xff;
            }
            else
            {
                alpha = (255.0f * (sunTransitionTime / 4000.0f));
            }
        }
        else
        {
            if (sunTransitionTime > 12000.0f || sunTransitionTime == 0.0f)
            {
                return;
            }
            alpha = (255.0f - 255.0f * (sunTransitionTime / 12000.0f));
        }
        firstDisplayList = SKY_STAR_SMALL_DISPLAY_LIST_COUNT;
        pointSizeDivisor = 2;
    }
    else
    {
        firstDisplayList = 0;
        alpha = 0xff;
        pointSizeDivisor = 1;
    }
    Camera_RebuildProjectionMatrix();
    Camera_UpdateViewMatrices();
    unsigned pointSize;
    for (i = firstDisplayList; i < SKY_STAR_DISPLAY_LIST_COUNT; i++)
    {
        colorRange = &gNewCloudStarColorRanges[i & 3];
        red = randomGetRange(colorRange->minRed, colorRange->maxRed);
        green = randomGetRange(colorRange->minGreen, colorRange->maxGreen);
        blue = randomGetRange(colorRange->minBlue, colorRange->maxBlue);
        if (i < SKY_STAR_SMALL_DISPLAY_LIST_COUNT)
        {
            alphaRange = &gNewCloudStarAlphaRanges[(i & 0xc) >> 2];
            starAlpha = (alpha * randomGetRange(alphaRange->min, alphaRange->max)) >> 8;
        }
        else
        {
            starAlpha = alpha;
        }
        if (i < SKY_STAR_SMALL_DISPLAY_LIST_COUNT)
        {
            pointSize=(u8)randomGetRange(0xc, 0xc);
        }
        else if (i & 4)
        {
            pointSize=(u8)(randomGetRange(0x30, 0x3c) / pointSizeDivisor);
        }
        else
        {
            pointSize=(u8)(randomGetRange(0x48, 0x60) / pointSizeDivisor);
        }
        fh_pica_stars_group(i,((u32)(u8)red<<24)|((u32)(u8)green<<16)|((u32)(u8)blue<<8)|(u8)starAlpha,pointSize);
    }
    float viewport[6];unsigned clip[4];
    fh_pica_live_viewport(viewport);GXGetScissor(clip,clip+1,clip+2,clip+3);
    fh_pica_stars_draw(firstDisplayList,fh_pica_hud_page(gNewCloudStarTextureA),fh_pica_hud_page(gNewCloudStarTextureB),
        (const float(*)[4])Camera_GetProjectionMatrix(),(const float(*)[4])Camera_GetViewRotationMatrix(),viewport,clip);
}


void initSkyStars(void)
{
    int displayListIndex;
    int pointIndex;
    Vec3f* constellation;
    Vec3f* constellationPoint;
    int constellationPointIndex;
    int constellationIndex;
    Vec3f starPosition;
    Mtx rotationA;
    Mtx rotationB;

    fh_pica_stars_allocate();
    testAndSet_onlyUseHeap3(0);
    constellation = mmAlloc(SKY_STAR_CONSTELLATION_POINT_COUNT * sizeof(Vec3f), 0x7f7f7fff, 0);
    testAndSet_onlyUseHeap3(1);
    for (constellationPointIndex = 0, constellationPoint = constellation;
         constellationPointIndex < SKY_STAR_CONSTELLATION_POINT_COUNT;
         constellationPointIndex++)
    {
        do
        {
            starPosition.x = (int)
            randomGetRange(-5000, 5000);
            starPosition.y = (int)
            randomGetRange(-5000, 5000);
            starPosition.z = (int)
            randomGetRange(-5000, 5000);
        }
        while (starPosition.x == 0.0f && starPosition.y == 0.0f && starPosition.z == 0.0f);
        PSVECNormalize(&starPosition, &starPosition);
        PSVECScale(&starPosition, &starPosition, 5000.0f);
        constellationPoint->x = starPosition.x;
        constellationPoint->y = starPosition.y;
        constellationPoint->z = starPosition.z;
        constellationPoint++;
    }
    gNewCloudStarsInitialized = 1;
    gNewCloudStarTextureA = textureLoadAsset(NEWCLOUD_TEXTURE_STAR_A);
    gNewCloudStarTextureB = textureLoadAsset(NEWCLOUD_TEXTURE_STAR_B);
    fh_pica_game_hud_loaded(gNewCloudStarTextureA);
    fh_pica_game_hud_loaded(gNewCloudStarTextureB);
    for (displayListIndex = 0; displayListIndex < SKY_STAR_DISPLAY_LIST_COUNT; displayListIndex++)
    {
        for (pointIndex = 0; pointIndex < SKY_STAR_POINTS_PER_DISPLAY_LIST; pointIndex++)
        {
            if (randomGetRange(0, 9) < 5)
            {
                do
                {
                    starPosition.x = (int)
                    randomGetRange(-5000, 5000);
                    starPosition.y = (int)
                    randomGetRange(-5000, 5000);
                    starPosition.z = (int)
                    randomGetRange(-5000, 5000);
                }
                while (starPosition.x == 0.0f && starPosition.y == 0.0f && starPosition.z == 0.0f);
                PSVECNormalize(&starPosition, &starPosition);
                PSVECScale(&starPosition, &starPosition, 5000.0f);
            }
            else
            {
                f32 ax;
                constellationIndex = randomGetRange(0, SKY_STAR_CONSTELLATION_POINT_COUNT - 1);
                starPosition.x = constellation[constellationIndex].x;
                starPosition.y = constellation[constellationIndex].y;
                starPosition.z = constellation[constellationIndex].z;
                ax = __fabsf(starPosition.x);
                if (ax > 3750.0f)
                {
                    PSMTXRotRad(rotationA, 0x79,
                                (0.015f *
                                    (2.0f *
                                        (3.142f *
                                            randomGetRange(-0x8000, 0x8000))
                    )
                    )
                    /
                    32768.0f
                    )
                    ;
                    PSMTXRotRad(rotationB, 0x7a,
                                (0.015f *
                                    (2.0f *
                                        (3.142f *
                                            randomGetRange(-0x8000, 0x8000))
                    )
                    )
                    /
                    32768.0f
                    )
                    ;
                }
                else
                {
                    f32 ay = __fabsf(starPosition.y);
                    if (ay > 3750.0f)
                    {
                        PSMTXRotRad(rotationA, 0x78,
                                    (0.015f *
                                        (2.0f *
                                            (3.142f *
                                                randomGetRange(-0x8000, 0x8000))
                        )
                        )
                        /
                        32768.0f
                        )
                        ;
                        PSMTXRotRad(rotationB, 0x7a,
                                    (0.015f *
                                        (2.0f *
                                            (3.142f *
                                                randomGetRange(-0x8000, 0x8000))
                        )
                        )
                        /
                        32768.0f
                        )
                        ;
                    }
                    else
                    {
                        PSMTXRotRad(rotationA, 0x78,
                                    (0.015f *
                                        (2.0f *
                                            (3.142f *
                                                randomGetRange(-0x8000, 0x8000))
                        )
                        )
                        /
                        32768.0f
                        )
                        ;
                        PSMTXRotRad(rotationB, 0x79,
                                    (0.015f *
                                        (2.0f *
                                            (3.142f *
                                                randomGetRange(-0x8000, 0x8000))
                        )
                        )
                        /
                        32768.0f
                        )
                        ;
                    }
                }
                PSMTXConcat(rotationB, rotationA, rotationA);
                PSMTXMultVecSR(rotationA, &starPosition, &starPosition);
            }
            fh_pica_stars_point(displayListIndex,pointIndex,starPosition.x,starPosition.y,starPosition.z);
        }

    }
    mm_free(constellation);
    fh_pica_stars_commit();
}
