#include "d6States.h"

#include <wdispapi.h>

class cImStates : public cD6States
{
public:
    cD6States *Instance();
    virtual cD6States *DeInstance() override;

protected:
    cImStates(const class cImStates &);
    cImStates();
    virtual ~cImStates();

private:
    cImStates *m_Instance;
};

class cMSStates : public cD6States {
public:
    cD6States * Instance();
    virtual cD6States * DeInstance() override;
    virtual int Initialize(DWORD dwRequestedFlags) override;
    virtual int SetDefaultsStates(DWORD dwRequestedFlags) override;

protected:
    cMSStates(const cMSStates &);
    cMSStates();
    virtual ~cMSStates();

private:
    cMSStates * m_Instance;
    unsigned long m_dwCurrentTexLevel;
    int m_bTexturePending;
    grs_bitmap * m_LastLightMapBm;

public:
    virtual void set_texture_id(int n) override;
    virtual int reload_texture(tdrv_texture_info *info) override;
    virtual void cook_info(tdrv_texture_info *info) override;
    virtual void SetLightMapMode(DWORD dwFlag) override;
    virtual void SetTextureLevel(int n) override;
    virtual int EnableMTMode(DWORD dwMTOn) override;
    virtual void TurnOffTexuring(bool bTexOff) override;
};

void calc_size(tdrv_texture_info *info, d3d_cookie cookie); 
void GetAvailableTexMem(unsigned int *local, unsigned int *agp); 
void blit_8to16(tdrv_texture_info *info, uint16 *dst, int drow, uint16 *pal); 
void blit_8to16_trans(tdrv_texture_info *info, uint16 *dst, int drow, uint16 *pal); 
void blit_8to16_clut(tdrv_texture_info *info, uint16 *dst, int drow, uint16 *pal); 
void blit_8to16_scale(tdrv_texture_info *info, uint16 *dst, int drow, uint16 *pal); 
void blit_8to16_clut_scale(tdrv_texture_info *info, uint16 *dst, int drow, uint16 *pal); 
void blit_8to8(tdrv_texture_info *info, unsigned __int8 *dst, int drow); 
void blit_8to8_clut(tdrv_texture_info *info, unsigned __int8 *dst, int drow); 
void blit_8to8_scale(tdrv_texture_info *info, unsigned __int8 *dst, int drow); 
void blit_8to8_clut_scale(tdrv_texture_info *info, unsigned __int8 *dst, int drow); 
void blit_16to16(tdrv_texture_info *info, uint16 *dst, int drow); 
void blit_16to16_scale(tdrv_texture_info *info, uint16 *dst, int drow); 
void blit_32to32(tdrv_texture_info *info, uint16 *dst, int drow); 
void blit_32to32_scale(tdrv_texture_info *info, uint16 *dst, int drow); 
void blit_32to16(tdrv_texture_info *info, uint16 *dst, unsigned int drow);
int EnumTextureFormatsCallback(_DDPIXELFORMAT *lpDDPixFmt, void *lpContext); 
void CheckSurfaces(sWinDispDevCallbackInfo *info); 
void InitDefaultTexture(int size); 
int FindClosestColor(float r, float g, float b); 
int init_size_tables(int **p_size_list); 
void init_size_table(int *size_table, unsigned __int8 type); 
int munge_size_tables(int **p_size_list); 
int get_num_sizes(int *size_list, int num_sizes, int *size_table); 
