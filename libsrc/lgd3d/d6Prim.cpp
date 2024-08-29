#include <lgassert.h>
#include "d6Prim.h"
#include "d6States.h"

typedef struct {
	double left;
	double top;
	double right;
	double bot;
} cliprect;

cliprect lgd3d_clip;
uint16 waEdgeIndices[50];

extern float g_XOffset;
extern float g_YOffset;

extern "C" {
	extern BOOL zlinear;
}

uint16 hack_light_alpha_pal[16] =
{
  4095u,
  8191u,
  12287u,
  16383u,
  20479u,
  24575u,
  28671u,
  32767u,
  36863u,
  40959u,
  45055u,
  49151u,
  53247u,
  57343u,
  61439u,
  65535u
}; // idb

double z_near = 1.0; // idb
double z_far = 200.0; // idb
double inv_z_far = 0.005; // idb
double z1 = 1.005025125628141; // idb
double z2 = 1.005025125628141; // idb
double z2d = 1.0; // idb
double w2d = 1.0; // idb

cD6Primitives::cD6Primitives()
{
	m_bFlushingOn = 0;
	m_bPointMode = 0;
	m_ePolyMode = kLgd3dPolyModeFillWTexture;
	m_dwNoCashedPoints = 0;
	m_dwPointBufferSize = 50;
	m_dwNoCashedVertices = 0;
	m_dwVertexBufferSize = 50;
	m_pVIB = 0;
	m_hack_light_bm = 0;

	memset(m_saVertexBuffer, 0, sizeof(m_saVertexBuffer));

	for (int i = 0; i < 50; ++i)
		waEdgeIndices[i] = i;

	CreateVertIndBuffer(16);

	lgd3d_release_ip_func = lgd3d_release_indexed_primitives;
}

cD6Primitives::~cD6Primitives()
{
	if (m_hack_light_bm)
	{
		if (g_tmgr)
			g_tmgr->unload_texture(m_hack_light_bm);

		gr_free(m_hack_light_bm);
		m_hack_light_bm = 0;
	}

	DeleteVertIndBuffer();

	lgd3d_release_ip_func = 0;
}

cD6Primitives* cD6Primitives::DeInstance()
{
	return nullptr;
}

D3DTLVERTEX* cD6Primitives::ReservePolySlots(unsigned int n)
{
	if (m_bPointMode)
	{
		FlushPoints();
		m_bPointMode = 0;
	}

	if (n > m_dwVertexBufferSize)
		CriticalMsg("ReservePolySlots(): poly too large!");

	m_dwNoCashedVertices = n;

	return m_saVertexBuffer;
}

void cD6Primitives::DrawPoly(BOOL bSuspendTexturing)
{
	if ((m_ePolyMode & kLgd3dPolyModeFillWColor) != 0)
		bSuspendTexturing = 1;

	if (bSuspendTexturing != g_bTexSuspended)
	{
		FlushIndPolies();

		if (bSuspendTexturing)
			StartNonTexMode();
		else
			EndNonTexMode();
	}

	if (!lgd3d_punt_d3d)
	{
		if ((m_ePolyMode & 3) != 0)
		{
			HRESULT hResult = g_lpD3Ddevice->DrawPrimitive(
				D3DPT_TRIANGLEFAN,
				452,
				m_saVertexBuffer,
				m_dwNoCashedVertices,
				12);

			if (FAILED(hResult))
			{
				const char* DDErrorMsg = GetDDErrorMsg(hResult);
				const char* msg = _LogFmt("%s: error %d\n%s ", "DrawPrimitive failed", hResult, DDErrorMsg);
				CriticalMsg(msg);
			}
		}

		if ((m_ePolyMode & kLgd3dPolyModeDrawEdges) != 0)
			DrawStandardEdges(m_saVertexBuffer, m_dwNoCashedVertices);
	}
}

