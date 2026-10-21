#ifndef FH_PICA_POST_STAGES_H
#define FH_PICA_POST_STAGES_H
/* Material programs own 0..3. World height fog and actor colour fade
 * are mutually exclusive users of 4; their owners invalidate each other on
 * publication. GX distance fog follows all TEV work in slot 5. Actor height
 * fog remains a separate missing family and must precede actor colour fade.
 * Texture/procedural unit numbers are unrelated to combiner slots. */
enum { FH_PICA_COLOR_FADE_STAGE=4, FH_PICA_WORLD_HEIGHT_FOG_STAGE=4,
       FH_PICA_FOG_STAGE=5 };
#endif
