#include "fh_replay.h"
#include <math.h>
#include <stdlib.h>
typedef float f32;typedef unsigned short u16;
typedef struct {short rotX,rotY,rotZ;float scale,x,y,z;} MatrixTransform;
static float gVecMathZero[1];
static void angleToVec2(unsigned short a,float*s,float*c){*s=sinf(a*(3.14159265358979323846f/32768));*c=cosf(a*(3.14159265358979323846f/32768));}
