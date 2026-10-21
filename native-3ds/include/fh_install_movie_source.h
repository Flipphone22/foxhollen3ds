#ifndef FH_INSTALL_MOVIE_SOURCE_H
#define FH_INSTALL_MOVIE_SOURCE_H
#include "fh_install_disc.h"
enum{FH_INSTALL_MOVIE_FRAMES=1797,FH_INSTALL_MOVIE_SOURCE_BYTES=48931320};
enum{FH_INSTALL_MOVIE_HASH,FH_INSTALL_MOVIE_INDEX};
typedef int(*FhInstallMovieSourceProgress)(void*,unsigned phase,unsigned done,unsigned total);
typedef struct{unsigned offset,bytes,video,audio;}FhInstallMovieIndex;
typedef struct{
 FhInstallDisc*disc;unsigned file,disc_offset,bytes;
 FhInstallMovieIndex*index;unsigned char*frame;
 unsigned char header[48],info[40],sha[32];char error[112];
}FhInstallMovieSource;
/* Startup-only exclusive borrowing of ISO cursor. Header, complete source hash
 * and all frame links are checked before exposing frames. Zero initialize.
 * Return1/0/-1 for complete/error/cancel. ISO stays open and unmodified. */
int fh_install_movie_source_begin(FhInstallMovieSource*,FhInstallDisc*,FhInstallMovieSourceProgress,void*);
int fh_install_movie_source_read(FhInstallMovieSource*,unsigned frame);
void fh_install_movie_source_close(FhInstallMovieSource*);
#endif
