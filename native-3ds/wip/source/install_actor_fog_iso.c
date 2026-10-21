#include "fh_install_actor_fog_iso.h"
#include "fh_install_actor_fog_topology.h"
#include "fh_install_actor_fog_width_catalog.h"
#include "fh_install_geometry_iso.h"
#include <stdlib.h>
#include <string.h>
static int fail(FhInstallTransaction*t,const char*why){t->failed=1;snprintf(t->error,sizeof(t->error),"%s",why);return 0;}
int fh_install_actor_fog_iso(FhInstallTransaction*t,FhInstallDisc*d,unsigned artifact,
    const char*topology,FhInstallActorFogCoverage*coverage,FhInstallActorFogProgress progress,void*context){
    if(!t||t->failed||t->output||!d||!d->file||artifact>=FH_INSTALL_ARTIFACTS)return 0;
    if(coverage)memset(coverage,0,sizeof(*coverage));
    if(t->state.complete&(1u<<artifact))return 1;
    FhInstallActorFogCoverage stats={0};FhInstallActorFogCollection collection={0};
    FhInstallActorFogTopology patches={0};int result=0;
    FhInstallGeometryScan*scan=calloc(1,sizeof(*scan));
    FhInstallActorFogWidthCatalog*widths=calloc(1,sizeof(*widths));
    if(!scan||!widths){fail(t,"Not enough startup memory for actor fog source scanner.");goto done;}
    if(topology&&!fh_install_actor_fog_topology_open(&patches,topology)){fail(t,"Invalid prepared actor fog topology input.");goto done;}
    if(!fh_install_actor_fog_width_catalog_begin(widths,d)){fail(t,widths->error);goto done;}
    if(!fh_install_actor_fog_collection_begin(&collection,t,artifact))goto done;
    if(!fh_install_geometry_scan_begin(scan,d,t->state.cursor[0],t->state.cursor[1])){fail(t,scan->error);goto done;}
    for(;;){
        if(progress&&!progress(context,scan->scanned_entries,scan->total_entries)){result=-1;break;}
        FhInstallGeometrySource source;int next=fh_install_geometry_scan_next(scan,&source);
        if(next==FH_INSTALL_GEO_ERROR||scan->unsupported){fail(t,scan->error[0]?scan->error:"Unsupported ISO geometry record in fog preparation.");goto done;}
        if(next==FH_INSTALL_GEO_DONE){result=1;break;}
        /* Empty table slots have no new payload or decoded source to retain.
         * They are cheap to revisit after cancellation; syncing each batch
         * would turn ~120k sparse slots into thousands of needless SD commits. */
        if(next==FH_INSTALL_GEO_SKIP)continue;
        if(source.id>>31||!source.data[0xf8]||!source.data[0xf5]){
            if(source.id>>31)stats.world++;else stats.empty++;
            if(!fh_install_cursor_duplicate(t,&collection.cursor,source.next_file,source.next_index))goto done;
            continue;
        }
        stats.models++;
        if(topology&&!fh_install_actor_fog_topology_apply(&patches,scan->decoded,source.bytes,source.id)){fail(t,"Cannot apply prepared actor fog topology.");goto done;}
        for(unsigned bump=0;bump<2;bump++){
            /* A checkpoint after state zero must replay this source record.
             * Otherwise power loss between states silently drops state one. */
            unsigned index=source.next_index-(bump==0);
            if(!fh_install_actor_fog_collection_add(&collection,source.data,source.bytes,source.id,bump,
                fh_install_actor_fog_width_catalog,widths,source.next_file,index)){
                char why[112];snprintf(why,sizeof(why),"Fog file %u model %u state %u: %.60s",source.file,source.id,bump,widths->error[0]?widths->error:t->error);
                fail(t,why);goto done;
            }
        }
    }
    if(result==-1&&!fh_install_cursor_flush(t,&collection.cursor))result=0;
    stats.records=collection.collection.count;stats.duplicates=collection.duplicates;
    if(result==1)result=fh_install_actor_fog_collection_finish(&collection);
done:
    if(widths){stats.texture_banks=widths->count;stats.texture_definitions=widths->definitions;stats.texture_workspace_peak=widths->workspace_peak;}
    stats.topology_applied=patches.applied;if(coverage)*coverage=stats;
    fh_install_actor_fog_topology_close(&patches);fh_install_actor_fog_collection_close(&collection);
    if(scan){fh_install_geometry_scan_close(scan);free(scan);}free(widths);return result;
}
