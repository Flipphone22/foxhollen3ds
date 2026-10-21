#ifndef FH_PICA_MOTION_BLUR_H
#define FH_PICA_MOTION_BLUR_H
/* Called at the original post-object/pre-secondary-effects boundary. */
void fh_pica_motion_blur_step(float alpha,unsigned requested,unsigned capture);
#endif
