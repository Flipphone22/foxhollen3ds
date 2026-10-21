#include "fh_ground_result.h"
#include <assert.h>
#include <stdio.h>
int main(void){
    FhGroundResults s={.generation=1};FhGroundResultKey key={0};
    key.position[0]=123.5f;key.position[1]=47;key.position[2]=-82.75f;
    key.origin[0]=640;key.origin[1]=-1280;key.count=2;key.generation=s.generation;
    key.cells[0]=0x100000;key.cells[1]=0x200000;key.grid[1][0]=640;
    FhGroundQuery q={.hits=1,.best_height=42,.normal={0,1,0}},out={0};
    assert(!fh_ground_result_get(&s,&key,&out));fh_ground_result_put(&s,&key,&q);
    assert(fh_ground_result_get(&s,&key,&out)&&out.best_height==42);
    FhGroundResultKey changed=key;changed.cells[0]=0x300000;
    assert(!fh_ground_result_get(&s,&changed,&out));
    changed=key;changed.grid[1][0]+=640;assert(!fh_ground_result_get(&s,&changed,&out));
    changed=key;changed.origin[0]+=1;assert(!fh_ground_result_get(&s,&changed,&out));
    changed=key;changed.count=1;assert(!fh_ground_result_get(&s,&changed,&out));
    changed=key;changed.position[1]+=.001f;assert(!fh_ground_result_get(&s,&changed,&out));
    fh_ground_results_changed(&s);key.generation=s.generation;
    assert(!fh_ground_result_get(&s,&key,&out));
    q.best_height=55;fh_ground_result_put(&s,&key,&q);assert(fh_ground_result_get(&s,&key,&out)&&out.best_height==55);
    q.hits=0;fh_ground_results_changed(&s);key.generation=s.generation;
    fh_ground_result_put(&s,&key,&q);assert(fh_ground_result_get(&s,&key,&out)&&!out.hits);
    s.generation=UINT64_MAX;fh_ground_results_changed(&s);key.generation=1;
    assert(!fh_ground_result_get(&s,&key,&out));
    for(unsigned i=0;i<10000;i++){
        key.position[0]=i;key.generation=s.generation;q.hits=1;q.best_height=i;
        fh_ground_result_put(&s,&key,&q);assert(fh_ground_result_get(&s,&key,&out)&&out.best_height==i);
    }
    printf("PASS exact query, mutation, origin, membership/order, zero-hit, epoch wrap and bounded eviction; %zu bytes\n",sizeof(s));
}