BOOL cD6Primitives::Poly(int n, r3s_point** ppl)
{
	_r3s_point* _src; // [esp+8h] [ebp-1Ch]
	D3DTLVERTEX* _dest; // [esp+Ch] [ebp-18h]
	int _sy; // [esp+10h] [ebp-14h]
	int _sx; // [esp+14h] [ebp-10h]
	int c0; // [esp+20h] [ebp-4h]

	c0 = pcStates->get_color();
	D3DTLVERTEX* vlist = ReservePolySlots(n);
	for (int j = 0; j < n; ++j)
	{
		vlist[j].color = c0;
		vlist[j].specular = m_dcFogSpecular;
		_dest = &vlist[j];
		_src = ppl[j];
		_sx = _src->grp.sx + 0x8000;
		_sy = _src->grp.sy + 0x8000;
		if (_sx > grd_canvas->gc.clip.f.right)
			_sx = grd_canvas->gc.clip.f.right;
		if (_sx < grd_canvas->gc.clip.f.left)
			_sx = grd_canvas->gc.clip.f.left;
		if (_sy > grd_canvas->gc.clip.f.bot)
			_sy = grd_canvas->gc.clip.f.bot;
		if (_sy < grd_canvas->gc.clip.f.top)
			_sy = grd_canvas->gc.clip.f.top;
		_dest->sx = (double)_sx / 65536.0 + g_XOffset;
		_dest->sy = (double)_sy / 65536.0 + g_YOffset;
		if (zlinear)
		{
			_dest->sz = z2d;
		}
		else if (lgd3d_z_normal)
		{
			_dest->sz = _src->p.z * inv_z_far;
		}
		else
		{
			_dest->sz = z1 - _src->grp.w * z2;
			_dest->rhw = _src->grp.w;
			if (_dest->sz <= 1.0)
			{
				if (_dest->sz < 0.0)
					_dest->sz = 0.0;
			}
			else
			{
				_dest->sz = 1.0;
			}
		}
	}

	DrawPoly(TRUE);

	return 0;
}

ePolyMode cD6Primitives::GetPolyMode()
{
	return m_ePolyMode;
}

BOOL cD6Primitives::SetPolyMode(ePolyMode eNewMode)
{
	if ((eNewMode & 7) == 0)
		return FALSE;

	if (m_ePolyMode != eNewMode)
		FlushPrimitives();

	m_ePolyMode = eNewMode;

	return TRUE;
}

void cD6Primitives::DrawStandardEdges(void* pVertera, unsigned int dwNoVeriteces)
{
	if (!g_bTexSuspended)
	{
		FlushIndPolies();
		StartNonTexMode();
	}

	waEdgeIndices[dwNoVeriteces] = 0;

	HRESULT hResult = g_lpD3Ddevice->DrawIndexedPrimitive(
		D3DPT_LINESTRIP,
		452u,
		pVertera,
		dwNoVeriteces,
		waEdgeIndices,
		dwNoVeriteces + 1,
		12u);

	waEdgeIndices[dwNoVeriteces] = dwNoVeriteces;

	if (FAILED(hResult))
	{
		const char* DDErrorMsg = GetDDErrorMsg(hResult);
		const char* msg = _LogFmt("%s: error %d\n%s ", "DrawStandardEdges failed", hResult, DDErrorMsg);
		CriticalMsg(msg);
	}
}

void cD6Primitives::FlushPrimitives()
{
	if (m_bPrimitivesPending)
	{
		if (!m_bFlushingOn)
		{
			m_bFlushingOn = TRUE;

			FlushIndPolies();
			FlushPoints();

			m_bPrimitivesPending = FALSE;
			m_bFlushingOn = FALSE;
		}
	}
}

void cD6Primitives::StartFrame()
{
	lgd3d_clip.left = (double)grd_canvas->gc.clip.f.left / 65536.0 + g_XOffset;
	lgd3d_clip.right = (double)grd_canvas->gc.clip.f.right / 65536.0 + g_XOffset;
	lgd3d_clip.top = (double)grd_canvas->gc.clip.f.top / 65536.0 + g_YOffset;
	lgd3d_clip.bot = (double)grd_canvas->gc.clip.f.bot / 65536.0 + g_YOffset;

	pcStates->EnableFog(FALSE);
	pcStates->SetFogSpecularLevel(0.0f);

	m_dwNoCashedPoints = 0;

	g_bTexSuspended = FALSE;
}

void cD6Primitives::EndFrame()
{
	FlushPrimitives();
	EndIndexedRun();
}

