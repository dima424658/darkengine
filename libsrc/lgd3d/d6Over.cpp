#include <d3dtypes.h>

#include <d6Over.h>
#include <grspoint.h>
#include <types.h>

#define SOME_ERROR_1 MAKE_HRESULT(1, 0x4, 0x303)
#define SOME_ERROR_2 MAKE_HRESULT(1, 0x4, 0x304)
#define SOME_ERROR_3 MAKE_HRESULT(1, 0x4, 0x305)

cD6OverlayHandler *pcOverlayHandler = nullptr;

class cD6AlphaOverlay : public cD6OvelayType {
  enum { kBitmapUnlocked = 0xFF };

public:
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
  int m_nTexID; // FIXME: massize_t
  grs_bitmap *m_psBitmap;

  sBaseOverData m_sOverData;
  D3DTLVERTEX m_saVertices[4];
};

int lgd3d_aol_add(sLGD3DOverlayInfo *psOverInfo, unsigned int *phOver);
int lgd3d_aol_insert(sLGD3DOverlayInfo *psOverInfo, unsigned int hAfterMe,
                     unsigned int *phOver);
int lgd3d_aol_remove(unsigned int hOver);
void lgd3d_aol_remove_all();
int lgd3d_aol_switch(unsigned int hOver, int bOn);
int lgd3d_aol_update(unsigned int hOver, sLGD3DOverlayInfo *psOverInfo);
int lgd3d_aol_lock_bitmap_data(unsigned int hOver, grs_bitmap **ppsBitmap);
int lgd3d_aol_unlock_bitmap_data(unsigned int hOver);
int lgd3d_aol_set_position(unsigned int hOver, float fScreenX, float fScreenY);
int lgd3d_aol_move(unsigned int hOver, float fDeltaX, float fDeltaY);
int lgd3d_aol_set_color(unsigned int hOver, unsigned int nRed,
                        unsigned int nGreen, unsigned int nBlue,
                        unsigned int nAlpha);
int lgd3d_aol_set_alpha(unsigned int hOver, unsigned int nAlpha);
void lgd3d_aol_set_clip_rect(sOverRectangle *psInRect);
void lgd3d_aol_get_clip_rect(sOverRectangle *psOutRect);

void cD6OverlayHandler::cD6OverlayHandler() {
  m_viewRect = {};
  m_viewRect.fX0 = 0.0;
  m_viewRect.fY0 = 0.0;
  m_viewRect.fX1 = g_dwScreenWidth;
  m_viewRect.fY1 = g_dwScreenHeight;

  m_cListHead = {};
  m_cListHead.m_pcParent = nullptr;
  m_cListHead.m_pcNext = nullptr;
  m_cListHead.m_pcPrevious = nullptr;
}

void cD6OverlayHandler::~cD6OverlayHandler() { RemoveAllOverlays(); }

void cD6OverlayHandler::AddOverlay(cD6OvelayType *pcNewOver, DWORD *phOver) {
  for (cD6OverlayHandler *pcOverCurr = this; pcOverCurr->m_cListHead.m_pcNext;
       pcOverCurr = pcOverCurr->m_cListHead.m_pcNext->m_pcParent) {
  }

  pcOverCurr->m_cListHead.m_pcNext = pcNewOver;
  pcNewOver->m_pcNext = nullptr;
  pcNewOver->m_pcPrevious = &pcOverCurr->m_cListHead;
  pcNewOver->m_pcParent = this;
  *phOver = MakeNewOverlayHandle(pcNewOver);
}

void cD6OverlayHandler::InsertOverlayAfter(cD6OvelayType *pcNewOver,
                                           cD6OvelayType *pcAfterOver,
                                           DWORD *phOver) {
  Assert_(pcNewOver != nullptr);
  Assert_(pcAfterOver != nullptr);

  pcNewOver->m_pcNext = pcAfterOver->m_pcNext;
  if (pcNewOver->m_pcNext)
    pcNewOver->m_pcNext->m_pcPrevious = pcNewOver;

  pcAfterOver->m_pcNext = pcNewOver;
  pcNewOver->m_pcPrevious = pcAfterOver;
  pcNewOver->m_pcParent = this;

  *phOver = MakeNewOverlayHandle(pcNewOver);
}

void cD6OverlayHandler::RemoveOverlay(cD6OvelayType *pcOver) {
  Assert_(pcOver->m_pcPrevious != nullptr);

  pcOver->m_pcPrevious->m_pcNext = pcOver->m_pcNext;
  if (pcOver->m_pcNext)
    pcOver->m_pcNext->m_pcPrevious = pcOver->m_pcPrevious;

  delete pcOver;
}

