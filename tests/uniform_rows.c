#include "fh_uniform_rows.h"
#include <assert.h>
#include <stdio.h>
static FhUniformRows cache;
static uint32_t expected[96][4],device[96][4];
static unsigned checks,writes,requests;
static uint32_t seed=0x97cafa15;
static uint32_t random_word(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
static void upload(unsigned row,const uint32_t words[4]){
    float values[4];memcpy(values,words,16);memcpy(expected[row],words,16);requests++;
    if(fh_uniform_row_changed(&cache,row,values)){memcpy(device[row],values,16);writes++;}
}
static void check(void){assert(!memcmp(expected,device,sizeof(device)));checks++;}
int main(void){
    uint32_t inputs[96][4];
    for(unsigned r=0;r<96;r++)for(unsigned c=0;c<4;c++)inputs[r][c]=random_word();
    for(unsigned draw=0;draw<10000;draw++){
        if(draw%113==0){
            /* Program constants replace rows without application setters. */
            for(unsigned r=0;r<96;r++)for(unsigned c=0;c<4;c++)expected[r][c]=device[r][c]=random_word();
            fh_uniform_rows_reset(&cache);
        }
        unsigned row=random_word()%96;
        inputs[row][random_word()%4]=random_word();
        /* Alternate full lighting, flat lighting and transform-only draws.
         * Skipped values must remain installed, not match unused inputs. */
        for(unsigned r=0;r<96;r++){
            if(draw%3==1 && r>=47 && r!=87)continue;
            if(draw%3==2 && r>=34)continue;
            upload(r,inputs[r]);
        }
        check();
    }
    const uint32_t edge[][4]={{0,0,0,0},{0x80000000,0,0,0},{0x7fc01234,0x7f800000,1,0xff800000},
                              {0x7fc05678,0x7f800000,1,0xff800000}};
    for(unsigned i=0;i<4;i++){upload(93,edge[i]);check();unsigned before=writes;upload(93,edge[i]);assert(writes==before);check();}
    assert(writes<requests/5);
    printf("uniform row oracle: %u draws checked, %u requests, %u writes; shader clobbers, unused rows and exact float bits pass\n",checks,requests,writes);
}
