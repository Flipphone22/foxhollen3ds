#ifndef FH_INSTALL_MOVIE_DECODE_H
#define FH_INSTALL_MOVIE_DECODE_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
enum{FH_INSTALL_MOVIE_WIDTH=512,FH_INSTALL_MOVIE_HEIGHT=384,FH_INSTALL_MOVIE_VIDEO_CAP=196608};
typedef struct{unsigned char*rgb;unsigned bytes,width,height;char error[112];}FhInstallMovieImage;
typedef int(*FhInstallMovieDecodeProgress)(void*,unsigned rows,unsigned total);
/* Startup-only PAL THP JPEG -> RGB. Zero initialize owner. Decoder unit must
 * use the SDK JPEG ABI (-fshort-enums on ARM); this public ABI has no enums or
 * JPEG structs. Returns1 complete,0 error,-1 cancel; no partial image escapes. */
int fh_install_movie_decode(FhInstallMovieImage*,const void*,unsigned bytes,FhInstallMovieDecodeProgress,void*);
void fh_install_movie_image_close(FhInstallMovieImage*);
#ifdef __cplusplus
}
#endif
#endif