void cD6OverlayHandler::RemoveAllOverlays() {
  auto *pcOverCurr = m_cListHead.m_pcNext;
  if (pcOverCurr)
    KillBranch(pcOverCurr);
}

void cD6OverlayHandler::KillBranch(cD6OvelayType *pcOver) {
  if (pcOver->m_pcNext)
    KillBranch(pcOver->m_pcNext);

  delete pcOver;
}

void cD6OverlayHandler::SetClipViewport(sOverRectangle *psInRect) {
  if (psInRect) {
    Assert_(psInRect->fX0 < psInRect->fX1);
    Assert_(psInRect->fY0 < psInRect->fY1);

    m_viewRect = *psInRect;
  } else {
    m_viewRect.fX0 = 0.0;
    m_viewRect.fY0 = 0.0;
    m_viewRect.fX1 = g_dwScreenWidth;
    m_viewRect.fY1 = g_dwScreenHeight;
  }
}

void cD6OverlayHandler::GetClipViewport(sOverRectangle *psOutRect) {
  Assert(psOutRect != nullptr);

  *psOutRect = m_viewRect;
}

void cD6OverlayHandler::DrawOverlays() {
  auto *pcOverIterator = m_cListHead.m_pcNext;
  if (!pcOverIterator)
    return;

  auto bZwrite = lgd3d_is_zwrite_on();
  auto bZcompare = lgd3d_is_zcompare_on();

  lgd3d_blend_normal();
  if (bZwrite)
    lgd3d_set_zwrite(FALSE);
  if (bZcompare)
    lgd3d_set_zcompare(FALSE);

  while (pcOverIterator) {
    pcOverIterator->DrawOverlay();
    pcOverIterator = pcOverIterator->m_pcNext;
  }

  if (bZwrite)
    lgd3d_set_zwrite(TRUE);
  if (bZcompare)
    lgd3d_set_zcompare(TRUE);
  lgd3d_set_blend(FALSE);
}

BOOL cD6OverlayHandler::OverlayFromHandle(DWORD hOver,
                                          cD6OvelayType **ppcOver) {
  auto *pcOverIterator = m_cListHead.m_pcNext;
  *ppcOver = nullptr;
  while (pcOverIterator) {
    if (pcOverIterator->m_dwID == hOver) {
      *ppcOver = pcOverIterator;
      break;
    }
    pcOverIterator = pcOverIterator->m_pcNext;
  }

  AssertMsg1(*ppcOver != nullptr, "invalid overlay handle : %d", hOver);

  return *ppcOver != nullptr;
}

BOOL cD6OverlayHandler::AlphaOverlayFromHandle(DWORD hOver,
                                               cD6OvelayType **ppcAlphaOver) {
  cD6OvelayType *pcTempType = nullptr;
  *ppcAlphaOver = nullptr;
  if (OverlayFromHandle(pcOverlayHandler, hOver, &pcTempType))
    *ppcAlphaOver = pcTempType;

  return *ppcAlphaOver != nullptr;
}

DWORD cD6OverlayHandler::MakeNewOverlayHandle(cD6OvelayType *pcOver) {
  pcOver->m_dwID = ++s_dwIdGenerator;
  return pcOver->m_dwID;
}

cD6AlphaOverlay::cD6AlphaOverlay() {
  m_bAlphaBlend = FALSE;
  m_bTextureLoaded = FALSE;
  m_nTexID = -1;
  m_psBitmap = nullptr;
}

cD6AlphaOverlay::~cD6AlphaOverlay() {
  if (m_bTextureLoaded && g_tmgr)
    g_tmgr->unload_texture(m_psBitmap);
}

