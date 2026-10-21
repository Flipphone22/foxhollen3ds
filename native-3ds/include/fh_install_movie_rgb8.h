#ifndef FH_INSTALL_MOVIE_RGB8_H
#define FH_INSTALL_MOVIE_RGB8_H
#include "fh_install_movie_frame.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Candidate lossless encoding of the existing half-resolution Lanczos result.
 * Independent native RGB8 frames, no temporal history. Preparation only;
 * runtime support and memory/SD acceptance are separate requirements. */
int fh_install_movie_rgb8_frame(FhInstallMovieFrame*,const FhInstallMovieImage*,FhInstallMovieFrameProgress,void*);
#ifdef __cplusplus
}
#endif
#endif