void cD6Primitives::Clear(int c)
{
	int fc_save = grd_canvas->gc.fcolor;

	D3DTLVERTEX* vlist = ReservePolySlots(4);

	grd_canvas->gc.fcolor = c;

	int ca = pcStates->get_color();

	grd_canvas->gc.fcolor = fc_save;

	float x0 = (double)grd_canvas->gc.clip.f.left / 65536.0 + g_XOffset;
	float x1 = (double)grd_canvas->gc.clip.f.right / 65536.0 + g_XOffset;
	float y0 = (double)grd_canvas->gc.clip.f.top / 65536.0 + g_YOffset;
	float y1 = (double)grd_canvas->gc.clip.f.bot / 65536.0 + g_YOffset;

	for (int i = 0; i < 4; ++i)
	{
		float v3 = z2d;
		vlist[i].sz = v3;
		float v2 = w2d;
		vlist[i].rhw = v2;
		vlist[i].color = ca;
		vlist[i].specular = m_dcFogSpecular;
	}

	vlist[0].sx = x0;
	vlist[0].sy = y0;
	vlist[1].sx = x1;
	vlist[1].sy = y0;
	vlist[2].sx = x1;
	vlist[2].sy = y1;
	vlist[3].sx = x0;
	vlist[3].sy = y1;

	DrawPoly(TRUE);
}

void cD6Primitives::StartNonTexMode()
{
	m_iSavedTexId = pcStates->get_texture_id();
	if (m_iSavedTexId != -1)
		pcStates->TurnOffTexuring(TRUE);
}

void cD6Primitives::EndNonTexMode()
{
	if (m_iSavedTexId != -1)
		pcStates->TurnOffTexuring(FALSE);
}

void cD6Primitives::DrawIndPolies()
{
	if (m_dwTempIndCounter <= 2)
	{
		const char* msg = _LogFmt("A poly has more than %d points!", m_dwTempIndCounter);
		CriticalMsg(msg);
	}

	m_waIndices[m_dwNoIndices] = m_waTempIndices[0];
	m_waIndices[m_dwNoIndices + 1] = m_waTempIndices[1];
	m_waIndices[m_dwNoIndices + 2] = m_waTempIndices[2];
	m_dwNoIndices += 3;

	for (int i = 3; i < m_dwTempIndCounter; ++i)
	{
		m_waIndices[m_dwNoIndices] = m_waTempIndices[0];
		m_waIndices[m_dwNoIndices + 1] = *((WORD*)&m_dwNoIndices + i + 1);
		m_waIndices[m_dwNoIndices + 2] = m_waTempIndices[i];
		m_dwNoIndices += 3;
	}

	m_dwTempIndCounter = 0;
	m_bPrimitivesPending = TRUE;
}

D3DTLVERTEX* cD6Primitives::ReservePointSlots(int n)
{
	if (!m_bPointMode)
	{
		FlushIndPolies();
		m_bPointMode = TRUE;
	}

	if (n + m_dwNoCashedPoints > m_dwPointBufferSize)
		FlushPoints();

	if (n + m_dwNoCashedPoints > m_dwPointBufferSize)
		CriticalMsg("ReservePointSlots(): too many points!");

	D3DTLVERTEX* retval = &m_saPointBuffer[m_dwNoCashedPoints];
	m_dwNoCashedPoints += n;
	m_bPrimitivesPending = TRUE;
	return retval;
}

void cD6Primitives::FlushPoints()
{
	if (m_dwNoCashedPoints)
	{
		if (!g_bTexSuspended)
			StartNonTexMode();

		HRESULT hResult = g_lpD3Ddevice->DrawPrimitive(
			D3DPT_POINTLIST,
			452u,
			m_saPointBuffer,
			m_dwNoCashedPoints,
			12u);

		if (FAILED(hResult))
		{
			const char* DDErrorMsg = GetDDErrorMsg(hResult);
			const char* msg = _LogFmt("%s: error %d\n%s ", "DrawPrimitive(points) failed", hResult, DDErrorMsg);
			CriticalMsg(msg);
		}

		m_dwNoCashedPoints = 0;
	}
}

