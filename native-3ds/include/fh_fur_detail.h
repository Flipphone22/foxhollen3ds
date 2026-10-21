#ifndef FH_FUR_DETAIL_H
#define FH_FUR_DETAIL_H
/* User-authorized 3DS visual LOD. Zero retains every original shell. The
 * first shell always executes animation/hit-volume preparation; subsequent
 * samples span the original depth and retain their original noise indices. */
extern unsigned fh_fur_shell_budget,fh_fur_shells_original,fh_fur_shells_rendered;
extern unsigned fh_fur_shell_advance;
static inline unsigned fh_fur_detail_count(unsigned original,unsigned budget){
    return budget>=2 && budget<original?budget:original;
}
static inline unsigned fh_fur_detail_index(unsigned sample,unsigned count,unsigned original){
    return count<=1?0:(sample*(original-1)+(count-1)/2)/(count-1);
}
/* Joint shell expansion is cumulative in the pinned game. Preserve the
 * skipped depth steps instead of shrinking the fur to the first few layers.
 * This is visual LOD, not bit-exact multiplication-order equivalence. */
static inline float fh_fur_detail_scale(float step,unsigned advance){
    float result=1.f;for(unsigned i=0;i<advance;i++)result*=step;return result;
}
#endif
