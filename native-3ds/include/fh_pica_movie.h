#ifndef FH_PICA_MOVIE_H
#define FH_PICA_MOVIE_H
/* The decoder writes directly into three bounded, GPU-owned native images.
 * Begin retires readers before writes; commit publishes only verified frames. */
void *fh_pica_movie_begin(unsigned slot,unsigned width,unsigned height);
void *fh_pica_movie_begin_format(unsigned slot,unsigned width,unsigned height,unsigned format);
void fh_pica_movie_commit(void *pixels);
int fh_pica_movie_bind(const void *pixels,unsigned format,unsigned compact);
void fh_pica_movie_shutdown(void);
int fh_pica_game_movie_matches(const void *callback);
const void *fh_pica_game_movie_pixels(void);
#endif
