#include "fh_startup_runner.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    FhStartupMenu menu;
    const FhStartupDisplay *display;
    const char *names[16];
    uint64_t last_poll,last_draw;
    unsigned cancel,exit_requested;
    char previous[2048];
} Runner;

static void draw(Runner*r){
    char text[2048];
    if(!fh_startup_menu_text(&r->menu,r->names,text,sizeof(text)))return;
    if(strcmp(r->previous,text)){
        r->display->present(r->display->context,text);
        memcpy(r->previous,text,strlen(text)+1);
    }
}
static int progress(void*context,const char*phase,uint32_t done,uint32_t total){
    Runner*r=context;const FhStartupDisplay*d=r->display;
    uint64_t now=d->milliseconds(d->context);
    if(!phase){r->cancel=1;return 0;}
    if(!strncmp(phase,"@install/",9)){
        if(strlen(phase)!=10||phase[9]<'0'||phase[9]>'5'||
           !fh_install_eta_update(&r->menu.estimate,(unsigned)(phase[9]-'0'),done,total,now)){r->cancel=1;return 0;}
    }else if(!fh_startup_menu_progress(&r->menu,phase,done,total)){r->cancel=1;return 0;}
    if(now>=r->menu.estimate.last)r->menu.estimate.last=now;
    if(now-r->last_poll>=16){
        unsigned buttons=0;r->last_poll=now;
        if(!d->poll(d->context,&buttons)){r->exit_requested=r->cancel=1;}
        if(fh_startup_menu_input(&r->menu,buttons)==FH_STARTUP_CANCEL)r->cancel=1;
    }
    if(now-r->last_draw>=100){r->last_draw=now;draw(r);}
    return !r->cancel;
}
int fh_startup_run(const char*build,unsigned hardware_new,const FhStartupTasks*t,const FhStartupDisplay*d){
    if(!t||!t->ready||!d||!d->milliseconds||!d->poll||!d->present||!d->wait||
       t->diagnostic_count>16||(t->diagnostic_count&&!t->diagnostics))return -1;
    for(unsigned i=0;i<t->diagnostic_count;i++)if(!t->diagnostics[i].name||!t->diagnostics[i].run)return -1;
    Runner r={0};r.display=d;
    for(unsigned i=0;i<t->diagnostic_count;i++)r.names[i]=t->diagnostics[i].name;
    fh_startup_menu_init(&r.menu,build,hardware_new,t->ready(t->context),t->diagnostic_count);
    for(;;){
        draw(&r);unsigned buttons=0;
        if(!d->poll(d->context,&buttons))return 0;
        unsigned action=fh_startup_menu_input(&r.menu,buttons);
        if(action==FH_STARTUP_EXIT)return 0;
        if(action==FH_STARTUP_PLAY||action==FH_STARTUP_BENCHMARK){
            /* An earlier ready flag cannot authorize changed/removed assets. */
            if(t->ready(t->context))return action==FH_STARTUP_BENCHMARK?2:1;
            fh_startup_menu_finish(&r.menu,0,"Prepared assets changed. Prepare or resume them.");
        }else if(action==FH_STARTUP_SAVE_SETTINGS){
            snprintf(r.menu.message,sizeof(r.menu.message),"%s",fh_graphics_settings_save()?"Graphics preset saved.":"Could not save settings. Selection is for this launch only.");
        }else if(action==FH_STARTUP_PREPARE||action==FH_STARTUP_RUN_PROBE){
            FhStartupTask task=t->prepare;void*context=t->context;
            if(action==FH_STARTUP_RUN_PROBE){task=t->diagnostics[r.menu.probe].run;context=t->diagnostics[r.menu.probe].context;}
            char message[128]={0};r.cancel=0;r.last_poll=r.last_draw=d->milliseconds(d->context);
            memset(&r.menu.estimate,0,sizeof(r.menu.estimate));
            if(action==FH_STARTUP_PREPARE)fh_install_eta_begin(&r.menu.estimate,r.last_poll);
            draw(&r);
            int result=task?task(context,progress,&r,message,sizeof(message)):0;
            message[sizeof(message)-1]=0;
            if(!message[0])snprintf(message,sizeof(message),"%s",!task?"Preparation is not available in this build.":result==1?"Task finished.":result==-1?"Cancelled. Completed checkpoints are retained.":"Task failed. Previous prepared assets are retained.");
            fh_startup_menu_finish(&r.menu,t->ready(t->context),message);
            if(r.exit_requested)return 0;
        }
        d->wait(d->context);
    }
}
