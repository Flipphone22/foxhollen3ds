#include "fh_install_performance_pack.h"
#include "fh_install_geometry_iso.h"
#include <math.h>
#include <stdlib.h>
typedef struct{FhInstallPerformanceStageProgress function;void*context;unsigned phase;} Progress;
static int report(void*context,unsigned done,unsigned total){Progress*p=context;return !p->function||p->function(p->context,p->phase,done,total);}
int fh_install_performance_prepare(FhInstallTransaction*t,FhInstallDisc*d,unsigned spool_id,unsigned topology,unsigned geometry,float ratio,float error,FhInstallPerformanceStageProgress progress,void*context){
 if(!t||t->failed||!d||!d->file)return 0;
 if(spool_id>=FH_INSTALL_ARTIFACTS||topology>=FH_INSTALL_ARTIFACTS||geometry>=FH_INSTALL_ARTIFACTS||spool_id==topology||spool_id==geometry||topology==geometry||
    !isfinite(ratio)||ratio<=0||ratio>=1||!isfinite(error)||error<=0||error>.1f){t->failed=1;snprintf(t->error,sizeof(t->error),"Invalid performance preparation recipe.");return 0;}
 FhInstallGeometryScan*scan=NULL;FhInstallPerformanceSpool spool={0};int result=0;Progress tracking={progress,context,FH_INSTALL_PERF_REDUCE};
 if(!(t->state.complete&(1u<<spool_id))){
  if(!fh_install_performance_spool_begin(&spool,t,spool_id))goto done;
  spool.cursor.batch=64;
  spool.cursor.payload_bytes=1024*1024;
  scan=calloc(1,sizeof(*scan));if(!scan){t->failed=1;snprintf(t->error,sizeof(t->error),"Not enough startup memory for geometry scanner.");goto done;}
  if(!fh_install_geometry_scan_begin(scan,d,t->state.cursor[0],t->state.cursor[1]))goto scan_error;
  for(;;){
   if(progress&&!progress(context,FH_INSTALL_PERF_SCAN,scan->scanned_entries,scan->total_entries)){result=-1;goto done;}
   FhInstallGeometrySource source;int next=fh_install_geometry_scan_next(scan,&source);
   if(next==FH_INSTALL_GEO_ERROR)goto scan_error;
   if(scan->unsupported){snprintf(scan->error,sizeof(scan->error),"Unsupported indexed geometry source; conversion is incomplete.");goto scan_error;}
   if(next==FH_INSTALL_GEO_DONE)break;if(next==FH_INSTALL_GEO_SKIP)continue;
   result=fh_install_performance_spool_add(&spool,source.data,source.bytes,source.id,source.next_file,source.next_index,ratio,error,report,&tracking);
   if(result!=1)goto done;result=0;
  }
  if(progress&&!progress(context,FH_INSTALL_PERF_SCAN,scan->total_entries,scan->total_entries)){result=-1;goto done;}
  if(!fh_install_performance_spool_finish(&spool))goto done;fh_install_geometry_scan_close(scan);free(scan);scan=NULL;
 }
 tracking.phase=FH_INSTALL_PERF_TOPOLOGY_PACK;result=fh_install_performance_pack(t,spool_id,topology,FH_INSTALL_PERFORMANCE_TOPOLOGY,report,&tracking);
 if(result!=1)goto done;tracking.phase=FH_INSTALL_PERF_GEOMETRY_PACK;result=fh_install_performance_pack(t,spool_id,geometry,FH_INSTALL_PERFORMANCE_GEOMETRY,report,&tracking);goto done;
scan_error:snprintf(t->error,sizeof(t->error),"%s",scan->error);t->failed=1;
done:if(result==-1&&spool.entries&&!fh_install_cursor_flush(t,&spool.cursor))result=0;if(scan){fh_install_geometry_scan_close(scan);free(scan);}fh_install_performance_spool_close(&spool);if(result<=0)fh_install_transaction_close(t);return result;
}
