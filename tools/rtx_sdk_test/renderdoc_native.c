// Linux/GDB helper: explicitly capture one settled native SDK frame.
#define _GNU_SOURCE
#include <renderdoc_app.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
static RENDERDOC_API_1_6_0 *api;
void dark_capture_start(void) {
 void *module=dlopen("librenderdoc.so",RTLD_NOW|RTLD_NOLOAD);
 pRENDERDOC_GetAPI get=module?(pRENDERDOC_GetAPI)dlsym(module,"RENDERDOC_GetAPI"):NULL;
 int available=get && get(eRENDERDOC_API_Version_1_6_0,(void**)&api);
 fprintf(stderr,"Native RenderDoc API available=%d\n",available);
 if(available){
  const char *path=getenv("DARK_NATIVE_CAPTURE");
  api->SetCaptureFilePathTemplate(path?path:"/tmp/dark-sdk-native");
  api->StartFrameCapture(NULL,NULL);
 }
}
void dark_capture_end(void) {if(api)fprintf(stderr,"Native RenderDoc capture result=%u\n",api->EndFrameCapture(NULL,NULL));}
