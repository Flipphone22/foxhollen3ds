/* Headless OpenGL byte oracle for Azahar's D24S8 reinterpretation shader.
 * Upload every possible packed D24 value and compare the shader's actual
 * RGBA8 output to those original bytes; no game-derived expected image. */
#define GL_GLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GLuint shader(GLenum type,const char *source){
    GLuint id=glCreateShader(type);glShaderSource(id,1,&source,NULL);glCompileShader(id);
    GLint ok;glGetShaderiv(id,GL_COMPILE_STATUS,&ok);
    if(!ok){char log[4096];glGetShaderInfoLog(id,sizeof(log),NULL,log);fprintf(stderr,"%s\n",log);exit(1);}
    return id;
}
static GLuint program(const char *path){
    FILE *f=fopen(path,"rb");assert(f);fseek(f,0,SEEK_END);long length=ftell(f);rewind(f);
    char *source=calloc(length+64,1);assert(source);strcpy(source,"#version 430 core\n");
    assert(fread(source+strlen(source),1,length,f)==(size_t)length);fclose(f);
    const char *vertex="#version 430 core\nlayout(location=0)out vec2 tex_coord;void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);tex_coord=p;gl_Position=vec4(p*2.0-1.0,0,1);}";
    GLuint p=glCreateProgram(),v=shader(GL_VERTEX_SHADER,vertex),s=shader(GL_FRAGMENT_SHADER,source);
    glAttachShader(p,v);glAttachShader(p,s);glLinkProgram(p);GLint ok;glGetProgramiv(p,GL_LINK_STATUS,&ok);assert(ok);
    glDeleteShader(v);glDeleteShader(s);free(source);return p;
}
int main(int argc,char **argv){
    assert(argc==3);
    PFNEGLGETPLATFORMDISPLAYEXTPROC get_display=(void*)eglGetProcAddress("eglGetPlatformDisplayEXT");assert(get_display);
    EGLDisplay display=get_display(EGL_PLATFORM_SURFACELESS_MESA,EGL_DEFAULT_DISPLAY,NULL);assert(eglInitialize(display,NULL,NULL));
    assert(eglBindAPI(EGL_OPENGL_API));EGLint attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_BIT,EGL_NONE};
    EGLConfig config;EGLint count;assert(eglChooseConfig(display,attrs,&config,1,&count)&&count==1);
    EGLint context_attrs[]={EGL_CONTEXT_MAJOR_VERSION,4,EGL_CONTEXT_MINOR_VERSION,3,EGL_CONTEXT_OPENGL_PROFILE_MASK,EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,EGL_NONE};
    EGLContext context=eglCreateContext(display,config,EGL_NO_CONTEXT,context_attrs);assert(context!=EGL_NO_CONTEXT);
    EGLint surface_attrs[]={EGL_WIDTH,1,EGL_HEIGHT,1,EGL_NONE};
    EGLSurface surface=eglCreatePbufferSurface(display,config,surface_attrs);assert(surface!=EGL_NO_SURFACE);
    assert(eglMakeCurrent(display,surface,surface,context));
    fprintf(stderr,"Renderer: %s\n",glGetString(GL_RENDERER));
    enum {W=1024,H=256,N=W*H};
    uint32_t *packed=malloc(N*4);uint8_t *rgba=malloc(N*4);assert(packed&&rgba);
    GLuint tex[3],fbo,vao;glGenTextures(3,tex);
    for(unsigned i=0;i<3;i++){
        glActiveTexture(GL_TEXTURE0+i);glBindTexture(GL_TEXTURE_2D,tex[i]);
        glTexStorage2D(GL_TEXTURE_2D,1,i<2?GL_DEPTH24_STENCIL8:GL_RGBA8,W,H);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        if(i<2)glTexParameteri(GL_TEXTURE_2D,GL_DEPTH_STENCIL_TEXTURE_MODE,i?GL_STENCIL_INDEX:GL_DEPTH_COMPONENT);
    }
    glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,tex[2],0);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
    glGenVertexArrays(1,&vao);glBindVertexArray(vao);glViewport(0,0,W,H);
    uint64_t mismatch[3]={0};
    for(unsigned mode=0;mode<3;mode++){
        GLuint p=mode<2?program(argv[1+mode]):0;glUseProgram(p);
        for(unsigned start=0;start<(1u<<24);start+=N){
            for(unsigned i=0;i<N;i++)packed[i]=((start+i)<<8)|((i*37)&255);
            for(unsigned t=0;t<2;t++){
                glActiveTexture(GL_TEXTURE0+t);glBindTexture(GL_TEXTURE_2D,tex[t]);
                glTexSubImage2D(GL_TEXTURE_2D,0,0,0,W,H,GL_DEPTH_STENCIL,GL_UNSIGNED_INT_24_8,packed);
            }
            if(mode<2){glDrawArrays(GL_TRIANGLES,0,3);glReadPixels(0,0,W,H,GL_RGBA,GL_UNSIGNED_BYTE,rgba);}
            else {
                glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,tex[0]);
                glGetTexImage(GL_TEXTURE_2D,0,GL_DEPTH_STENCIL,GL_UNSIGNED_INT_24_8,rgba);
            }
            assert(glGetError()==GL_NO_ERROR);
            for(unsigned i=0;i<N;i++){
                uint32_t actual;
                if(mode==2)memcpy(&actual,rgba+4*i,4);
                else actual=((uint32_t)rgba[4*i+1]<<24)|((uint32_t)rgba[4*i+2]<<16)|((uint32_t)rgba[4*i+3]<<8)|rgba[4*i];
                if(actual!=packed[i]){
                    if(mismatch[mode]<3)fprintf(stderr,"mode%u packed=%08x actual=%08x\n",mode,packed[i],actual);
                    mismatch[mode]++;
                }
            }
        }
        glDeleteProgram(p);
    }
    printf("{\"packed_values_per_route\":16777216,\"old_mismatches\":%llu,\"rounding_candidate_mismatches\":%llu,\"packed_readback_mismatches\":%llu}\n",(unsigned long long)mismatch[0],(unsigned long long)mismatch[1],(unsigned long long)mismatch[2]);
    glDeleteFramebuffers(1,&fbo);glDeleteTextures(3,tex);glDeleteVertexArrays(1,&vao);free(packed);free(rgba);
    eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroySurface(display,surface);eglDestroyContext(display,context);eglTerminate(display);
    return !mismatch[0] || !mismatch[1] || mismatch[2]!=0;
}
