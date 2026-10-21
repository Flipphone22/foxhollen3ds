#include "fh_install_geometry_pack.h"
#include "fh_install_geometry_iso.h"
#include <stdlib.h>

typedef struct{FhInstallGeometryStageProgress function;void*context;} Progress;
static int assembly(void*context,unsigned done,unsigned total){
    Progress*p=context;return !p->function||p->function(p->context,FH_INSTALL_GEOMETRY_ASSEMBLE,done,total);
}
int fh_install_geometry_prepare(FhInstallTransaction*t,FhInstallDisc*d,unsigned spool_id,unsigned output,FhInstallGeometryStageProgress progress,void*context){
    if(!t||t->failed||!d||!d->file||spool_id>=FH_INSTALL_ARTIFACTS||output>=FH_INSTALL_ARTIFACTS||spool_id==output)return 0;
    FhInstallGeometryScan*scan=NULL;FhInstallGeometrySpool spool={0};int result=0;
    if(!(t->state.complete&(1u<<spool_id))){
        if(!fh_install_geometry_spool_begin(&spool,t,spool_id))goto done;
        spool.cursor.batch=64;
        spool.cursor.payload_bytes=1024*1024;
        scan=calloc(1,sizeof(*scan));if(!scan){snprintf(t->error,sizeof(t->error),"Not enough startup memory for geometry scanner.");t->failed=1;goto done;}
        if(!fh_install_geometry_scan_begin(scan,d,t->state.cursor[0],t->state.cursor[1]))goto scan_error;
        for(;;){
            if(progress&&!progress(context,FH_INSTALL_GEOMETRY_SCAN,scan->scanned_entries,scan->total_entries)){result=-1;goto done;}
            FhInstallGeometrySource source;int next=fh_install_geometry_scan_next(scan,&source);
            if(next==FH_INSTALL_GEO_ERROR)goto scan_error;
            if(scan->unsupported){snprintf(scan->error,sizeof(scan->error),"Unsupported indexed geometry source; conversion is incomplete.");goto scan_error;}
            if(next==FH_INSTALL_GEO_DONE)break;
            if(next==FH_INSTALL_GEO_SKIP)continue;
            if(!fh_install_geometry_spool_add(&spool,source.data,source.bytes,source.id,source.next_file,source.next_index))goto done;
        }
        if(progress&&!progress(context,FH_INSTALL_GEOMETRY_SCAN,scan->total_entries,scan->total_entries)){result=-1;goto done;}
        if(!fh_install_geometry_spool_finish(&spool))goto done;
        fh_install_geometry_scan_close(scan);free(scan);scan=NULL;
    }
    {Progress tracking={progress,context};result=fh_install_geometry_pack(t,spool_id,output,assembly,&tracking);}goto done;
scan_error:
    snprintf(t->error,sizeof(t->error),"%s",scan->error);t->failed=1;
done:
    if(result==-1&&spool.entries&&!fh_install_cursor_flush(t,&spool.cursor))result=0;
    if(scan){fh_install_geometry_scan_close(scan);free(scan);}fh_install_geometry_spool_close(&spool);
    if(result<=0)fh_install_transaction_close(t);return result;
}
