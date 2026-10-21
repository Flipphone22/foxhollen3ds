#ifndef FH_INSTALL_MOVIE_PACK_H
#define FH_INSTALL_MOVIE_PACK_H
#include "fh_install_transaction.h"
#include "fh_install_movie_source.h"
enum{FH_INSTALL_MOVIE_PREP_HASH,FH_INSTALL_MOVIE_PREP_INDEX,FH_INSTALL_MOVIE_PREP_DECODE,FH_INSTALL_MOVIE_PREP_RESIZE,FH_INSTALL_MOVIE_PREP_ENCODE,FH_INSTALL_MOVIE_PREP_COMPRESS,FH_INSTALL_MOVIE_PREP_WRITE,FH_INSTALL_MOVIE_PREP_ASSEMBLE};
typedef int(*FhInstallMovieProgress)(void*,unsigned phase,unsigned frame,unsigned frames,unsigned done,unsigned total);
/* Two recipe-owned artifacts: durable frame spool and complete THP-compatible
 * movie. Repeated source JPEGs reuse encoded spool bytes; audio is always copied
 * from that particular original frame. Scale1/2. Cancellation is resumable at
 * frame boundaries, no incomplete movie or whole-game activation is published. */
int fh_install_movie_prepare(FhInstallTransaction*,FhInstallDisc*,unsigned spool,unsigned output,unsigned scale,FhInstallMovieProgress,void*);
/* Candidate native RGB8 movie, same half-res resize/audio/frame sequence.
 * Distinct recipe/spool format. Does not activate a runtime installation. */
int fh_install_movie_prepare_rgb8(FhInstallTransaction*,FhInstallDisc*,unsigned spool,unsigned output,FhInstallMovieProgress,void*);
#endif