void cD6Primitives::DrawLine(r3s_point* p0, r3s_point* p1)
{
	_r3s_point* v4; // [esp+4h] [ebp-44h]
	_r3s_point* tmp; // [esp+8h] [ebp-40h]
	_D3DTLVERTEX* __dest; // [esp+Ch] [ebp-3Ch]
	int left; // [esp+10h] [ebp-38h]
	int top; // [esp+14h] [ebp-34h]
	float y1; // [esp+18h] [ebp-30h]
	float y0; // [esp+1Ch] [ebp-2Ch]
	_D3DTLVERTEX* vlist; // [esp+20h] [ebp-28h]
	int i; // [esp+24h] [ebp-24h]
	int temp; // [esp+28h] [ebp-20h]
	int tempa; // [esp+28h] [ebp-20h]
	int tempb; // [esp+28h] [ebp-20h]
	int tempc; // [esp+28h] [ebp-20h]
	float x1; // [esp+2Ch] [ebp-1Ch]
	int bot; // [esp+30h] [ebp-18h]
	float x0; // [esp+34h] [ebp-14h]
	int right; // [esp+38h] [ebp-10h]
	int c; // [esp+3Ch] [ebp-Ch]
	int x10; // [esp+40h] [ebp-8h]
	int y10; // [esp+44h] [ebp-4h]

	c = (this->m_nAlpha << 24) | pcStates->get_color();
	left = grd_canvas->gc.clip.f.left;
	right = grd_canvas->gc.clip.f.right;
	top = grd_canvas->gc.clip.f.top;
	bot = grd_canvas->gc.clip.f.bot;
	if (bot - top >= 0x10000 && right - left >= 0x10000)
	{
		vlist = ReservePolySlots(4u);
		for (i = 0; i < 4; ++i)
		{
			vlist[i].color = c;
			vlist[i].specular = this->m_dcFogSpecular;
			__dest = &vlist[i];
			if (zlinear)
			{
				__dest->sz = z2d;
			}
			else if (lgd3d_z_normal)
			{
				__dest->sz = p0->p.z * inv_z_far;
			}
			else
			{
				__dest->sz = z1 - p0->grp.w * z2;
				__dest->rhw = p0->grp.w;
				if (__dest->sz <= 1.0)
				{
					if (__dest->sz < 0.0)
						__dest->sz = 0.0;
				}
				else
				{
					__dest->sz = 1.0;
				}
			}
			vlist[i].tv = 0.0;
			vlist[i].tu = 0.0;
		}
		x10 = p1->grp.sx - p0->grp.sx;
		y10 = p1->grp.sy - p0->grp.sy;
		if (x10 < 0)
			x10 = p0->grp.sx - p1->grp.sx;
		if (y10 < 0)
			y10 = p0->grp.sy - p1->grp.sy;
		if (y10 <= x10)
		{
			top += 0x8000;
			bot -= 0x8000;
			if (p0->grp.sx > p1->grp.sx)
			{
				v4 = p0;
				p0 = p1;
				p1 = v4;
			}
		}
		else
		{
			left += 0x8000;
			right -= 0x8000;
			if (p0->grp.sy > p1->grp.sy)
			{
				tmp = p0;
				p0 = p1;
				p1 = tmp;
			}
		}
		temp = p0->grp.sx + 0x8000;
		if (temp < left)
			temp = left;
		if (temp > right)
			temp = right;
		x0 = (double)temp / 65536.0 + g_XOffset;
		tempa = p1->grp.sx + 0x8000;
		if (tempa < left)
			tempa = left;
		if (tempa > right)
			tempa = right;
		x1 = (double)tempa / 65536.0 + g_XOffset;
		tempb = p0->grp.sy + 0x8000;
		if (tempb < top)
			tempb = top;
		if (tempb > bot)
			tempb = bot;
		y0 = (double)tempb / 65536.0 + g_YOffset;
		tempc = p1->grp.sy + 0x8000;
		if (tempc < top)
			tempc = top;
		if (tempc > bot)
			tempc = bot;
		y1 = (double)tempc / 65536.0 + g_YOffset;
		if (y10 <= x10)
		{
			vlist->sy = y0 + 0.5;
			vlist[1].sy = y0 - 0.5;
			vlist[2].sy = y1 - 0.5;
			vlist[3].sy = y1 + 0.5;
			vlist->sx = x0;
			vlist[1].sx = x0;
			vlist[2].sx = x1;
			vlist[3].sx = x1;
		}
		else
		{
			vlist->sx = x0 - 0.5;
			vlist[1].sx = x0 + 0.5;
			vlist[2].sx = x1 + 0.5;
			vlist[3].sx = x1 - 0.5;
			vlist->sy = y0;
			vlist[1].sy = y0;
			vlist[2].sy = y1;
			vlist[3].sy = y1;
		}

		DrawPoly(TRUE);
	}
}

void cD6Primitives::InitializeVIBCounters()
{
	m_dwNoVIBEntries = 0;
	m_dwNoIndices = 0;
	m_dwTempIndCounter = 0;
	m_dwMinVIndex = 256;
	m_dwMaxVIndex = 0;
}

