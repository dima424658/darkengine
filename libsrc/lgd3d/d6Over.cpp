#include <d3dtypes.h>

#include <d6Over.h>
#include <types.h>
#include <grspoint.h>

struct sOverRectangle
{
    float fX0, fU0, fY0, fV0;
    float fX1, fU1, fY1, fV1;
};

struct sLGD3DOverlayInfo
{
    DWORD dwFlags;
    grs_bitmap *pBitmap;
    float fX0, fY0;
    float fX1, fY1;
    float tu0, tv0;
    float tu1, tv1;
    int nAlpha, nRed, nGreen, nBlue;
};

struct sBaseOverData
{
    float fX0, fY0;
    float fX1, fY1;
    float fU0, fV0;
    float fU1, fV1;
    DWORD dwColor;
    bool bReady;
};

class cD6AlphaOverlay : public cD6OvelayType
{
    enum
    {
        kBitmapUnlocked = 0xFF
    };

public:
    cD6AlphaOverlay(const cD6AlphaOverlay &);
    cD6AlphaOverlay();
    virtual ~cD6AlphaOverlay();
    void SetPositionF(float fX, float fY);
    void Move(float fDeltaX, float fDeltaY);
    void SetAlpha(int nAlpha);
    void SetColor(int nRed, int nGreen, int nBlue, int nAlpha);
    void SetOverlayBaseData(sLGD3DOverlayInfo *psOverInfo);
    void UpdateOverlayBaseData(sLGD3DOverlayInfo *psOverInfo);
    virtual void DrawOverlay() override;
    long PrepareOverlay();
    long LockAndGetBitmap(grs_bitmap **ppsBitmap);
    long UnLockAndReload();

private:
    void SetBitmap(grs_bitmap *psBitmap);

private:
    int m_bAlphaBlend;
    int m_bTextureLoaded;
    int m_nTexID;
    grs_bitmap *m_psBitmap;

    sBaseOverData m_sOverData;
    D3DTLVERTEX m_saVertices[4];
};

class cD6OverlayHandler
{
public:
    cD6OverlayHandler(const cD6OverlayHandler &);
    cD6OverlayHandler();
    ~cD6OverlayHandler();

    void AddOverlay(cD6OvelayType *pcNewOver, unsigned long *phOver);
    void InsertOverlayAfter(cD6OvelayType *pcNewOver, cD6OvelayType *pcAfterOver, unsigned long *phOver);
    void RemoveOverlay(cD6OvelayType *pcOver);
    void RemoveAllOverlays();
    void KillBranch(cD6OvelayType *pcOver);
    void SetClipViewport(sOverRectangle *psInRect);
    void GetClipViewport(sOverRectangle *psInRect);
    void DrawOverlays();
    int OverlayFromHandle(unsigned long hOver, cD6OvelayType **ppcAlphaOver);
    int AlphaOverlayFromHandle(unsigned long hOver, cD6AlphaOverlay **ppcAlphaOver);
    unsigned long MakeNewOverlayHandle(cD6OvelayType *pcOver);

private:
    cD6OvelayType m_cListHead;
};

int lgd3d_aol_add(sLGD3DOverlayInfo *psOverInfo, unsigned int *phOver);
int lgd3d_aol_insert(sLGD3DOverlayInfo *psOverInfo, unsigned int hAfterMe, unsigned int *phOver);
int lgd3d_aol_remove(unsigned int hOver);
void lgd3d_aol_remove_all();
int lgd3d_aol_switch(unsigned int hOver, int bOn);
int lgd3d_aol_update(unsigned int hOver, sLGD3DOverlayInfo *psOverInfo);
int lgd3d_aol_lock_bitmap_data(unsigned int hOver, grs_bitmap **ppsBitmap);
int lgd3d_aol_unlock_bitmap_data(unsigned int hOver);
int lgd3d_aol_set_position(unsigned int hOver, float fScreenX, float fScreenY);
int lgd3d_aol_move(unsigned int hOver, float fDeltaX, float fDeltaY);
int lgd3d_aol_set_color(unsigned int hOver, unsigned int nRed, unsigned int nGreen, unsigned int nBlue, unsigned int nAlpha);
int lgd3d_aol_set_alpha(unsigned int hOver, unsigned int nAlpha);
void lgd3d_aol_set_clip_rect(sOverRectangle *psInRect);
void lgd3d_aol_get_clip_rect(sOverRectangle *psOutRect);
