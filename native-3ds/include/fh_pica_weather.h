#ifndef FH_PICA_WEATHER_H
#define FH_PICA_WEATHER_H
#include <stdint.h>
enum { FH_WEATHER_BATCH=11, FH_WEATHER_SHARED_BATCH=80, FH_WEATHER_SHAPES=20, FH_WEATHER_STORAGE=512 };
/* Persistent corner/instance IDs; live source centres and shared shape rows
 * reach uniforms, never CPU-expanded triangles. Source owns simulation. */
void fh_pica_weather_init(void);
void fh_pica_weather_load(void);
void fh_pica_weather_invalidate(void);
void fh_pica_weather_begin(const float view[3][4],const float projection[4][4],
    const void *shapes,unsigned stride,const int16_t uv[6],uint32_t rgba,const unsigned clip[4]);
void fh_pica_weather_particle(unsigned page,float x,float y,float z,float size,unsigned shape);
void fh_pica_weather_end(void);
void fh_pica_weather_shutdown(void);
#endif
