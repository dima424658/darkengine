#pragma once
#ifdef __cplusplus
extern "C" {
#endif
void ResImageCloneSource(const void *original,const void *replacement);
void ResImageForgetSource(void *bitmap);
#ifdef __cplusplus
}
#endif
