void source_walk_direction(FhPadState pad,int camera,float*x,float*z){
 int px=abs(pad.x),py=abs(pad.y);px=px<=15?0:(px-15)*72/112;py=py<=15?0:(py-15)*72/112;
 int d=px>=py?40*px+32*py:40*py+32*px;if(d>2880){px=px*2880/d;py=py*2880/d;}
 if(pad.x<0)px=-px;if(pad.y<0)py=-py;
 float magnitude=fminf(56,hypotf(px,py))/56;
 if(magnitude<.3f)magnitude=0;
 MatrixTransform tr={.rotX=(short)(getAngle(px,-py)-camera),.scale=1};float m[16],y;
 setMatrixFromObjectPos(m,&tr);Matrix_TransformPoint(m,0,0,-2.5f*magnitude,x,&y,z);
}
