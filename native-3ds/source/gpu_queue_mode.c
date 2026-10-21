#include "fh_gpu_queue_mode.h"
static unsigned early;
unsigned fh_gpu_queue_early(void){return early;}
int fh_gpu_queue_select(unsigned value){if(value>1)return 0;early=value;return 1;}
