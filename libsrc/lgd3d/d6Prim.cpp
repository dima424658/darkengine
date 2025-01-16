#define D3D_OVERLOADS
#include <lgassert.h>
#include "d6Prim.h"
#include "d6States.h"

typedef struct
{
	double left;
	double top;
	double right;
	double bot;
} cliprect;

cliprect lgd3d_clip;
uint16 waEdgeIndices[50];

extern float g_XOffset;
extern float g_YOffset;

extern double z_near, z_far, inv_z_far;
extern double z1, z2;

extern "C"
{
	extern BOOL zlinear;
	extern double z2d, w2d;
}

uint16 hack_light_alpha_pal[] =
{
	0x0FFF,
	0x1FFF,
	0x2FFF,
	0x3FFF,
	0x4FFF,
	0x5FFF,
	0x6FFF,
	0x7FFF,
	0x8FFF,
	0x9FFF,
	0xAFFF,
	0xBFFF,
	0xCFFF,
	0xDFFF,
	0xEFFF,
	0xFFFF
};

cD6Primitives::cD6Primitives()
	: m_bFlushingOn{FALSE},
	  m_bPointMode{FALSE},
	  m_ePolyMode{ePolyMode::kLgd3dPolyModeFillWTexture},
	  m_dwNoCashedPoints{0},
	  m_dwPointBufferSize{std::size(m_saPointBuffer)},
	  m_dwNoCashedVertices{0},
	  m_dwVertexBufferSize{std::size(m_saVertexBuffer)},
	  m_pVIB{nullptr},
	  m_hack_light_bm{nullptr},
	  m_saVertexBuffer{}
{
	for (int i = 0; i < std::size(waEdgeIndices); ++i)
		waEdgeIndices[i] = i;

	CreateVertIndBuffer(16);

	lgd3d_release_ip_func = lgd3d_release_indexed_primitives;
}

cD6Primitives::~cD6Primitives()
{
	if (m_hack_light_bm)
	{
		lgd3d_unload_texture(m_hack_light_bm);
		gr_free(m_hack_light_bm);
		m_hack_light_bm = nullptr;
	}

	DeleteVertIndBuffer();

	lgd3d_release_ip_func = nullptr;
}

cD6Primitives *cD6Primitives::DeInstance()
{
	return nullptr;
}

D3DTLVERTEX *cD6Primitives::ReservePolySlots(unsigned int n)
{
	if (m_bPointMode)
	{
		FlushPoints();
		m_bPointMode = FALSE;
	}

	AssertMsg(n <= m_dwVertexBufferSize, "ReservePolySlots(): poly too large!");
	m_dwNoCashedVertices = n;

	return m_saVertexBuffer;
}

void cD6Primitives::DrawPoly(BOOL bSuspendTexturing)
{
	if (m_ePolyMode & ePolyMode::kLgd3dPolyModeFillWColor)
		bSuspendTexturing = TRUE;

	if (bSuspendTexturing != g_bTexSuspended)
	{
		FlushIndPolies();

		if (bSuspendTexturing)
			StartNonTexMode();
		else
			EndNonTexMode();
	}

	if (lgd3d_punt_d3d)
		return;

	if (m_ePolyMode & (ePolyMode::kLgd3dPolyModeFillWTexture || ePolyMode::kLgd3dPolyModeFillWColor))
	{
		auto hResult = g_lpD3Ddevice->DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_TLVERTEX, m_saVertexBuffer, m_dwNoCashedVertices, D3DDP_DONOTCLIP | D3DDP_DONOTUPDATEEXTENTS);
		AssertMsg3(SUCCEEDED(hResult), "%s: error %d\n%s ", "DrawPrimitive failed", hResult, GetDDErrorMsg(hResult));
	}

	if (m_ePolyMode & ePolyMode::kLgd3dPolyModeDrawEdges)
		DrawStandardEdges(m_saVertexBuffer, m_dwNoCashedVertices);
}

BOOL cD6Primitives::Poly(int n, r3s_point **ppl)
{
	auto c0 = pcStates->get_color();
	auto *vlist = ReservePolySlots(n);

	for (int j = 0; j < n; ++j)
	{
		vlist[j].color = c0;
		vlist[j].specular = m_dcFogSpecular;

		auto _sx = ppl[j][0].grp.sx + fix_from_float(0.5);
		auto _sy = ppl[j][0].grp.sy + fix_from_float(0.5);
		_sx = std::clamp(_sx, grd_canvas->gc.clip.f.left, grd_canvas->gc.clip.f.right);
		_sy = std::clamp(_sy, grd_canvas->gc.clip.f.top, grd_canvas->gc.clip.f.bot);
		vlist[j].sx = fix_float(_sx) + g_XOffset;
		vlist[j].sy = fix_float(_sy) + g_YOffset;

		if (zlinear)
		{
			vlist[j].sz = z2d;
		}
		else if (lgd3d_z_normal)
		{
			vlist[j].sz = ppl[j][0].p.z * inv_z_far;
		}
		else
		{
			vlist[j].sz = z1 - ppl[j][0].grp.w * z2;
			vlist[j].rhw = ppl[j][0].grp.w;
			vlist[j].sz = std::clamp(vlist[j].sz, 0.0, 1.0);
		}
	}

	DrawPoly(TRUE);

	return FALSE;
}

BOOL cD6Primitives::SPoly(int n, r3s_point **ppl)
{
	auto c0 = pcStates->get_color();
	auto *vlist = ReservePolySlots(n);

	for (int j = 0; j < n; ++j)
	{
		auto i = std::min(ppl[j][0].grp.i, 1.0);
		auto r = std::min(((c0 >> 16) & 0xFF) * i, 255);
		auto g = std::min(((c0 >> 8) & 0xFF) * i, 255);
		auto b = std::min((c0 & 0xFF) * i, 255);

		vlist[j].color = (c0 & 0xFF000000) | (r << 16) | (g << 8) | b;
		vlist[j].specular = m_dcFogSpecular;

		auto _sx = ppl[j][0].grp.sx + fix_from_float(0.5);
		auto _sy = ppl[j][0].grp.sy + fix_from_float(0.5);
		_sx = std::clamp(_sx, grd_canvas->gc.clip.f.left, grd_canvas->gc.clip.f.right);
		_sy = std::clamp(_sy, grd_canvas->gc.clip.f.top, grd_canvas->gc.clip.f.bot);
		vlist[j].sx = fix_float(_sx) + g_XOffset;
		vlist[j].sy = fix_float(_sy) + g_YOffset;

		if (zlinear)
		{
			vlist[j]->sz = z2d;
		}
		else if (lgd3d_z_normal)
		{
			vlist[j]->sz = ppl[j][0].p.z * inv_z_far;
		}
		else
		{
			vlist[j]->sz = z1 - ppl[j][0].grp.w * z2;
			vlist[j]->rhw = ppl[j][0].grp.w;
			vlist[j].sz = std::clamp(vlist[j].sz, 0.0, 1.0);
		}
	}

	DrawPoly(TRUE);

	return FALSE;
}

BOOL cD6Primitives::RGB_Poly(int n, r3s_point **ppl)
{
	auto c0 = pcStates->get_color();
	auto *vlist = ReservePolySlots(n);

	for (int j = 0; j < n; ++j)
	{
		auto r = std::min(((c0 >> 16) & 0xFF) * ppl[j][0].grp.i, 255);
		auto g = std::min(((c0 >> 8) & 0xFF) * ppl[j][1].p.x, 255);
		auto b = std::min((c0 & 0xFF) * ppl[j][1].p.y, 255);

		vlist[j].color = (m_nAlpha << 24) | (r << 16) | (g << 8) | b;
		vlist[j].specular = m_dcFogSpecular;

		auto _sx = ppl[j][0].grp.sx + fix_from_float(0.5);
		auto _sy = ppl[j][0].grp.sy + fix_from_float(0.5);
		_sx = std::clamp(_sx, grd_canvas->gc.clip.f.left, grd_canvas->gc.clip.f.right);
		_sy = std::clamp(_sy, grd_canvas->gc.clip.f.top, grd_canvas->gc.clip.f.bot);
		vlist[j].sx = fix_float(_sx) + g_XOffset;
		vlist[j].sy = fix_float(_sy) + g_YOffset;

		if (zlinear)
		{
			vlist[j]->sz = z2d;
		}
		else if (lgd3d_z_normal)
		{
			vlist[j]->sz = ppl[j][0].p.z * inv_z_far;
		}
		else
		{
			vlist[j]->sz = z1 - ppl[j][0].grp.w * z2;
			vlist[j]->rhw = ppl[j][0].grp.w;
			vlist[j].sz = std::clamp(vlist[j].sz, 0.0, 1.0);
		}
	}

	DrawPoly(TRUE);

	return FALSE;
}