void cD6AlphaOverlay::SetOverlayBaseData(sLGD3DOverlayInfo *psOverInfo) {
  m_sOverData.fX0 = psOverInfo->fX0;
  m_sOverData.fY0 = psOverInfo->fY0;
  m_psBitmap = psOverInfo->pBitmap;

  auto dwFlags = psOverInfo->dwFlags;
  if (dwFlags & 0x10) {
    m_sOverData.fX1 = psOverInfo->fX1;
    m_sOverData.fY1 = psOverInfo->fY1;
  } else {
    auto fWidth = 1 << m_psBitmap->wlog;
    auto fHeight = 1 << m_psBitmap->hlog;
    m_sOverData.fX1 = fWidth + psOverInfo->fX0;
    m_sOverData.fY1 = fHeight + psOverInfo->fY0;
  }

  if (dwFlags & 0x20) {
    m_sOverData.fU0 = psOverInfo->tu0;
    m_sOverData.fU1 = psOverInfo->tu1;
    m_sOverData.fV0 = psOverInfo->tv0;
    m_sOverData.fV1 = psOverInfo->tv1;
  } else {
    m_sOverData.fU0 = 0.0;
    m_sOverData.fV0 = 0.0;
    m_sOverData.fU1 = 1.0;
    m_sOverData.fV1 = 1.0;
  }

  int nAlpha = 0;
  if (dwFlags & 0x04)
    nAlpha = psOverInfo->nAlpha;
  else
    nAlpha = 255;
  m_bAlphaBlend = nAlpha < 255;

  DWORD dwColor = 0;
  if (dwFlags & 0x08)
    dwColor = psOverInfo->nBlue | (psOverInfo->nGreen << 8) |
              (psOverInfo->nRed << 16) | (nAlpha << 24);
  else
    dwColor = 0x00FFFFFF | (nAlpha << 24);
  m_sOverData.dwColor = dwColor;
}

void cD6AlphaOverlay::UpdateOverlayBaseData(sLGD3DOverlayInfo *psOverInfo) {
  auto dwFlags = psOverInfo->dwFlags;
  if (dwFlags & 0x01) {
    m_sOverData.fX0 = psOverInfo->fX0;
    m_sOverData.fY0 = psOverInfo->fY0;
  }
  if (dwFlags & 0x10) {
    m_sOverData.fX1 = psOverInfo->fX1;
    m_sOverData.fY1 = psOverInfo->fY1;
  }
  if (dwFlags & 0x20) {
    m_sOverData.fU0 = psOverInfo->tu0;
    m_sOverData.fU1 = psOverInfo->tu1;
    m_sOverData.fV0 = psOverInfo->tv0;
    m_sOverData.fV1 = psOverInfo->tv1;
  }

  if (!(dwFlags & 0x0C))
    return;

  DWORD nAlpha = 0;
  if (dwFlags & 0x04)
    nAlpha = psOverInfo->nAlpha >> 24;
  else
    nAlpha = m_sOverData.dwColor & 0xFF000000;

  if (dwFlags & 0x08)
    m_sOverData.dwColor = psOverInfo->nBlue | (psOverInfo->nGreen << 8) |
                          (psOverInfo->nRed << 16);
  else
    m_sOverData.dwColor &= 0x00FFFFFF;

  m_sOverData.dwColor |= nAlpha;
  m_bAlphaBlend = nAlpha < 0xFF00'0000;
}

int cD6AlphaOverlay::PrepareOverlay() {
  auto fX0 = g_XOffset + m_sOverData.fX0;
  auto fY0 = g_YOffset + m_sOverData.fY0;
  auto fX1 = g_XOffset + m_sOverData.fX1;
  auto fY1 = g_YOffset + m_sOverData.fY1;
  auto fU0 = m_sOverData.fU0;
  auto fV0 = m_sOverData.fV0;
  auto fU1 = m_sOverData.fU1;
  auto fV1 = m_sOverData.fV1;

  m_bVisible = FALSE;

  m_pcParent->GetClipViewport(&sClipView);
  if (fX1 < sClipView.fX0 || fX0 > sClipView.fX1 || fY1 < sClipView.fY0 ||
      fY0 > sClipView.fY1) {
    return 262928;
  }

  auto dwClipFlags = 0;
  if (fX0 < sClipView.fX0)
    dwClipFlags |= 1;
  if (fY0 < sClipView.fY0)
    dwClipFlags |= 2;
  if (fX1 > sClipView.fX1)
    dwClipFlags |= 4;
  if (fY1 > sClipView.fY1)
    dwClipFlags |= 8;

  if (dwClipFlags) {
    fHTexScale = (fU1 - fU0) / (fX1 - fX0);
    fVTexScale = (fV1 - fV0) / (fY1 - fY0);
    if (dwClipFlags & 1) {
      fD = sClipView.fX0 - fX0;
      fX0 = sClipView.fX0;
      fU0 = fD * fHTexScale + fU0;
    }
    if (dwClipFlags & 2) {
      fDb = sClipView.fY0 - fY0;
      fY0 = sClipView.fY0;
      fV0 = fDb * fVTexScale + fV0;
    }
    if (dwClipFlags & 4) {
      fDa = fX1 - sClipView.fX1;
      fX1 = sClipView.fX1;
      fU1 = fU1 - fDa * fHTexScale;
    }
    if (dwClipFlags & 8) {
      fDc = fY1 - sClipView.fY1;
      fY1 = sClipView.fY1;
      fV1 = fV1 - fDc * fVTexScale;
    }
  }

  m_saVertices[0].sx = fX0;
  m_saVertices[0].sy = fY0;
  m_saVertices[0].sz = 0.01;
  m_saVertices[0].rhw = 100.0;
  m_saVertices[0].color = m_sOverData.dwColor;
  m_saVertices[0].specular = 0xFF00'0000;
  m_saVertices[0].tu = fU0;
  m_saVertices[0].tv = fV0;

  m_saVertices[1].sx = fX1;
  m_saVertices[1].sy = fY0;
  m_saVertices[1].sz = 0.01;
  m_saVertices[1].rhw = 100.0;
  m_saVertices[1].color = m_sOverData.dwColor;
  m_saVertices[1].specular = 0xFF00'0000;
  m_saVertices[1].tu = fU1;
  m_saVertices[1].tv = fV0;

  m_saVertices[2].sx = fX1;
  m_saVertices[2].sy = fY1;
  m_saVertices[2].sz = 0.01;
  m_saVertices[2].rhw = 100.0;
  m_saVertices[2].color = m_sOverData.dwColor;
  m_saVertices[2].specular = 0xFF00'0000;
  m_saVertices[2].tu = fU1;
  m_saVertices[2].tv = fV1;

  m_saVertices[3].sx = fX0;
  m_saVertices[3].sy = fY1;
  m_saVertices[3].sz = 0.01;
  m_saVertices[3].rhw = 100.0;
  m_saVertices[3].color = m_sOverData.dwColor;
  m_saVertices[3].specular = 0xFF00'0000;
  m_saVertices[3].tu = fU0;
  m_saVertices[3].tv = fV1;

  m_bVisible = TRUE;

  return 0;
}

