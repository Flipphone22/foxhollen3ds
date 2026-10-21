/* Load-time companion layout. Requires ordinary native actor inputs 0..5;
 * does not describe the separate groom shader's additional vertex stream. */
static int fh_pica_actor_fog_layout_prepare(C3D_AttrInfo *out,unsigned compact){
    if(!out || compact>1)return 0;
    C3D_AttrInfo a;AttrInfo_Init(&a);
    AttrInfo_AddLoader(&a,0,compact?GPU_SHORT:GPU_FLOAT,4);
    AttrInfo_AddLoader(&a,1,compact?GPU_UNSIGNED_BYTE:GPU_FLOAT,4);
    AttrInfo_AddLoader(&a,2,compact?GPU_SHORT:GPU_FLOAT,compact?4:2);
    AttrInfo_AddLoader(&a,3,compact?GPU_BYTE:GPU_FLOAT,compact?4:3);
    AttrInfo_AddLoader(&a,4,compact?GPU_UNSIGNED_BYTE:GPU_FLOAT,compact?4:3);
    if(compact)AttrInfo_AddLoader(&a,5,GPU_FLOAT,3);
    else AttrInfo_AddFixed(&a,5);
    AttrInfo_AddLoader(&a,6,GPU_FLOAT,3);
    AttrInfo_AddLoader(&a,7,GPU_UNSIGNED_BYTE,4);
    /* Pinned citro3d's AddFixed computes regId << (id*4) as an int:
     * attributes >=8 require a 64-bit permutation. Finish these two fixed
     * entries explicitly; their fixed bits were set by AttrInfo_Init. */
    a.attrCount=10;
    a.flags[1]=(a.flags[1]&~0xF0000000u)|(9u<<28);
    a.permutation=UINT64_C(0x9876543210);
    *out=a;
    return 1;
}
