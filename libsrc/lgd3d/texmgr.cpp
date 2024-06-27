#include <types.h>
#include <d6States.h>


texture_manager *get_dopey_texture_manager(cD6States *driver);
uint8 *tmgr_set_clut(uint8 *clut); 
unsigned int tmgr_bytes_loaded(); 
int tmgr_get_utilization(float *utilization); 
void tmgr_restore_bits(grs_bitmap *bm); 
void swapout_bitmap(tdrv_texture_info *info, tmap_chain *chain); 
void do_unload(grs_bitmap *bm); 
void release_texture(int i); 
int dump_all_textures();
void tmgr_unload_texture(grs_bitmap *bm); 
void tmgr_reload_texture(grs_bitmap *bm); 
void tmgr_load_texture(grs_bitmap *bm); 
int do_load_1(tdrv_texture_info *info); 
void tmgr_set_texture(grs_bitmap *bm); 
void do_set_texture(grs_bitmap *bm); 
void tmgr_set_texture_callback();
void tmgr_start_frame(int frame); 
void tmgr_end_frame();
int tmgr_init(grs_bitmap *bm, int num_textures, int *size_list, int num_sizes);
tmgr_texture_info *init_bitmap_list();
void tmgr_shutdown();
int tmgr_stats();

int lgd3d_enumerate_devices();
int lgd3d_enumerate_devices_capable_of(unsigned int flags);
void lgd3d_unenumerate_devices();
lgd3ds_device_info *lgd3d_get_device_info(int device_number);

void GetDevices(LGD3D_sEnumerationInfo *info);

BOOL CALLBACK c_DDEnumCallback(GUID* lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName, LPVOID lpContext);
HRESULT CALLBACK c_EnumDisplayModesCallback(LPDDSURFACEDESC2 pSD, LPVOID data);
HRESULT CALLBACK c_EnumDevicesCallback(GUID* lpGuid, LPSTR lpDeviceDescription, LPSTR lpDeviceName, LPD3DDEVICEDESC pDeviceDesc, LPD3DDEVICEDESC lpD3DHELDeviceDesc, LPVOID lpUserArg);