int cD6Primitives::CreateVertIndBuffer(DWORD dwInitialNoEntries)
{
	DeleteVertIndBuffer();

	m_dwVIBSizeInEntries = dwInitialNoEntries;

	m_pVIB = (D3DTLVERTEX*)malloc(sizeof(D3DTLVERTEX) * m_dwVIBSizeInEntries);
	InitializeVIBCounters();

	m_dwVIBmax = 256;

	return m_pVIB != 0;
}

void cD6Primitives::init_hack_light_bm()
{
	m_hack_light_bm = gr_alloc_bitmap(2u, 2u, 32, 32);
	uchar* bits = m_hack_light_bm->bits;
	for (int i = 0; i < 32; ++i)
	{
		for (int j = 0; j < 32; ++j)
		{
			uchar val = 0;

			float alpha = 128.0 / (((double)i - 15.5) * ((double)i - 15.5) + ((double)j - 15.5) * ((double)j - 15.5)); \
				if (alpha <= 15.0)
					val = (__int64)alpha;
				else
					val = 15;

			bits[j] = val;
		}

		bits += 32;
	}
}

void cD6Primitives::do_quad_light(r3s_point* p, float r, grs_bitmap* bm)
{
	float y_top; // [esp+14h] [ebp-34h]
	float v_bot; // [esp+18h] [ebp-30h]
	_D3DTLVERTEX* vl; // [esp+1Ch] [ebp-2Ch]
	float y; // [esp+20h] [ebp-28h]
	float x; // [esp+24h] [ebp-24h]
	unsigned int fog_specular; // [esp+28h] [ebp-20h]
	float x_left; // [esp+2Ch] [ebp-1Ch]
	float x_right; // [esp+30h] [ebp-18h]
	float y_bot; // [esp+34h] [ebp-14h]
	unsigned int c; // [esp+38h] [ebp-10h]
	float u_left; // [esp+3Ch] [ebp-Ch]
	float u_right; // [esp+40h] [ebp-8h]
	float v_top; // [esp+44h] [ebp-4h]

	x = (double)p->grp.sx / 65536.0 + g_XOffset;
	y = (double)p->grp.sy / 65536.0 + g_YOffset;
	if (x + r >= lgd3d_clip.left && x - r <= lgd3d_clip.right && y + r >= lgd3d_clip.top && y - r <= lgd3d_clip.bot)
	{
		c = pcStates->get_color() | 0xFF000000;
		fog_specular = this->m_dcFogSpecular;
		grd_canvas->gc.fill_type = 3;
		pcStates->SetAlphaPalette(hack_light_alpha_pal);
		if (g_tmgr)
			g_tmgr->set_texture(bm);
		vl = ReservePolySlots(4u);
		x_right = x + r;
		x_left = x - r;
		u_left = 0.0;
		u_right = 1.0;
		if (x_left < (double)lgd3d_clip.left)
		{
			u_left = (lgd3d_clip.left - x_left) / (2.0 * r);
			x_left = lgd3d_clip.left;
		}
		if (x_right > (double)lgd3d_clip.right)
		{
			u_right = (1.0 - u_left) * (lgd3d_clip.right - x_left) / (x_right - x_left) + u_left;
			x_right = lgd3d_clip.right;
		}
		y_bot = y + r;
		y_top = y - r;
		v_top = 0.0;
		v_bot = 1.0;
		if (y_top < (double)lgd3d_clip.top)
		{
			v_top = (lgd3d_clip.top - y_top) / (2.0 * r);
			y_top = lgd3d_clip.top;
		}
		if (y_bot > (double)lgd3d_clip.bot)
		{
			v_bot = v_top + (1.0 - v_top) * (lgd3d_clip.bot - y_top) / (y_bot - y_top);
			y_bot = lgd3d_clip.bot;
		}
		vl->tu = u_left;
		vl->tv = v_top;
		vl->sx = x_left;
		vl->sy = y_top;
		vl->color = c;
		vl->specular = fog_specular;
		if (zlinear)
		{
			vl->sz = z2d;
		}
		else if (lgd3d_z_normal)
		{
			vl->sz = p->p.z * inv_z_far;
		}
		else
		{
			vl->sz = z1 - p->grp.w * z2;
			vl->rhw = p->grp.w;
			if (vl->sz <= 1.0)
			{
				if (vl->sz < 0.0)
					vl->sz = 0.0;
			}
			else
			{
				vl->sz = 1.0;
			}
		}
		vl[1].tu = u_right;
		vl[1].tv = v_top;
		vl[1].sx = x_right;
		vl[1].sy = y_top;
		vl[1].color = c;
		vl[1].specular = fog_specular;
		if (zlinear)
		{
			vl[1].sz = z2d;
		}
		else if (lgd3d_z_normal)
		{
			vl[1].sz = p->p.z * inv_z_far;
		}
		else
		{
			vl[1].sz = z1 - p->grp.w * z2;
			vl[1].rhw = p->grp.w;
			if (vl[1].sz <= 1.0)
			{
				if (vl[1].sz < 0.0)
					vl[1].sz = 0.0;
			}
			else
			{
				vl[1].sz = 1.0;
			}
		}
		vl[2].tu = u_right;
		vl[2].tv = v_bot;
		vl[2].sx = x_right;
		vl[2].sy = y_bot;
		vl[2].color = c;
		vl[2].specular = fog_specular;
		if (zlinear)
		{
			vl[2].sz = z2d;
		}
		else if (lgd3d_z_normal)
		{
			vl[2].sz = p->p.z * inv_z_far;
		}
		else
		{
			vl[2].sz = z1 - p->grp.w * z2;
			vl[2].rhw = p->grp.w;
			if (vl[2].sz <= 1.0)
			{
				if (vl[2].sz < 0.0)
					vl[2].sz = 0.0;
			}
			else
			{
				vl[2].sz = 1.0;
			}
		}
		vl[3].tu = u_left;
		vl[3].tv = v_bot;
		vl[3].sx = x_left;
		vl[3].sy = y_bot;
		vl[3].color = c;
		vl[3].specular = fog_specular;
		if (zlinear)
		{
			vl[3].sz = z2d;
		}
		else if (lgd3d_z_normal)
		{
			vl[3].sz = p->p.z * inv_z_far;
		}
		else
		{
			vl[3].sz = z1 - p->grp.w * z2;
			vl[3].rhw = p->grp.w;
			if (vl[3].sz <= 1.0)
			{
				if (vl[3].sz < 0.0)
					vl[3].sz = 0.0;
			}
			else
			{
				vl[3].sz = 1.0;
			}
		}

		DrawPoly(FALSE);

		grd_canvas->gc.fill_type = 0;
	}
}

