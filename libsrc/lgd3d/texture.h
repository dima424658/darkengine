// $Header: x:/prj/tech/libsrc/lgd3d/RCS/texture.h 1.6 1998/02/02 12:52:06 KEVIN Exp $

#ifndef __TEXTURE_H
#define __TEXTURE_H

extern D3DTEXTUREHANDLE TextureHandle[];
extern D3DMATERIALHANDLE MaterialHandle[];
extern grs_bitmap *TdrvBitmap[];

typedef struct lgd3ds_device_info lgd3ds_device_info;
extern void InitTextureSys(lgd3ds_device_info *info);
extern void ShutdownTextureSys(void);

#define LGD3D_MAX_TEXTURES 1024

#endif