BOOL cD6Primitives::RGBA_Poly(int n, r3s_point **ppl)
{
	auto c0 = pcStates->get_color();
	auto *vlist = ReservePolySlots(n);

	for (int j = 0; j < n; ++j)
	{
		auto r = std::min(((c0 >> 16) & 0xFF) * ppl[j][0].grp.i, 255);
		auto g = std::min(((c0 >> 8) & 0xFF) * ppl[j][1].p.x, 255);
		auto b = std::min((c0 & 0xFF) * ppl[j][1].p.y, 255);
		auto a = std::min(m_nAlpha * ppl[j][1].p.z, 255);

		vlist[j].color = (a << 24) | (r << 16) | (g << 8) | b;
		vlist[j].specular = m_dcFogSpecular;

		auto _sx = ppl[j][0].grp.sx + fix_from_float(0.5);
		auto _sy = ppl[j][0].grp.sy + fix_from_float(0.5);
		_sx = std::clamp(_sx, grd_canvas->gc.clip.f.left, grd_canvas->gc.clip.f.right);
		_sy = std::clamp(_sy, grd_canvas->gc.clip.f.top, grd_canvas->gc.clip.f.bot);
		vlist[j].sx = fix_float(_sx) + g_XOffset;
		vlist[j].sy = fix_float(_sy) + g_YOffset;

		if (zlinear)
		{
			vlist[j].sz = z2d;
		}
		else if (lgd3d_z_normal)
		{
			vlist[j].sz = ppl[j][0].p.z * inv_z_far;
		}
		else
		{
			vlist[j].sz = z1 - ppl[j][0].grp.w * z2;
			vlist[j].rhw = ppl[j][0].grp.w;
			vlist[j].sz = std::clamp(vlist[j].sz, 0.0, 1.0);
		}
	}

	DrawPoly(TRUE);

	return FALSE;
}

BOOL cD6Primitives::Trifan(int n, r3s_point **ppl)
{
	auto c0 = (m_nAlpha << 24) + 0xFFFFFF;
	auto *vlist = ReservePolySlots(n);

	for (int j = 0; j < n; ++j)
	{
		vlist[j].color = c0;
		vlist[j].specular = m_dcFogSpecular;
		vlist[j].tu = ppl[j][0].grp.u;
		vlist[j].tv = ppl[j][0].grp.v;

		auto _sx = ppl[j][0].grp.sx + fix_from_float(0.5);
		auto _sy = ppl[j][0].grp.sy + fix_from_float(0.5);
		_sx = std::clamp(_sx, grd_canvas->gc.clip.f.left, grd_canvas->gc.clip.f.right);
		_sy = std::clamp(_sy, grd_canvas->gc.clip.f.top, grd_canvas->gc.clip.f.bot);
		vlist[j].sx = fix_float(_sx) + g_XOffset;
		vlist[j].sy = fix_float(_sy) + g_YOffset;

		if (zlinear)
		{
			vlist[j].sz = z2d;
		}
		else if (lgd3d_z_normal)
		{
			vlist[j].sz = ppl[j][0].p.z * inv_z_far;
		}
		else
		{
			vlist[j].sz = z1 - ppl[j][0].grp.w * z2;
			vlist[j].rhw = ppl[j][0].grp.w;
			vlist[j].sz = std::clamp(vlist[j].sz, 0.0, 1.0);
		}
	}

	DrawPoly(FALSE);

	return FALSE;
}

BOOL cD6Primitives::LitTrifan(int n, r3s_point **ppl)
{
	auto c0 = (m_nAlpha << 24) + 0xFFFFFF;
	auto *vlist = ReservePolySlots(n);

	for (int j = 0; j < n; ++j)
	{
		auto i = std::min(ppl[j][0].grp.i, 1.0);
		auto r = std::min(((c0 >> 16) & 0xFF) * i, 255);
		auto g = std::min(((c0 >> 8) & 0xFF) * i, 255);
		auto b = std::min((c0 & 0xFF) * i, 255);

		vlist[j].color = (c0 & 0xFF000000) | (r << 16) | (g << 8) | b;
		vlist[j].specular = m_dcFogSpecular;
		vlist[j].tu = ppl[j][0].grp.u;
		vlist[j].tv = ppl[j][0].grp.v;

		auto _sx = ppl[j][0].grp.sx + fix_from_float(0.5);
		auto _sy = ppl[j][0].grp.sy + fix_from_float(0.5);
		_sx = std::clamp(_sx, grd_canvas->gc.clip.f.left, grd_canvas->gc.clip.f.right);
		_sy = std::clamp(_sy, grd_canvas->gc.clip.f.top, grd_canvas->gc.clip.f.bot);
		vlist[j].sx = fix_float(_sx) + g_XOffset;
		vlist[j].sy = fix_float(_sy) + g_YOffset;

		if (zlinear)
		{
			vlist[j].sz = z2d;
		}
		else if (lgd3d_z_normal)
		{
			vlist[j].sz = ppl[j][0].p.z * inv_z_far;
		}
		else
		{
			vlist[j].sz = z1 - ppl[j][0].grp.w * z2;
			vlist[j].rhw = ppl[j][0].grp.w;
			vlist[j].sz = std::clamp(vlist[j].sz, 0.0, 1.0);
		}
	}

	DrawPoly(FALSE);

	return FALSE;
}

BOOL cD6Primitives::RGBlitTrifan(int n, r3s_point **ppl)
{
	auto *vlist = ReservePolySlots(n);

	for (int j = 0; j < n; ++j)
	{
		auto r = std::min(255.0 * ppl[j][0].grp.coord[0], 255);
		auto g = std::min(255.0 * ppl[j][0].grp.coord[3], 255);
		auto b = std::min(255.0 * ppl[j][0].grp.coord[4], 255);

		vlist[j].color = (m_nAlpha << 24) | (r << 16) | (g << 8) | b;
		vlist[j].specular = m_dcFogSpecular;
		vlist[j].tu = ppl[j][0].grp.u;
		vlist[j].tv = ppl[j][0].grp.v;

		auto _sx = ppl[j][0].grp.sx + fix_from_float(0.5);
		auto _sy = ppl[j][0].grp.sy + fix_from_float(0.5);
		_sx = std::clamp(_sx, grd_canvas->gc.clip.f.left, grd_canvas->gc.clip.f.right);
		_sy = std::clamp(_sy, grd_canvas->gc.clip.f.top, grd_canvas->gc.clip.f.bot);
		vlist[j].sx = fix_float(_sx) + g_XOffset;
		vlist[j].sy = fix_float(_sy) + g_YOffset;

		if (zlinear)
		{
			vlist[j].sz = z2d;
		}
		else if (lgd3d_z_normal)
		{
			vlist[j].sz = ppl[j][0].p.z * inv_z_far;
		}
		else
		{
			vlist[j].sz = z1 - ppl[j][0].grp.w * z2;
			vlist[j].rhw = ppl[j][0].grp.w;
			vlist[j].sz = std::clamp(vlist[j].sz, 0.0, 1.0);
		}
	}

	DrawPoly(FALSE);

	return FALSE;
}