int cD6AlphaOverlay::LockAndGetBitmap(grs_bitmap **ppsBitmap) {
  if (m_nTexID != -1)
    return SOME_ERROR_1;

  if (!m_bTextureLoaded)
    return SOME_ERROR_3;

  m_nTexID = (int)m_psBitmap->bits;
  g_tmgr->restore_bits(m_psBitmap);
  *ppsBitmap = m_psBitmap;
  return 0;
}

int cD6AlphaOverlay::UnLockAndReload() {
  if (m_nTexID == -1)
    return SOME_ERROR_2;

  if (!m_bTextureLoaded)
    return SOME_ERROR_3;

  m_psBitmap->bits = (uchar *)m_nTexID;
  g_tmgr->reload_texture(m_psBitmap);
  m_nTexID = -1;

  return 0;
}

void cD6AlphaOverlay::DrawOverlay() {
  if (!m_bVisible || m_bTurnedOff)
    return;

  Assert(m_nTexID != -1, "Draw(alpha)Overlay: the bitmap is locked!\n");

  if (m_bAlphaBlend)
    lgd3d_set_blend(TRUE);

  lgd3d_set_texture_level(FALSE);
  if (g_tmgr)
    g_tmgr->set_texture(m_psBitmap);

  m_bTextureLoaded = TRUE;
  if (pcStates->EnableMTMode(0))
    g_lpD3Ddevice->DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_TLVERTEX,
                                 m_saVertices, std::size(m_saVertices),
                                 D3DDP_DONOTCLIP | D3DDP_DONOTUPDATEEXTENTS);
}

void cD6AlphaOverlay::SetPositionF(float fX, float fY) {
  auto fXoffest = m_sOverData.fX1 - m_sOverData.fX0;
  auto fYoffset = m_sOverData.fY1 - m_sOverData.fY0;

  m_sOverData.fX0 = fX;
  m_sOverData.fX1 = fX + fXoffest;
  m_sOverData.fY0 = fY;
  m_sOverData.fY1 = fY + fYoffset;
}

void cD6AlphaOverlay::SetColor(int nRed, int nGreen, int nBlue, int nAlpha) {
  m_sOverData.dwColor = nBlue | (nGreen << 8) | (nRed << 16) | (nAlpha << 24);
  m_bAlphaBlend = nAlpha < 255;
}

void cD6AlphaOverlay::Move(float fDeltaX, float fDeltaY) {
  m_sOverData.fX0 += fDeltaX;
  m_sOverData.fX1 += fDeltaX;
  m_sOverData.fY0 += fDeltaY;
  m_sOverData.fY1 += fDeltaY;
}

void cD6AlphaOverlay::SetAlpha(int nAlpha) {
  m_sOverData.dwColor = (nAlpha << 24) | 0x00FFFFFF & m_sOverData.dwColor;
  m_bAlphaBlend = nAlpha < 255;
}
