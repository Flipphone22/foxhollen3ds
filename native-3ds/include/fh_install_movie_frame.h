#ifndef FH_INSTALL_MOVIE_FRAME_H
#define FH_INSTALL_MOVIE_FRAME_H
#include "fh_install_movie_decode.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum{FH_INSTALL_MOVIE_RESIZE,FH_INSTALL_MOVIE_ENCODE,FH_INSTALL_MOVIE_COMPRESS};
typedef int(*FhInstallMovieFrameProgress)(void*,unsigned phase,unsigned done,unsigned total);
typedef struct{unsigned char*data;unsigned bytes,width,height,raw_bytes,codec;uint64_t squared_error,components;char error[112];}FhInstallMovieFrame;
/* Separable Lanczos half resize, clipped/rounded after each RGB pass. Fixed
 * PAL dimensions. Caller output may be partial on cancellation; owner must
 * publish only return1. Source and destination cannot overlap. */
int fh_install_movie_half_rgb(void*,unsigned capacity,const void*,unsigned bytes,FhInstallMovieDecodeProgress,void*);
/* Startup-only existing FHPICA01 ETC1 payload, optional LZ4+CRC. Input remains
 * borrowed/immutable. Scale1 or2; zero-initialize owner. No audio alteration,
 * GPU storage or gameplay JPEG work. An optional startup core2 owner encodes
 * disjoint rows; joins before return/cancel. Returns1/0/-1 as decode API. */
int fh_install_movie_frame(FhInstallMovieFrame*,const FhInstallMovieImage*,unsigned scale,FhInstallMovieFrameProgress,void*);
void fh_install_movie_frame_close(FhInstallMovieFrame*);
#ifdef __cplusplus
}
#endif
#endif