BOOL cD6Primitives::RGBAlitTrifan(int n, r3s_point **ppl)
{
	auto *vlist = ReservePolySlots(n);

	for (int j = 0; j < n; ++j)
	{
		auto r = std::min(255.0 * ppl[j][0].grp.coord[0], 255);
		auto g = std::min(255.0 * ppl[j][0].grp.coord[3], 255);
		auto b = std::min(255.0 * ppl[j][0].grp.coord[4], 255);
		auto a = std::min(255.0 * ppl[j][0].grp.coord[5], 255);

		vlist[j].color = (a << 24) | (r << 16) | (g << 8) | b;
		vlist[j].specular = m_dcFogSpecular;
		vlist[j].tu = ppl[j][0].grp.u;
		vlist[j].tv = ppl[j][0].grp.v;

		auto _sx = ppl[j][0].grp.sx + fix_from_float(0.5);
		auto _sy = ppl[j][0].grp.sy + fix_from_float(0.5);
		_sx = std::clamp(_sx, grd_canvas->gc.clip.f.left, grd_canvas->gc.clip.f.right);
		_sy = std::clamp(_sy, grd_canvas->gc.clip.f.top, grd_canvas->gc.clip.f.bot);
		vlist[j].sx = fix_float(_sx) + g_XOffset;
		vlist[j].sy = fix_float(_sy) + g_YOffset;

		if (zlinear)
		{
			vlist[j].sz = z2d;
		}
		else if (lgd3d_z_normal)
		{
			vlist[j].sz = ppl[j][0].p.z * inv_z_far;
		}
		else
		{
			vlist[j].sz = z1 - ppl[j][0].grp.w * z2;
			vlist[j].rhw = ppl[j][0].grp.w;
			vlist[j].sz = std::clamp(vlist[j].sz, 0.0, 1.0);
		}
	}

	DrawPoly(FALSE);

	return FALSE;
}

BOOL cD6Primitives::RGBAFogLitTrifan(int n, r3s_point **ppl)
{
	auto *vlist = ReservePolySlots(n);

	for (int j = 0; j < n; ++j)
	{
		auto r = std::min(255 * ppl[j][0].grp.coord[0], 255);
		auto g = std::min(255 * ppl[j][0].grp.coord[3], 255);
		auto b = std::min(255 * ppl[j][0].grp.coord[4], 255);
		auto a = std::min(255 * ppl[j][0].grp.coord[5], 255);
		auto f = std::min((1.0 - ppl[j][0].grp.coord[6]) * 255, 255);

		vlist[j].color = (a << 24) | (r << 16) | (g << 8) | b;
		vlist[j].specular = f << 24;
		vlist[j].tu = ppl[j][0].grp.u;
		vlist[j].tv = ppl[j][0].grp.v;

		auto _sx = ppl[j][0].grp.sx + fix_from_float(0.5);
		auto _sy = ppl[j][0].grp.sy + fix_from_float(0.5);
		_sx = std::clamp(_sx, grd_canvas->gc.clip.f.left, grd_canvas->gc.clip.f.right);
		_sy = std::clamp(_sy, grd_canvas->gc.clip.f.top, grd_canvas->gc.clip.f.bot);
		vlist[j].sx = fix_float(_sx) + g_XOffset;
		vlist[j].sy = fix_float(_sy) + g_YOffset;

		if (zlinear)
		{
			vlist[j].sz = z2d;
		}
		else if (lgd3d_z_normal)
		{
			vlist[j].sz = ppl[j][0].p.z * inv_z_far;
		}
		else
		{
			vlist[j].sz = z1 - ppl[j][0].grp.w * z2;
			vlist[j].rhw = ppl[j][0].grp.w;
			vlist[j].sz = std::clamp(vlist[j].sz, 0.0, 1.0);
		}
	}

	DrawPoly(FALSE);

	return FALSE;
}

BOOL cD6Primitives::DiffuseSpecularLitTrifan(int n, r3s_point **ppl)
{
	auto *vlist = ReservePolySlots(n);

	for (int j = 0; j < n; ++j)
	{
		auto r = std::min(255 * ppl[j][0].grp.coord[0], 255);
		auto g = std::min(255 * ppl[j][0].grp.coord[3], 255);
		auto b = std::min(255 * ppl[j][0].grp.coord[4], 255);
		auto a = std::min(255 * ppl[j][0].grp.coord[5], 255);

		auto aa = std::min(255 * ppl[j][0].grp.coord[7], 255);
		auto ra = std::min(255 * ppl[j][0].grp.coord[8], 255);
		auto ga = std::min(255 * ppl[j][0].grp.coord[9], 255);
		auto ba = std::min(255 * ppl[j][0].grp.coord[10], 255);

		vlist[j].color = (a << 24) | (r << 16) | (g << 8) | b;
		vlist[j].specular = (aa << 24) | (ra << 16) | (ga << 8) | ba;
		vlist[j].tu = ppl[j][0].grp.u;
		vlist[j].tv = ppl[j][0].grp.v;

		auto _sx = ppl[j][0].grp.sx + fix_from_float(0.5);
		auto _sy = ppl[j][0].grp.sy + fix_from_float(0.5);
		_sx = std::clamp(_sx, grd_canvas->gc.clip.f.left, grd_canvas->gc.clip.f.right);
		_sy = std::clamp(_sy, grd_canvas->gc.clip.f.top, grd_canvas->gc.clip.f.bot);
		vlist[j].sx = fix_float(_sx) + g_XOffset;
		vlist[j].sy = fix_float(_sy) + g_YOffset;

		if (zlinear)
		{
			vlist[j].sz = z2d;
		}
		else if (lgd3d_z_normal)
		{
			vlist[j].sz = ppl[j][0].p.z * inv_z_far;
		}
		else
		{
			vlist[j].sz = z1 - ppl[j][0].grp.w * z2;
			vlist[j].rhw = ppl[j][0].grp.w;
			vlist[j].sz = std::clamp(vlist[j].sz, 0.0, 1.0);
		}
	}

	DrawPoly(FALSE);

	return FALSE;
}

BOOL cD6Primitives::g2UPoly(int n, g2s_point **ppl)
{
	auto c0 = pcStates->get_color();
	auto *vlist = ReservePolySlots(n);

	for (int j = 0; j < n; ++j)
	{
		vlist[j].color = c0;
		vlist[j].specular = m_dcFogSpecular;
		vlist[j].sz = z2d;
		vlist[j].rhw = w2d;

		auto _sx = ppl[j][0].grp.sx + fix_from_float(0.5);
		auto _sy = ppl[j][0].grp.sy + fix_from_float(0.5);
		_sx = std::clamp(_sx, grd_canvas->gc.clip.f.left, grd_canvas->gc.clip.f.right);
		_sy = std::clamp(_sy, grd_canvas->gc.clip.f.top, grd_canvas->gc.clip.f.bot);
		vlist[j].sx = fix_float(_sx) + g_XOffset;
		vlist[j].sy = fix_float(_sy) + g_YOffset;
	}

	DrawPoly(TRUE);

	return FALSE;
}

BOOL cD6Primitives::g2Poly(int n, g2s_point **ppl)
{
	g2s_point **cpl = nullptr;
	auto m = g2_clip_poly(n, G2C_CLIP_NONE, ppl, &cpl);
	if (m >= 3)
	{
		g2UPoly(m, cpl);
		code = 0;
	}
	else
	{
		code = 16;
	}

	if (cpl != nullptr && cpl != ppl)
		temp_free(cpl);

	return code;
}

