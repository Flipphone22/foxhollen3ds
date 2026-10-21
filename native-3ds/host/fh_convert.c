#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include "fh_install_task.h"
#include "install_recipe.h"

static const char *stage_names[]={"Verify disc","Geometry","Performance geometry","Textures","Movie","Publish"};

static int progress(void *context,const char *label,uint32_t done,uint32_t total){
    time_t *last=context;
    if(!label||strncmp(label,"@install/",9))return 1;
    unsigned stage=(unsigned)(label[9]-'0');
    time_t now=time(NULL);
    if(now==*last&&done!=total)return 1;
    *last=now;
    printf("\r%-22s %5.1f%%",stage<6?stage_names[stage]:"",total?100.0*done/total:0.0);
    fflush(stdout);
    return 1;
}

int main(int argc,char **argv){
    if(argc!=3){
        fprintf(stderr,"usage: %s DISC.iso OUTPUT-DIR\n",argv[0]);
        return 2;
    }
    mkdir(argv[2],0755);
    FhInstallTask task={.disc_path=argv[1],.root=argv[2],.spec=fh_install_game_spec};
    if(fh_install_task_ready(&task)){
        printf("%s already holds a complete prepared bundle for this disc\n",argv[2]);
        return 0;
    }
    char error[256]={0};
    time_t last=0;
    int result=fh_install_task_prepare(&task,progress,&last,error,sizeof error);
    printf("\n");
    if(result!=1){
        fprintf(stderr,"conversion failed: %s\n",error[0]?error:"cancelled");
        return 1;
    }
    if(!fh_install_task_ready(&task)){
        fprintf(stderr,"conversion finished but the bundle does not verify\n");
        return 1;
    }
    printf("prepared bundle written to %s\n",argv[2]);
    return 0;
}