void cD6Primitives::HackLight(r3s_point* p, float r)
{
	if (r > 1.0)
	{
		if (!m_hack_light_bm)
			init_hack_light_bm();

		do_quad_light(p, r, m_hack_light_bm);
	}
	else
	{
		DrawPoint(p);
	}
}

cMSBuffer::cMSBuffer()
{
	m_dwMaxNoMTVertices = 50;
}

cMSBuffer::~cMSBuffer()
{
}

cD6Primitives* cMSBuffer::Instance()
{
	if (!m_Instance)
	{
		m_Instance = new cMSBuffer();
	}

	return m_Instance;
}

void cMSBuffer::DrawPoly(BOOL bSuspendTexturing)
{
	if ((m_ePolyMode & 2) != 0)
		bSuspendTexturing = 1;

	if (bSuspendTexturing != g_bTexSuspended)
	{
		FlushIndPolies();

		if (bSuspendTexturing)
			StartNonTexMode();
		else
			EndNonTexMode();
	}

	if ((g_bTexSuspended || pcStates->EnableMTMode(0)) && !lgd3d_punt_d3d)
	{
		if ((m_ePolyMode & 3) != 0)
		{
			HRESULT hResult = g_lpD3Ddevice->DrawPrimitive(
				D3DPT_TRIANGLEFAN,
				452u,
				m_saVertexBuffer,
				m_dwNoCashedVertices,
				12u);

			if (FAILED(hResult))
			{
				const char* DDErrorMsg = GetDDErrorMsg(hResult);
				const char* msg = _LogFmt("%s: error %d\n%s ", "DrawPrimitive(poly) failed", hResult, DDErrorMsg);
				CriticalMsg(msg);
			}
		}

		if ((m_ePolyMode & kLgd3dPolyModeDrawEdges) != 0)
			DrawStandardEdges(m_saVertexBuffer, m_dwNoCashedVertices);
	}
}

MTVERTEX* cMSBuffer::ReserveMTPolySlots(int n)
{
	if (n > m_dwMaxNoMTVertices)
		CriticalMsg("ReservePolySlots(): poly too large!");

	m_dwNoMTVertices = n;

	return m_saMTVertices;
}