BOOL cD6Primitives::g2UTrifan(int n, g2s_point **ppl)
{
	auto c0 = (m_nAlpha << 24) + 0xFFFFFF;
	auto *vlist = ReservePolySlots(n);

	for (int j = 0; j < n; ++j)
	{
		auto i = ppl[j][0].coord[0];
		auto r = std::min(((c0 >> 16) & 0xFF) * i, 255);
		auto g = std::min(((c0 >> 8) & 0xFF) * i, 255);
		auto b = std::min((c0 & 0xFF) * i, 255);

		vlist[j].color = (c0 & 0xFF000000) | (r << 16) | (g << 8) | b;
		vlist[j].specular = m_dcFogSpecular;
		vlist[j].tu = ppl[j][0].coord[1];
		vlist[j].tv = ppl[j][0].coord[2];
		vlist[j].sz = z2d;
		vlist[j].rhw = w2d;

		auto _sx = ppl[j][0].grp.sx + fix_from_float(0.5);
		auto _sy = ppl[j][0].grp.sy + fix_from_float(0.5);
		_sx = std::clamp(_sx, grd_canvas->gc.clip.f.left, grd_canvas->gc.clip.f.right);
		_sy = std::clamp(_sy, grd_canvas->gc.clip.f.top, grd_canvas->gc.clip.f.bot);
		vlist[j].sx = fix_float(_sx) + g_XOffset;
		vlist[j].sy = fix_float(_sy) + g_YOffset;
	}

	DrawPoly(FALSE);

	return FALSE;
}

BOOL cD6Primitives::g2Trifan(int n, g2s_point **ppl)
{
	g2s_point **cpl = nullptr;
	auto m = g2_clip_poly(n, G2C_CLIP_UVI, ppl, &cpl);
	if (m >= 3)
	{
		g2UTrifan(m, cpl);
		code = 0;
	}
	else
	{
		code = 16;
	}

	if (cpl != nullptr && cpl != ppl)
		temp_free(cpl);

	return code;
}

ePolyMode cD6Primitives::GetPolyMode()
{
	return m_ePolyMode;
}

BOOL cD6Primitives::SetPolyMode(ePolyMode eNewMode)
{
	if (eNewMode == ePolyMode::kLgd3dILLEGALPolyMode)
		return FALSE;

	if (m_ePolyMode != eNewMode)
		FlushPrimitives();

	m_ePolyMode = eNewMode;

	return TRUE;
}

void cD6Primitives::DrawStandardEdges(void *pVertera, DWORD dwNoVeriteces)
{
	if (!g_bTexSuspended)
	{
		FlushIndPolies();
		StartNonTexMode();
	}

	waEdgeIndices[dwNoVeriteces] = 0;
	auto hResult = g_lpD3Ddevice->DrawIndexedPrimitive(D3DPT_LINESTRIP, D3DFVF_TLVERTEX, pVertera, dwNoVeriteces, waEdgeIndices, dwNoVeriteces + 1, D3DDP_DONOTCLIP | D3DDP_DONOTUPDATEEXTENTS);
	waEdgeIndices[dwNoVeriteces] = dwNoVeriteces;

	AssertMsg3(SUCCEEDED(hResult), "%s: error %d\n%s ", "DrawStandardEdges failed", hResult, GetDDErrorMsg(hResult));
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

void cD6Primitives::PassFogSpecularColor(ulong dcFogColor)
{
	m_dcFogSpecular = dcFogColor;
}

void cD6Primitives::PassAlphaColor(int nAlapha)
{
	m_nAlpha = nAlapha;
}

void cD6Primitives::StartFrame()
{
	lgd3d_clip.left = fix_float(grd_canvas->gc.clip.f.left) + g_XOffset;
	lgd3d_clip.right = fix_float(grd_canvas->gc.clip.f.right) + g_XOffset;
	lgd3d_clip.top = fix_float(grd_canvas->gc.clip.f.top) + g_YOffset;
	lgd3d_clip.bot = fix_float(grd_canvas->gc.clip.f.bot) + g_YOffset;

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
	auto fc_save = grd_canvas->gc.fcolor;

	constexpr auto vlist_size = 4;
	auto *vlist = ReservePolySlots(vlist_size);

	grd_canvas->gc.fcolor = c;

	auto ca = pcStates->get_color();

	grd_canvas->gc.fcolor = fc_save;

	float x0 = fix_float(grd_canvas->gc.clip.f.left) + g_XOffset;
	float x1 = fix_float(grd_canvas->gc.clip.f.right) + g_XOffset;
	float y0 = fix_float(grd_canvas->gc.clip.f.top) + g_YOffset;
	float y1 = fix_float(grd_canvas->gc.clip.f.bot) + g_YOffset;

	for (int i = 0; i < vlist_size; ++i)
	{
		vlist[i].sz = z2d;
		vlist[i].rhw = w2d;
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
		CriticalMsg1("A poly has more than %d points!", m_dwTempIndCounter);
	}

	m_waIndices[m_dwNoIndices] = m_waTempIndices[0];
	m_waIndices[m_dwNoIndices + 1] = m_waTempIndices[1];
	m_waIndices[m_dwNoIndices + 2] = m_waTempIndices[2];
	m_dwNoIndices += 3;

	for (int i = 3; i < m_dwTempIndCounter; ++i)
	{
		m_waIndices[m_dwNoIndices] = m_waTempIndices[0];
		// FIXME: wtf?
		m_waIndices[m_dwNoIndices + 1] = *((WORD *)&m_dwNoIndices + i + 1);
		m_waIndices[m_dwNoIndices + 2] = m_waTempIndices[i];
		m_dwNoIndices += 3;
	}

	m_dwTempIndCounter = 0;
	m_bPrimitivesPending = TRUE;
}

void cD6Primitives::FlushIndPolies()
{
	if (!m_dwNoIndices)
		return;

	for (int i = 0; i < m_dwNoIndices; ++i)
	{
		AssertMsg(m_waIndices[i] <= m_dwMaxVIndex, "Runaway Prim index!")

			if (m_dwMinVIndex)
				m_waIndices[i] -= m_dwMinVIndex;
	}

	if (!lgd3d_punt_d3d)
	{
		auto hResult = g_lpD3Ddevice->DrawIndexedPrimitive(
			D3DPT_TRIANGLELIST,
			D3DFVF_TLVERTEX,
			&m_pVIB[m_dwMinVIndex],
			m_dwMaxVIndex + 1 - m_dwMinVIndex,
			m_waIndices,
			m_dwNoIndices,
			D3DDP_DONOTCLIP | D3DDP_DONOTUPDATEEXTENTS);
		AssertMsg3(SUCCEEDED(hResult), "%s: error %d\n%s ", "DrawIndexedPrimitive failed", hResult, GetDDErrorMsg(hResult));
	}

	m_dwNoIndices = 0;
	m_dwMinVIndex = 256;
	m_dwMaxVIndex = 0;
}

D3DTLVERTEX *cD6Primitives::ReservePointSlots(int n)
{
	if (!m_bPointMode)
	{
		FlushIndPolies();
		m_bPointMode = TRUE;
	}

	if (n + m_dwNoCashedPoints > m_dwPointBufferSize)
		FlushPoints();

	AssertMsg(n + m_dwNoCashedPoints <= m_dwPointBufferSize, "ReservePointSlots(): too many points!");

	auto *retval = &m_saPointBuffer[m_dwNoCashedPoints];

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
			D3DFVF_TLVERTEX,
			m_saPointBuffer,
			m_dwNoCashedPoints,
			D3DDP_DONOTCLIP | D3DDP_DONOTUPDATEEXTENTS);

		AssertMsg3(SUCCEEDED(hResult), "%s: error %d\n%s ", "DrawPrimitive(points) failed", hResult, GetDDErrorMsg(hResult));

		m_dwNoCashedPoints = 0;
	}
}

int cD6Primitives::DrawPoint(r3s_point *p)
{
	auto sx = p->grp.sx + fix_from_float(0.5);
	auto sy = p->grp.sy + fix_from_float(0.5);
	if (sx > grd_canvas->gc.clip.f.right || sx < grd_canvas->gc.clip.f.left || sy > grd_canvas->gc.clip.f.bot || sy < grd_canvas->gc.clip.f.top)
	{
		return 16;
	}

	auto *vp = ReservePointSlots(1);
	vp->sx = fix_float(sx) + g_XOffset;
	vp->sy = fix_float(sy) + g_YOffset;

	auto color = pcStates->get_color();
	vp->color = color | 0xFF000000;
	vp->specular = m_dcFogSpecular;

	if (zlinear)
	{
		vp->sz = z2d;
	}
	else if (lgd3d_z_normal)
	{
		vp->sz = p->p.z * inv_z_far;
	}
	else
	{
		vp->sz = std::clamp(z1 - p->grp.w * z2, 0.0, 1.0);
	}

	return 0;
}

