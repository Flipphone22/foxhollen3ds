/* Immutable native vertex fetch layouts, compiled once with the pinned SDK.
 * Publish through Citro3D so program/FBO/APT invalidation remains authoritative.
 * Fixed attribute values remain live SDK state and are restored on each bind. */
static C3D_AttrInfo vertex_layouts[10];
static unsigned vertex_layout_binds[2];
static void fh_pica_vertex_layouts_init(void){
    vertex_layout_binds[0]=vertex_layout_binds[1]=0;
    for(unsigned compact=0;compact<2;compact++)for(unsigned textured=0;textured<2;textured++){
        C3D_AttrInfo *a=vertex_layouts+compact*2+textured;AttrInfo_Init(a);
        AttrInfo_AddLoader(a,0,compact?GPU_SHORT:GPU_FLOAT,4);
        AttrInfo_AddLoader(a,1,compact?GPU_UNSIGNED_BYTE:GPU_FLOAT,4);
        if(textured){
            AttrInfo_AddLoader(a,2,compact?GPU_SHORT:GPU_FLOAT,2);
            AttrInfo_AddLoader(a,3,compact?GPU_SHORT:GPU_FLOAT,compact?2:3);
        }
    }
    for(unsigned compact=0;compact<2;compact++)for(unsigned fur=0;fur<3;fur++){
        C3D_AttrInfo *a=vertex_layouts+4+compact*3+fur;AttrInfo_Init(a);
        if(compact){
            AttrInfo_AddLoader(a,0,GPU_SHORT,4);AttrInfo_AddLoader(a,1,GPU_UNSIGNED_BYTE,4);
            AttrInfo_AddLoader(a,2,GPU_SHORT,4);AttrInfo_AddLoader(a,3,GPU_BYTE,4);
            AttrInfo_AddLoader(a,4,GPU_UNSIGNED_BYTE,4);AttrInfo_AddLoader(a,5,GPU_FLOAT,3);
        }else{
            AttrInfo_AddLoader(a,0,GPU_FLOAT,4);AttrInfo_AddLoader(a,1,GPU_FLOAT,4);
            AttrInfo_AddLoader(a,2,GPU_FLOAT,2);AttrInfo_AddLoader(a,3,GPU_FLOAT,3);
            AttrInfo_AddLoader(a,4,GPU_FLOAT,3);AttrInfo_AddFixed(a,5);
        }
        if(fur==1)AttrInfo_AddFixed(a,6);
        else if(fur==2)AttrInfo_AddLoader(a,6,GPU_BYTE,4);
    }
}
static void fh_pica_world_layout_bind(unsigned compact,unsigned textured){
    C3D_SetAttrInfo(vertex_layouts+(!!compact)*2+!!textured);vertex_layout_binds[0]++;
}
static void fh_pica_actor_layout_bind(unsigned compact,unsigned fur,unsigned groom){
    C3D_SetAttrInfo(vertex_layouts+4+(!!compact)*3+(fur?(groom?2:1):0));
    if(!compact)C3D_FixedAttribSet(5,0,0,0,0);
    if(fur&&!groom)C3D_FixedAttribSet(6,0,0,0,0);
    vertex_layout_binds[1]++;
}
static void fh_pica_vertex_layouts_report(void){
    char line[160];snprintf(line,sizeof(line),"PICA vertex layouts: prepared=10 bytes=%u world_binds=%u actor_binds=%u runtime_construction=0\n",
        (unsigned)sizeof(vertex_layouts),vertex_layout_binds[0],vertex_layout_binds[1]);fh_platform_log(line);
}