void cD6Primitives::DrawLine(r3s_point *p0, r3s_point *p1)
{
	auto c = (m_nAlpha << 24) | pcStates->get_color();

	auto left = grd_canvas->gc.clip.f.left;
	auto right = grd_canvas->gc.clip.f.right;
	auto top = grd_canvas->gc.clip.f.top;
	auto bot = grd_canvas->gc.clip.f.bot;

	if (bot - top >= fix_from_float(1) && right - left >= fix_from_float(1))
	{
		constexpr auto vsize = 4;
		auto *vlist = ReservePolySlots(vsize);
		for (int i = 0; i < vsize; ++i)
		{
			vlist[i].color = c;
			vlist[i].specular = m_dcFogSpecular;
			if (zlinear)
			{
				vlist[i].sz = z2d;
			}
			else if (lgd3d_z_normal)
			{
				vlist[i].sz = p0->p.z * inv_z_far;
			}
			else
			{
				vlist[i].sz = std::clamp(z1 - p0->grp.w * z2, 0, 1);
				vlist[i].rhw = p0->grp.w;
			}

			vlist[i].tv = 0.0;
			vlist[i].tu = 0.0;
		}

		auto x10 = std::abs(p1->grp.sx - p0->grp.sx);
		auto y10 = std::abs(p1->grp.sy - p0->grp.sy);

		if (y10 <= x10)
		{
			top += fix_from_float(0.5);
			bot -= fix_from_float(0.5);
			if (p0->grp.sx > p1->grp.sx)
			{
				std::swap(p0, p1);
			}
		}
		else
		{
			left += fix_from_float(0.5);
			right -= fix_from_float(0.5);
			if (p0->grp.sy > p1->grp.sy)
			{
				std::swap(p0, p1);
			}
		}

		auto x0 = fix_float(std::clamp(p0->grp.sx + fix_from_float(0.5), left, right)) + g_XOffset;
		auto x1 = fix_float(std::clamp(p1->grp.sx + fix_from_float(0.5), left, right)) + g_XOffset;
		auto y0 = fix_float(std::clamp(p0->grp.sy + fix_from_float(0.5), bot, top)) + g_YOffset;
		auto y1 = fix_float(std::clamp(p1->grp.sy + fix_from_float(0.5), bot, top)) + g_YOffset;

		if (y10 <= x10)
		{
			vlist[0].sy = y0 + 0.5;
			vlist[1].sy = y0 - 0.5;
			vlist[2].sy = y1 - 0.5;
			vlist[3].sy = y1 + 0.5;

			vlist[0].sx = x0;
			vlist[1].sx = x0;
			vlist[2].sx = x1;
			vlist[3].sx = x1;
		}
		else
		{
			vlist[0].sx = x0 - 0.5;
			vlist[1].sx = x0 + 0.5;
			vlist[2].sx = x1 + 0.5;
			vlist[3].sx = x1 - 0.5;

			vlist[0].sy = y0;
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

	m_pVIB = (D3DTLVERTEX *)malloc(sizeof(D3DTLVERTEX) * m_dwVIBSizeInEntries);
	InitializeVIBCounters();

	m_dwVIBmax = 256;

	return m_pVIB != nullptr;
}

void cD6Primitives::DeleteVertIndBuffer()
{
	free(m_pVIB);
	m_pVIB = nullptr;
}

BOOL cD6Primitives::ResizeVertIndBuffer(DWORD dwNewNoEntries)
{
	auto *pvTemp = (D3DTLVERTEX *)realloc(m_pVIB, sizeof(D3DTLVERTEX) * dwNewNoEntries);
	if (!pvTemp)
		return FALSE;

	m_pVIB = pvTemp;
	m_dwVIBSizeInEntries = dwNewNoEntries;

	return TRUE;
}

void cD6Primitives::init_hack_light_bm()
{
	m_hack_light_bm = gr_alloc_bitmap(2u, 2u, 32, 32);
	uchar *bits = m_hack_light_bm->bits;
	for (int i = 0; i < 32; ++i)
	{
		for (int j = 0; j < 32; ++j)
		{
			float alpha = 128.0 / (std::pow(i - 15.5, 2) + std::pow(j - 15.5, 2));

			bits[j] = std::min(alpha, 15);
		}

		bits += 32;
	}
}

void cD6Primitives::do_quad_light(r3s_point *p, float r, grs_bitmap *bm)
{

	auto x = fix_float(p->grp.sx) + g_XOffset;
	auto y = fix_float(p->grp.sy) + g_YOffset;
	if (x + r >= lgd3d_clip.left && x - r <= lgd3d_clip.right && y + r >= lgd3d_clip.top && y - r <= lgd3d_clip.bot)
	{
		auto c = pcStates->get_color() | 0xFF'00'00'00;
		auto fog_specular = m_dcFogSpecular;
		grd_canvas->gc.fill_type = FILL_BLEND;
		pcStates->SetAlphaPalette(hack_light_alpha_pal);
		lgd3d_set_texture(bm);

		constexpr auto vsize = 4;
		auto *vlist = ReservePolySlots(vsize);
		auto x_right = x + r;
		auto x_left = x - r;
		auto u_left = 0.0;
		auto u_right = 1.0;
		if (x_left < lgd3d_clip.left)
		{
			u_left = (lgd3d_clip.left - x_left) / (2.0 * r);
			x_left = lgd3d_clip.left;
		}
		if (x_right > lgd3d_clip.right)
		{
			u_right = (1.0 - u_left) * (lgd3d_clip.right - x_left) / (x_right - x_left) + u_left;
			x_right = lgd3d_clip.right;
		}

		auto y_bot = y + r;
		auto y_top = y - r;
		auto v_top = 0.0;
		auto v_bot = 1.0;
		if (y_top < lgd3d_clip.top)
		{
			v_top = (lgd3d_clip.top - y_top) / (2.0 * r);
			y_top = lgd3d_clip.top;
		}
		if (y_bot > lgd3d_clip.bot)
		{
			v_bot = v_top + (1.0 - v_top) * (lgd3d_clip.bot - y_top) / (y_bot - y_top);
			y_bot = lgd3d_clip.bot;
		}

		vlist[0].tu = u_left;
		vlist[0].tv = v_top;
		vlist[0].sx = x_left;
		vlist[0].sy = y_top;

		vlist[1].tu = u_right;
		vlist[1].tv = v_top;
		vlist[1].sx = x_right;
		vlist[1].sy = y_top;

		vlist[2].tu = u_right;
		vlist[2].tv = v_bot;
		vlist[2].sx = x_right;
		vlist[2].sy = y_bot;

		vlist[3].tu = u_left;
		vlist[3].tv = v_bot;
		vlist[3].sx = x_left;
		vlist[3].sy = y_bot;

		for (auto i = vsize; i >= 0; --i)
		{
			vlist[i].color = c;
			vlist[i].specular = fog_specular;
			if (zlinear)
			{
				vlist[i].sz = z2d;
			}
			else if (lgd3d_z_normal)
			{
				vlist[i].sz = p->p.z * inv_z_far;
			}
			else
			{
				vlist[i].sz = std::clamp(z1 - p->grp.w * z2, 0, 1);
				vlist[i].rhw = p->grp.w;
			}
		}

		DrawPoly(FALSE);

		grd_canvas->gc.fill_type = FILL_NORM;
	}
}

void cD6Primitives::HackLight(r3s_point *p, float r)
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

void cD6Primitives::HackLightExtra(r3s_point *p, float r, grs_bitmap *bm)
{
	if (r > 1.0)
		do_quad_light(p, r, bm);
	else
		DrawPoint(p);
}

void cD6Primitives::FlushPrimitives()
{
	if (!m_bPrimitivesPending || m_bFlushingOn)
		return;

	m_bFlushingOn = TRUE;
	FlushIndPolies();
	FlushPoints();
	m_bPrimitivesPending = FALSE;
	m_bFlushingOn = FALSE;
}

void cD6Primitives::EndIndexedRun()
{
	FlushIndPolies();
	InitializeVIBCounters();
}

void cD6Primitives::FlushIfNoFit(int nIndicesToAdd, BOOL bSuspendTexturing)
{
	if (m_bPointMode)
	{
		FlushPoints();
		m_bPointMode = FALSE;
	}

	if (bSuspendTexturing != g_bTexSuspended)
	{
		FlushIndPolies();
		if (bSuspendTexturing)
			StartNonTexMode();
		else
			EndNonTexMode();
	}

	if (nIndicesToAdd + m_dwTempIndCounter >= 50 || m_dwNoIndices + 3 * (nIndicesToAdd - 2) >= m_dwVIBmax)
	{
		FlushIndPolies();
		m_dwTempIndCounter = 0;
	}
}

D3DTLVERTEX *cD6Primitives::GetIndPolySlot(int nPolySize, r3ixs_info *psIndInfo)
{
	auto wIndex = psIndInfo->index;
	AssertMsg(psIndInfo->index < m_dwVIBSizeInEntries || ResizeVertIndBuffer(((wIndex >> 8) + 1) << 8),
			  "Could not reallocate memory for VIB");

	wIndex = std::clamp(wIndex, m_dwMinVIndex, m_dwMaxVIndex);

	m_waTempIndices[m_dwTempIndCounter++] = wIndex;
	if (psIndInfo->flags & 1)
		return nullptr;

	return &m_pVIB[wIndex];
}

BOOL cD6Primitives::PolyInd(int n, r3s_point **ppl, r3ixs_info *psIndInfo)
{
	auto c0 = pcStates->get_color();
	FlushIfNoFit(n, TRUE);

	for (int j = 0; j < n; ++j)
	{
		auto *pVertex = GetIndPolySlot(n, psIndInfo++);
		if (pVertex == nullptr)
			continue;

		pVertex->color = c0;
		pVertex->specular = m_dcFogSpecular;

		auto _sx = ppl[j][0].grp.sx + fix_from_float(0.5);
		auto _sy = ppl[j][0].grp.sy + fix_from_float(0.5);
		_sx = std::clamp(_sx, grd_canvas->gc.clip.f.left, grd_canvas->gc.clip.f.right);
		_sy = std::clamp(_sy, grd_canvas->gc.clip.f.top, grd_canvas->gc.clip.f.bot);
		pVertex->sx = fix_float(_sx) + g_XOffset;
		pVertex->sy = fix_float(_sy) + g_YOffset;

		if (zlinear)
		{
			pVertex->sz = z2d;
		}
		else if (lgd3d_z_normal)
		{
			pVertex->sz = ppl[j][0].p.z * inv_z_far;
		}
		else
		{
			pVertex->sz = std::clamp(z1 - ppl[j][0].grp.w * z2, 0.0, 1.0);
			pVertex->rhw = ppl[j][0].grp.w;
		}
	}

	DrawIndPolies();

	return FALSE;
}

BOOL cD6Primitives::SPolyInd(int n, r3s_point **ppl, r3ixs_info *psIndInfo)
{
	auto c0 = pcStates->get_color();
	FlushIfNoFit(n, TRUE);

	for (int j = 0; j < n; ++j)
	{
		auto *pVertex = GetIndPolySlot(n, psIndInfo++);
		if (pVertex == nullptr)
			continue;

		auto i = std::min(ppl[j][0].grp.i, 1.0);
		auto r = std::min(((c0 >> 16) & 0xFF) * i, 255);
		auto g = std::min(((c0 >> 8) & 0xFF) * i, 255);
		auto b = std::min((c0 & 0xFF) * i, 255);

		pVertex->color = (c0 & 0xFF000000) | (r << 16) | (g << 8) | b;
		pVertex->specular = m_dcFogSpecular;

		auto _sx = ppl[j][0].grp.sx + fix_from_float(0.5);
		auto _sy = ppl[j][0].grp.sy + fix_from_float(0.5);
		_sx = std::clamp(_sx, grd_canvas->gc.clip.f.left, grd_canvas->gc.clip.f.right);
		_sy = std::clamp(_sy, grd_canvas->gc.clip.f.top, grd_canvas->gc.clip.f.bot);
		pVertex->sx = fix_float(_sx) + g_XOffset;
		pVertex->sy = fix_float(_sy) + g_YOffset;

		if (zlinear)
		{
			pVertex->sz = z2d;
		}
		else if (lgd3d_z_normal)
		{
			pVertex->sz = ppl[j][0].p.z * inv_z_far;
		}
		else
		{
			pVertex->sz = std::clamp(z1 - ppl[j][0].grp.w * z2, 0.0, 1.0);
			pVertex->rhw = ppl[j][0].grp.w;
		}
	}

	DrawIndPolies();

	return FALSE;
}

BOOL cD6Primitives::RGB_PolyInd(int n, r3s_point **ppl, r3ixs_info *psIndInfo)
{
	auto c0 = pcStates->get_color();
	FlushIfNoFit(n, TRUE);

	for (int j = 0; j < n; ++j)
	{
		auto *pVertex = GetIndPolySlot(n, psIndInfo++);
		if (pVertex == nullptr)
			continue;

		auto r = std::min(((c0 >> 16) & 0xFF) * ppl[j][0].grp.i, 255);
		auto g = std::min(((c0 >> 8) & 0xFF) * ppl[j][1].p.x, 255);
		auto b = std::min((c0 & 0xFF) * ppl[j][1].p.y, 255);

		pVertex->color = (m_nAlpha << 24) | (r << 16) | (g << 8) | b;
		pVertex->specular = m_dcFogSpecular;

		auto _sx = ppl[j][0].grp.sx + fix_from_float(0.5);
		auto _sy = ppl[j][0].grp.sy + fix_from_float(0.5);
		_sx = std::clamp(_sx, grd_canvas->gc.clip.f.left, grd_canvas->gc.clip.f.right);
		_sy = std::clamp(_sy, grd_canvas->gc.clip.f.top, grd_canvas->gc.clip.f.bot);
		pVertex->sx = fix_float(_sx) + g_XOffset;
		pVertex->sy = fix_float(_sy) + g_YOffset;

		if (zlinear)
		{
			pVertex->sz = z2d;
		}
		else if (lgd3d_z_normal)
		{
			pVertex->sz = ppl[j][0].p.z * inv_z_far;
		}
		else
		{
			pVertex->sz = std::clamp(z1 - ppl[j][0].grp.w * z2, 0.0, 1.0);
			pVertex->rhw = ppl[j][0].grp.w;
		}
	}

	DrawIndPolies();

	return FALSE;
}

BOOL cD6Primitives::RGBA_PolyInd(int n, r3s_point **ppl, r3ixs_info *psIndInfo)
{
	auto c0 = pcStates->get_color();
	FlushIfNoFit(n, TRUE);

	for (int j = 0; j < n; ++j)
	{
		auto *pVertex = GetIndPolySlot(n, psIndInfo++);
		if (pVertex == nullptr)
			continue;

		auto r = std::min(((c0 >> 16) & 0xFF) * ppl[j][0].grp.i, 255);
		auto g = std::min(((c0 >> 8) & 0xFF) * ppl[j][1].p.x, 255);
		auto b = std::min((c0 & 0xFF) * ppl[j][1].p.y, 255);
		auto a = std::min(m_nAlpha * ppl[j][1].p.z, 255);

		pVertex->color = (a << 24) | (r << 16) | (g << 8) | b;
		pVertex->specular = m_dcFogSpecular;

		auto _sx = ppl[j][0].grp.sx + fix_from_float(0.5);
		auto _sy = ppl[j][0].grp.sy + fix_from_float(0.5);
		_sx = std::clamp(_sx, grd_canvas->gc.clip.f.left, grd_canvas->gc.clip.f.right);
		_sy = std::clamp(_sy, grd_canvas->gc.clip.f.top, grd_canvas->gc.clip.f.bot);
		pVertex->sx = fix_float(_sx) + g_XOffset;
		pVertex->sy = fix_float(_sy) + g_YOffset;

		if (zlinear)
		{
			pVertex->sz = z2d;
		}
		else if (lgd3d_z_normal)
		{
			pVertex->sz = ppl[j][0].p.z * inv_z_far;
		}
		else
		{
			pVertex->sz = std::clamp(z1 - ppl[j][0].grp.w * z2, 0.0, 1.0);
			pVertex->rhw = ppl[j][0].grp.w;
		}
	}

	DrawIndPolies();

	return FALSE;
}

BOOL cD6Primitives::TrifanInd(int n, r3s_point **ppl, r3ixs_info *psIndInfo)
{
	auto c0 = (m_nAlpha << 24) + 0xFFFFFF;
	FlushIfNoFit(n, FALSE);

	for (int j = 0; j < n; ++j)
	{
		auto *pVertex = GetIndPolySlot(n, psIndInfo++);
		if (pVertex == nullptr)
			continue;

		pVertex->color = c0;
		pVertex->specular = m_dcFogSpecular;
		pVertex->tu = ppl[j][0].grp.u;
		pVertex->tv = ppl[j][0].grp.v;

		auto _sx = ppl[j][0].grp.sx + fix_from_float(0.5);
		auto _sy = ppl[j][0].grp.sy + fix_from_float(0.5);
		_sx = std::clamp(_sx, grd_canvas->gc.clip.f.left, grd_canvas->gc.clip.f.right);
		_sy = std::clamp(_sy, grd_canvas->gc.clip.f.top, grd_canvas->gc.clip.f.bot);
		pVertex->sx = fix_float(_sx) + g_XOffset;
		pVertex->sy = fix_float(_sy) + g_YOffset;

		if (zlinear)
		{
			pVertex->sz = z2d;
		}
		else if (lgd3d_z_normal)
		{
			pVertex->sz = ppl[j][0].p.z * inv_z_far;
		}
		else
		{
			pVertex->sz = std::clamp(z1 - ppl[j][0].grp.w * z2, 0.0, 1.0);
			pVertex->rhw = ppl[j][0].grp.w;
		}
	}

	DrawIndPolies();

	return FALSE;
}

BOOL cD6Primitives::LitTrifanInd(int n, r3s_point **ppl, r3ixs_info *psIndInfo)
{
	auto c0 = (m_nAlpha << 24) + 0xFFFFFF;
	FlushIfNoFit(n, FALSE);

	for (int j = 0; j < n; ++j)
	{
		auto *pVertex = GetIndPolySlot(n, psIndInfo++);
		if (pVertex == nullptr)
			continue;

		auto i = std::min(ppl[j][0].grp.i, 1.0);
		auto r = std::min(((c0 >> 16) & 0xFF) * i, 255);
		auto g = std::min(((c0 >> 8) & 0xFF) * i, 255);
		auto b = std::min((c0 & 0xFF) * i, 255);

		pVertex->color = (c0 & 0xFF000000) | (r << 16) | (g << 8) | b;
		pVertex->specular = m_dcFogSpecular;

		auto _sx = ppl[j][0].grp.sx + fix_from_float(0.5);
		auto _sy = ppl[j][0].grp.sy + fix_from_float(0.5);
		_sx = std::clamp(_sx, grd_canvas->gc.clip.f.left, grd_canvas->gc.clip.f.right);
		_sy = std::clamp(_sy, grd_canvas->gc.clip.f.top, grd_canvas->gc.clip.f.bot);
		pVertex->sx = fix_float(_sx) + g_XOffset;
		pVertex->sy = fix_float(_sy) + g_YOffset;

		if (zlinear)
		{
			pVertex->sz = z2d;
		}
		else if (lgd3d_z_normal)
		{
			pVertex->sz = ppl[j][0].p.z * inv_z_far;
		}
		else
		{
			pVertex->sz = std::clamp(z1 - ppl[j][0].grp.w * z2, 0.0, 1.0);
			pVertex->rhw = ppl[j][0].grp.w;
		}
	}

	DrawIndPolies();

	return FALSE;
}

BOOL cD6Primitives::RGBlitTrifanInd(int n, r3s_point **ppl, r3ixs_info *psIndInfo)
{
	FlushIfNoFit(n, FALSE);

	for (int j = 0; j < n; ++j)
	{
		auto *pVertex = GetIndPolySlot(n, psIndInfo++);
		if (pVertex == nullptr)
			continue;

		auto r = std::min(((c0 >> 16) & 0xFF) * ppl[j][0].grp.i, 255);
		auto g = std::min(((c0 >> 8) & 0xFF) * ppl[j][1].p.x, 255);
		auto b = std::min((c0 & 0xFF) * ppl[j][1].p.y, 255);

		pVertex->color = (m_nAlpha << 24) | (r << 16) | (g << 8) | b;
		pVertex->specular = m_dcFogSpecular;
		pVertex->tu = ppl[j][0].grp.u;
		pVertex->tv = ppl[j][0].grp.v;

		auto _sx = ppl[j][0].grp.sx + fix_from_float(0.5);
		auto _sy = ppl[j][0].grp.sy + fix_from_float(0.5);
		_sx = std::clamp(_sx, grd_canvas->gc.clip.f.left, grd_canvas->gc.clip.f.right);
		_sy = std::clamp(_sy, grd_canvas->gc.clip.f.top, grd_canvas->gc.clip.f.bot);
		pVertex->sx = fix_float(_sx) + g_XOffset;
		pVertex->sy = fix_float(_sy) + g_YOffset;

		if (zlinear)
		{
			pVertex->sz = z2d;
		}
		else if (lgd3d_z_normal)
		{
			pVertex->sz = ppl[j][0].p.z * inv_z_far;
		}
		else
		{
			pVertex->sz = std::clamp(z1 - ppl[j][0].grp.w * z2, 0.0, 1.0);
			pVertex->rhw = ppl[j][0].grp.w;
		}
	}

	DrawIndPolies();

	return FALSE;
}

BOOL cD6Primitives::RGBAlitTrifanInd(int n, r3s_point **ppl, r3ixs_info *psIndInfo)
{
	FlushIfNoFit(n, FALSE);

	for (int j = 0; j < n; ++j)
	{
		auto *pVertex = GetIndPolySlot(n, psIndInfo++);
		if (pVertex == nullptr)
			continue;

		auto r = std::min(((c0 >> 16) & 0xFF) * ppl[j][0].grp.i, 255);
		auto g = std::min(((c0 >> 8) & 0xFF) * ppl[j][1].p.x, 255);
		auto b = std::min((c0 & 0xFF) * ppl[j][1].p.y, 255);
		auto a = std::min(m_nAlpha * ppl[j][1].p.z, 255);

		pVertex->color = (a << 24) | (r << 16) | (g << 8) | b;
		pVertex->specular = m_dcFogSpecular;
		pVertex->tu = ppl[j][0].grp.u;
		pVertex->tv = ppl[j][0].grp.v;

		auto _sx = ppl[j][0].grp.sx + fix_from_float(0.5);
		auto _sy = ppl[j][0].grp.sy + fix_from_float(0.5);
		_sx = std::clamp(_sx, grd_canvas->gc.clip.f.left, grd_canvas->gc.clip.f.right);
		_sy = std::clamp(_sy, grd_canvas->gc.clip.f.top, grd_canvas->gc.clip.f.bot);
		pVertex->sx = fix_float(_sx) + g_XOffset;
		pVertex->sy = fix_float(_sy) + g_YOffset;

		if (zlinear)
		{
			pVertex->sz = z2d;
		}
		else if (lgd3d_z_normal)
		{
			pVertex->sz = ppl[j][0].p.z * inv_z_far;
		}
		else
		{
			pVertex->sz = std::clamp(z1 - ppl[j][0].grp.w * z2, 0.0, 1.0);
			pVertex->rhw = ppl[j][0].grp.w;
		}
	}

	DrawIndPolies();

	return FALSE;
}

BOOL cD6Primitives::RGBAFogLitTrifanInd(int n, r3s_point **ppl, r3ixs_info *psIndInfo)
{
	FlushIfNoFit(n, FALSE);

	for (int j = 0; j < n; ++j)
	{
		auto *pVertex = GetIndPolySlot(n, psIndInfo++);
		if (pVertex == nullptr)
			continue;

		pVertex->tu = ppl[j][0].grp.u;
		pVertex->tv = ppl[j][0].grp.v;
		auto r = std::min(255 * ppl[j][0].grp.i, 255);
		auto g = std::min(255 * ppl[j][1].p.x, 255);
		auto b = std::min(255 * ppl[j][1].p.y, 255);
		auto a = std::min(255 * ppl[j][1].p.z, 255);
		auto f = std::min((1.0 - ppl[j][1].ccodes) * 255, 255);

		pVertex->color = (a << 24) | (r << 16) | (g << 8) | b;
		pVertex->specular = f << 24;

		auto _sx = ppl[j][0].grp.sx + fix_from_float(0.5);
		auto _sy = ppl[j][0].grp.sy + fix_from_float(0.5);
		_sx = std::clamp(_sx, grd_canvas->gc.clip.f.left, grd_canvas->gc.clip.f.right);
		_sy = std::clamp(_sy, grd_canvas->gc.clip.f.top, grd_canvas->gc.clip.f.bot);
		pVertex->sx = fix_float(_sx) + g_XOffset;
		pVertex->sy = fix_float(_sy) + g_YOffset;

		if (zlinear)
		{
			pVertex->sz = z2d;
		}
		else if (lgd3d_z_normal)
		{
			pVertex->sz = ppl[j][0].p.z * inv_z_far;
		}
		else
		{
			pVertex->sz = std::clamp(z1 - ppl[j][0].grp.w * z2, 0.0, 1.0);
			pVertex->rhw = ppl[j][0].grp.w;
		}
	}

	DrawIndPolies();

	return FALSE;
}

BOOL cD6Primitives::DiffuseSpecularLitTrifanInd(int n, r3s_point **ppl, r3ixs_info *psIndInfo)
{
	FlushIfNoFit(n, FALSE);

	for (int j = 0; j < n; ++j)
	{
		auto *pVertex = GetIndPolySlot(n, psIndInfo++);
		if (pVertex == nullptr)
			continue;

		auto r = std::min(255 * ppl[j][0].grp.i, 255);
		auto g = std::min(255 * ppl[j][1].p.x, 255);
		auto b = std::min(255 * ppl[j][1].p.y, 255);
		auto a = std::min(255 * ppl[j][1].p.z, 255);

		auto aa = std::min(255 * ppl[j][1].grp.sx, 255);
		auto ra = std::min(255 * ppl[j][1].grp.sy, 255);
		auto ga = std::min(255 * ppl[j][1].grp.w, 255);
		auto ba = std::min(255 * ppl[j][1].grp.flags, 255);

		pVertex->color = (a << 24) | (r << 16) | (g << 8) | b;
		pVertex->specular = (aa << 24) | (ra << 16) | (ga << 8) | ba;
		pVertex->tu = ppl[j][0].grp.u;
		pVertex->tv = ppl[j][0].grp.v;

		auto _sx = ppl[j][0].grp.sx + fix_from_float(0.5);
		auto _sy = ppl[j][0].grp.sy + fix_from_float(0.5);
		_sx = std::clamp(_sx, grd_canvas->gc.clip.f.left, grd_canvas->gc.clip.f.right);
		_sy = std::clamp(_sy, grd_canvas->gc.clip.f.top, grd_canvas->gc.clip.f.bot);
		pVertex->sx = fix_float(_sx) + g_XOffset;
		pVertex->sy = fix_float(_sy) + g_YOffset;

		if (zlinear)
		{
			pVertex->sz = z2d;
		}
		else if (lgd3d_z_normal)
		{
			pVertex->sz = ppl[j][0].p.z * inv_z_far;
		}
		else
		{
			pVertex->sz = std::clamp(z1 - ppl[j][0].grp.w * z2, 0.0, 1.0);
			pVertex->rhw = ppl[j][0].grp.w;
		}
	}

	DrawIndPolies();

	return FALSE;
}

BOOL cD6Primitives::TrifanMTD(int n, r3s_point **ppl, LGD3D_tex_coord **pptc)
{
	return Trifan(n, ppl);
}

BOOL cD6Primitives::LitTrifanMTD(int n, r3s_point **ppl, LGD3D_tex_coord **pptc)
{
	return LitTrifan(n, ppl);
}

BOOL cD6Primitives::RGBlitTrifanMTD(int n, r3s_point **ppl, LGD3D_tex_coord **pptc)
{
	return RGBlitTrifan(n, ppl);
}

BOOL cD6Primitives::RGBAlitTrifanMTD(int n, r3s_point **ppl, LGD3D_tex_coord **pptc)
{
	return RGBAlitTrifan(n, ppl);
}

BOOL cD6Primitives::RGBAFogLitTrifanMTD(int n, r3s_point **ppl, LGD3D_tex_coord **pptc)
{
	return RGBAFogLitTrifan(n, ppl);
}

BOOL cD6Primitives::DiffuseSpecularLitTrifanMTD(int n, r3s_point **ppl, LGD3D_tex_coord **pptc)
{
	return DiffuseSpecularLitTrifan(n, ppl);
}

BOOL cD6Primitives::g2UTrifanMTD(int n, g2s_point **vpl, LGD3D_tex_coord **pptc)
{
	return g2UTrifan(n, vpl);
}

BOOL cD6Primitives::g2TrifanMTD(int n, g2s_point **vpl, LGD3D_tex_coord **pptc)
{
	return g2Trifan(n, vpl);
}

cMSBuffer::cMSBuffer()
{
	m_dwMaxNoMTVertices = 50;
}

cMSBuffer::~cMSBuffer()
{
}

cD6Primitives *cMSBuffer::Instance()
{
	if (!m_Instance)
	{
		m_Instance = new cMSBuffer();
	}

	return m_Instance;
}

void cMSBuffer::DrawPoly(BOOL bSuspendTexturing)
{
	if (m_ePolyMode & ePolyMode::kLgd3dPolyModeFillWColor)
		bSuspendTexturing = TRUE;

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
		if (m_ePolyMode & (ePolyMode::kLgd3dPolyModeFillWTexture || ePolyMode::kLgd3dPolyModeFillWColor))
		{
			auto hResult = g_lpD3Ddevice->DrawPrimitive(
				D3DPT_TRIANGLEFAN,
				D3DFVF_TLVERTEX,
				m_saVertexBuffer,
				m_dwNoCashedVertices,
				D3DDP_DONOTCLIP | D3DDP_DONOTUPDATEEXTENTS);

			AssertMsg3(SUCCEEDED(hResult), "%s: error %d\n%s ", "DrawPrimitive(poly) failed", hResult, GetDDErrorMsg(hResult));
		}

		if (m_ePolyMode & ePolyMode::kLgd3dPolyModeDrawEdges)
			DrawStandardEdges(m_saVertexBuffer, m_dwNoCashedVertices);
	}
}

MTVERTEX *cMSBuffer::ReserveMTPolySlots(int n)
{
	if (n > m_dwMaxNoMTVertices)
		CriticalMsg("ReservePolySlots(): poly too large!");

	m_dwNoMTVertices = n;

	return m_saMTVertices;
}
