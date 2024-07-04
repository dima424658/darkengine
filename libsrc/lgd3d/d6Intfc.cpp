#include <utility>
#include <algorithm>

#define D3D_OVERLOADS
#include <ddraw.h>
#include <d3dtypes.h>

#include <types.h>
#include <grs.h>
#include <r3ds.h>
#include <g2spoint.h>

#include <dbg.h>
#include <lgd3d.h>
#include <lgassert.h>

#include <d6Frame.h>
#include <d6Render.h>
#include <d6Prim.h>
#include <d6States.h>

BOOL lgd3d_g_bInitialized = FALSE;
BOOL g_bPrefer_RGB;
cD6Frame* pcFrame = nullptr;

extern cD6Renderer* pcRenderer;
extern cD6Primitives* pcRenderBuffer;
extern DDPIXELFORMAT g_TransRGBTextureFormat, g_PalTextureFormat, g_AlphaTextureFormat, g_8888TexFormat, g_RGBTextureFormat;
extern float g_XOffset, g_YOffset;

BOOL lgd3d_z_normal = TRUE;
double z_near = 1.0;
double z_far = 200.0;
// near / far
double inv_z_far = 1 / 200;
// far / (far - near)
double z1 = 200.0 / (200.0 - 1.0);
// near * far / (far - near)
double z2 = 1.0 * 200.0 / (200.0 - 1.0);
extern "C" {
double z2d = 1.0, w2d = 1.0;
}
double zbias = 0.0;

double drZBiasStock[LGD3D_ZBIAS_STACK_DEPTH] = {};
size_t dwZBiasStackPtr = 0;
long double drZBiasTable[] =
{
  0.0,
  1 / 32786,
  1 / 16384,
  1 / 8192,
  1 / 4096,
  1 / 2048,
  1 / 1024,
  1 / 512,
  1 / 256,
  1 / 128,
  1 / 64,
  1 / 32,
  1 / 16,
  1 / 8,
  1 / 4,
  1 / 2,
  1.0 // unused
};

static void set1(D3DMATRIX* m)
{
	*m = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
}

static void setwbnf(IDirect3DDevice3* lpDev, double dvWNear, double dvWFar)
{
	// WVP
	D3DMATRIX matWorld, matView, matProj;

	set1(&matWorld);
	set1(&matView);
	set1(&matProj);
	if (dvWFar > dvWNear)
	{
		lpDev->SetTransform(D3DTRANSFORMSTATE_WORLD, &matWorld);
		lpDev->SetTransform(D3DTRANSFORMSTATE_VIEW, &matView);
		// Generate left-handed perspective projection
		float Q = dvWFar / (dvWFar - dvWNear);
		matProj(2, 2) = Q;
		matProj(3, 2) = -dvWNear * Q;
		matProj(2, 3) = 1.0;
		matProj(3, 3) = 0.0;
		lpDev->SetTransform(D3DTRANSFORMSTATE_PROJECTION, &matProj);
	}
}

void lgd3d_set_RGB()
{
	// empty
}

void lgd3d_set_hardware()
{
	// empty
}

void lgd3d_set_software()
{
	// empty
}

void lgd3d_texture_set_RGB(BOOL is_RGB)
{
	g_bPrefer_RGB = is_RGB;
}


BOOL lgd3d_is_RGB()
{
	return TRUE;
}

BOOL lgd3d_is_hardware()
{
	return TRUE;
}


BOOL lgd3d_init(lgd3ds_device_info* device_info)
{
	if (pcFrame)
	{
		Warning(("lgd3d_init():: library already initialized!\n"));
		AssertMsg(lgd3d_g_bInitialized == TRUE, "Missuse of the variable\"lgd3d_g_bInitialized\"\n");
		return TRUE;
	}

	lgd3d_g_bInitialized = TRUE;
	pcFrame = new cD6Frame{ static_cast<ushort>(grd_cap->w), static_cast<ushort>(grd_cap->h), device_info };
	if (!lgd3d_g_bInitialized)
		lgd3d_shutdown();

	return lgd3d_g_bInitialized;
}

BOOL lgd3d_attach_to_lgsurface(ILGSurface* pILGSurface)
{
	if (!pILGSurface)
	{
		Warning(("lgd3d_attach_to_lgsurface::called with NULL"));
		return FALSE;
	}

	if (lgd3d_g_bInitialized)
		lgd3d_shutdown();

	lgd3d_g_bInitialized = TRUE;
	pcFrame = new cD6Frame{ pILGSurface };
	if (!lgd3d_g_bInitialized)
		lgd3d_shutdown();

	return lgd3d_g_bInitialized;
}

void lgd3d_clean_render_surface(BOOL bDepthBuffToo)
{
	pcRenderer->CleanRenderSurface(bDepthBuffToo);
}


void lgd3d_shutdown()
{
	if (pcFrame)
		delete pcFrame;

	pcFrame = nullptr;
	lgd3d_g_bInitialized = FALSE;
}


void lgd3d_start_frame(int frame)
{
	if (!pcFrame)
		return;

	pcRenderer->StartFrame(frame);
}

void lgd3d_end_frame()
{
	if (!pcFrame)
		return;

	pcRenderer->EndFrame();
}


BOOL lgd3d_overlays_master_switch(BOOL bOverlaysOn)
{
	return pcRenderer->SwitchOverlaysOnOff(bOverlaysOn);
}


void lgd3d_blit()
{
	// empty
}

void lgd3d_clear(int color_index)
{
	if (!pcFrame)
		return;

	pcRenderBuffer->Clear(color_index);
}


void lgd3d_set_zwrite(int zwrite)
{
	if (!pcFrame)
		return;

	pcStates->SetZWrite(zwrite);
}

void lgd3d_set_zcompare(int zwrite)
{
	if (!pcFrame)
		return;

	pcStates->SetZCompare(zwrite);
}

void lgd3d_zclear()
{
	Warning(("zclear not yet implemented\n"));
}

void lgd3d_set_znearfar(double znear, double zfar)
{
	z_near = znear;
	z_far = zfar;
	inv_z_far = 1.0 / zfar;
	z1 = zfar / (zfar - znear);
	z2 = znear * z1;
	z1 = z1 - zbias;

	if (pcStates)
		setwbnf(g_lpD3Ddevice, znear, zfar);
}

void lgd3d_get_znearfar(double* pdZNear, double* pdZFar)
{
	if (pdZNear)
		*pdZNear = z_near;

	if (pdZFar)
		*pdZFar = z_far;
}


void lgd3d_clear_z_rect(int x0, int y0, int x1, int y1)
{
	if (!pcFrame)
		return;

	pcRenderer->CleanDepthBuffer(x0, y0, x1, y1);
}

void lgd3d_set_z(float z)
{
	if (lgd3d_z_normal)
	{
		z2d = z;
		return;
	}

	z2d = std::clamp(z1 - z2 / z, 0.0, 1.0);
	w2d = 1.0 / z;
}

int /* D3DZBUFFERTYPE? */ lgd3d_get_depth_buffer_state()
{
	if (!pcFrame)
		return -1;

	return pcStates->GetDepthBufferState();
}

BOOL lgd3d_is_zwrite_on()
{
	if (!pcFrame)
		return -1;

	return pcStates->IsZWriteOn();
}

BOOL lgd3d_is_zcompare_on()
{
	if (!pcFrame)
		return -1;

	return pcStates->IsZCompareOn();
}


double lgd3d_set_zbias(double new_bias)
{
	auto old_bias = zbias;
	z1 = zbias - new_bias + z1;
	zbias = new_bias;
	return old_bias;
}

void lgd3d_push_zbias_i(int nZBias)
{
	AssertMsg(dwZBiasStackPtr < LGD3D_ZBIAS_STACK_DEPTH - 1, "Z-Bias stack overflow!"); // FIXME: dwZBiasStackPtr < LGD3D_ZBIAS_STACK_DEPTH
	drZBiasStock[dwZBiasStackPtr++] = zbias;

	AssertMsg1(nZBias <= 16, "Z-Bias value %i is out of the [0,16] range", nZBias);
	lgd3d_set_zbias(drZBiasTable[nZBias]);
}

void lgd3d_pop_zbias()
{
	AssertMsg(dwZBiasStackPtr > 0, "Z-Bias stack underflow!");
	lgd3d_set_zbias(drZBiasStock[dwZBiasStackPtr--]);
}


BOOL lgd3d_set_shading(BOOL bSmoothShading)
{
	if (!pcFrame)
		return -1;

	return pcStates->SetSmoothShading(bSmoothShading);
}

BOOL lgd3d_is_smooth_shading_on()
{
	if (!pcFrame)
		return -1;

	return pcStates->IsSmoothShadingOn();
}


BOOL lgd3d_is_alpha_blending_on()
{
	if (!pcFrame)
		return -1;

	return pcStates->IsAlphaBlendingOn();
}


void lgd3d_set_alpha(float alpha)
{
	if (!pcFrame)
		return;

	pcStates->SetAlphaColor(alpha);
}

void lgd3d_set_blend(BOOL do_blend)
{
	if (!pcFrame)
		return;

	pcStates->EnableAlphaBlending(do_blend);
}

void lgd3d_blend_normal()
{
	if (!pcFrame)
		return;

	pcStates->ResetDefaultAlphaModulate();
}


void lgd3d_blend_multiply(int blend_mode)
{
	if (!pcFrame)
		return;

	pcStates->SetAlphaModulateHack(blend_mode);
}


void lgd3d_set_chromakey(int red, int green, int blue)
{
	if (!pcFrame)
		return;

	pcStates->SetChromaKey(red, green, blue);
}


void lgd3d_set_dithering(BOOL bOn)
{
	if (!pcFrame)
		return;

	pcStates->SetDithering(bOn);
}

BOOL lgd3d_is_dithering_on()
{
	// FIXME: pcStates check?
	return pcStates->GetDitheringState();
}


void lgd3d_set_antialiasing(BOOL bOn)
{
	if (!pcFrame)
		return;

	pcStates->SetAntialiasing(bOn);
}

BOOL lgd3d_is_antialiasing_on()
{
	// FIXME: pcStates check?
	return pcStates->GetAntialiasingState();
}


BOOL lgd3d_enable_specular(BOOL bUseIt)
{
	// FIXME: pcStates check?
	return pcStates->EnableSpecular(bUseIt);
}


void lgd3d_set_fog_level(float fog_level)
{
	if (!pcFrame)
		return;

	pcStates->SetFogSpecularLevel(fog_level);
}

void lgd3d_set_fog_color(int r, int g, int b)
{
	if (!pcFrame)
		return;

	pcStates->SetFogColor(r, g, b);
}

void lgd3d_set_fog_enable(BOOL enable)
{
	if (!pcFrame)
		return;

	pcStates->EnableFog(enable);
}

void lgd3d_set_fog_density(float density)
{
	if (!pcFrame)
		return;

	pcStates->SetFogDensity(density);
}

BOOL lgd3d_is_fog_on()
{
	if (!pcFrame)
		return -1;

	return pcStates->IsFogOn();
}

int lgd3d_use_linear_table_fog(BOOL bUseIt)
{
	// FIXME: pcStates check?
	return pcStates->UseLinearTableFog(bUseIt);
}

void lgd3d_set_fog_start_end(float fStart, float fEnd)
{
	if (!pcFrame)
		return;

	pcStates->SetFogStartAndEnd(fStart, fEnd);
}

void lgd3d_set_linear_fog_distance(float fDistance)
{
	if (!pcFrame)
		return;

	pcStates->SetLinearFogDistance(fDistance);
}


void lgd3d_set_texture_level(int n)
{
	if (!pcFrame)
		return;

	pcStates->SetTextureLevel(n);
}

void lgd3d_get_texblending_modes(ulong* pulLevel0Mode, ulong* pulLevel1Mode)
{
	if (!pcFrame)
		return;

	pcStates->GetTexBlendingModes(pulLevel0Mode, pulLevel1Mode);
}


void lgd3d_hack_light(r3s_point* p, float r)
{
	if (!pcFrame)
		return;

	pcRenderBuffer->HackLight(p, r);
}

void lgd3d_hack_light_extra(r3s_point* p, float r, grs_bitmap* bm)
{
	if (!pcFrame)
		return;

	pcRenderBuffer->HackLightExtra(p, r, bm);
}


BOOL lgd3d_get_texture_wrapping(DWORD dwLevel)
{
	if (!pcFrame)
		return -1;

	return pcStates->GetTexWrapping(dwLevel);
}

BOOL lgd3d_set_texture_wrapping(DWORD dwLevel, BOOL bSetSmooth)
{
	if (!pcFrame)
		return -1;

	return pcStates->SetTexWrapping(dwLevel, bSetSmooth);
}


void lgd3d_set_pal(uint start, uint n, uchar* pal)
{
	lgd3d_set_pal_slot(start, n, pal, 0);
}

void lgd3d_set_pal_slot(uint start, uint n, uchar* pal, int slot)
{
	lgd3d_set_pal_slot_flags(start, n, pal, slot, 3);
}

void lgd3d_set_pal_slot_flags(uint start, uint n, uchar* pal, int slot, int flags)
{
	if (!pcFrame)
		return;

	pcStates->SetPalSlotFlags(start, n, pal, slot, flags);
}


void lgd3d_get_trans_texture_bitmask(grs_rgb_bitmask* bitmask)
{
	if (!g_TransRGBTextureFormat.dwFlags)
	{
		g_TransRGBTextureFormat.dwBBitMask = 0;
		g_TransRGBTextureFormat.dwGBitMask = 0;
		g_TransRGBTextureFormat.dwRBitMask = 0;
		g_TransRGBTextureFormat.dwRGBAlphaBitMask = 0;
	}

	bitmask->alpha = g_TransRGBTextureFormat.dwRGBAlphaBitMask;
	bitmask->red = g_TransRGBTextureFormat.dwRBitMask;
	bitmask->green = g_TransRGBTextureFormat.dwGBitMask;
	bitmask->blue = g_TransRGBTextureFormat.dwBBitMask;
}

void lgd3d_get_opaque_texture_bitmask(grs_rgb_bitmask* bitmask)
{
	if (!g_RGBTextureFormat.dwFlags)
	{
		g_RGBTextureFormat.dwBBitMask = 0;
		g_RGBTextureFormat.dwGBitMask = 0;
		g_RGBTextureFormat.dwRBitMask = 0;
		g_RGBTextureFormat.dwRGBAlphaBitMask = 0;
	}

	bitmask->alpha = g_RGBTextureFormat.dwRGBAlphaBitMask;
	bitmask->red = g_RGBTextureFormat.dwRBitMask;
	bitmask->green = g_RGBTextureFormat.dwGBitMask;
	bitmask->blue = g_RGBTextureFormat.dwBBitMask;
}

void lgd3d_get_alpha_texture_bitmask(grs_rgb_bitmask* bitmask)
{
	if (!g_AlphaTextureFormat.dwFlags)
	{
		g_AlphaTextureFormat.dwBBitMask = 0;
		g_AlphaTextureFormat.dwGBitMask = 0;
		g_AlphaTextureFormat.dwRBitMask = 0;
		g_AlphaTextureFormat.dwRGBAlphaBitMask = 0;
	}

	bitmask->alpha = g_AlphaTextureFormat.dwRGBAlphaBitMask;
	bitmask->red = g_AlphaTextureFormat.dwRBitMask;
	bitmask->green = g_AlphaTextureFormat.dwGBitMask;
	bitmask->blue = g_AlphaTextureFormat.dwBBitMask;
}


void lgd3d_set_texture_clut(uchar* clut)
{
	texture_clut = clut;
	if (!g_tmgr)
		return;

	g_tmgr->set_clut(clut);
}

uchar* lgd3d_set_clut(uchar* clut)
{
	auto* old_clut = std::exchange(lgd3d_clut, clut);
	lgd3d_set_texture_clut(clut);

	return old_clut;
}


void lgd3d_disable_palette()
{
	if (!pcFrame)
		return;

	pcStates->EnablePalette(FALSE);
}

void lgd3d_enable_palette()
{
	if (!pcFrame)
		return;

	pcStates->EnablePalette(TRUE);
}


void lgd3d_set_alpha_pal(ushort* pal)
{
	if (!pcFrame)
		return;

	pcStates->SetAlphaPalette(pal);
}


void lgd3d_set_offsets(int x, int y)
{
	g_XOffset = x;
	g_YOffset = y;
}

void lgd3d_get_offsets(int* x, int* y)
{
	if (x)
		*x = g_XOffset;

	if (y)
		*y = g_YOffset;
}


int lgd3d_draw_point(r3s_point* p)
{
	return pcRenderBuffer->DrawPoint(p);
}

void lgd3d_draw_line(r3s_point* p0, r3s_point* p1)
{
	if (!pcFrame)
		return;

	pcRenderBuffer->DrawLine(p0, p1);
}


BOOL lgd3d_set_poly_mode(ePolyMode eNewMode)
{
	if (!pcFrame)
		return FALSE;

	return pcRenderBuffer->SetPolyMode(eNewMode);
}

ePolyMode lgd3d_get_poly_mode()
{
	if (!pcFrame)
		return ePolyMode::kLgd3dILLEGALPolyMode;

	return pcRenderBuffer->GetPolyMode();
}


int lgd3d_g2upoly(int n, g2s_point** vpl)
{
	return pcRenderBuffer->g2UPoly(n, vpl);
}

int lgd3d_g2poly(int n, g2s_point** vpl)
{
	return pcRenderBuffer->g2Poly(n, vpl);
}

int lgd3d_g2utrifan(int n, g2s_point** vpl)
{
	return pcRenderBuffer->g2UTrifan(n, vpl);
}

int lgd3d_g2trifan(int n, g2s_point** vpl)
{
	return pcRenderBuffer->g2Trifan(n, vpl);
}


tLgd3dDrawPolyFunc lgd3d_draw_poly_func = nullptr;
tLgd3dDrawPolyIndexedFunc lgd3d_draw_poly_indexed_func = nullptr;

void lgd3d_tmap_setup(grs_bitmap* bm)
{
	if (!pcFrame)
		return;

	lgd3d_set_texture(bm);
	lgd3d_draw_poly_func = lgd3d_trifan;
	lgd3d_draw_poly_indexed_func = lgd3d_indexed_trifan;

}

void lgd3d_lit_tmap_setup(grs_bitmap* bm)
{
	if (!pcFrame)
		return;

	lgd3d_set_texture(bm);
	lgd3d_draw_poly_func = lgd3d_lit_trifan;
	lgd3d_draw_poly_indexed_func = lgd3d_lit_indexed_trifan;
}

void lgd3d_rgblit_tmap_setup(grs_bitmap* bm)
{
	if (!pcFrame)
		return;

	lgd3d_set_texture(bm);
	lgd3d_draw_poly_func = lgd3d_rgblit_trifan;
	lgd3d_draw_poly_indexed_func = lgd3d_rgblit_indexed_trifan;
}

void lgd3d_rgbalit_tmap_setup(grs_bitmap* bm)
{
	if (!pcFrame)
		return;

	lgd3d_set_texture(bm);
	lgd3d_draw_poly_func = lgd3d_rgbalit_trifan;
	lgd3d_draw_poly_indexed_func = lgd3d_rgbalit_indexed_trifan;
}

void lgd3d_rgbafoglit_tmap_setup(grs_bitmap* bm)
{
	if (!pcFrame)
		return;

	lgd3d_set_texture(bm);
	lgd3d_draw_poly_func = lgd3d_rgbafoglit_trifan;
	lgd3d_draw_poly_indexed_func = lgd3d_rgbafoglit_indexed_trifan;
}

void lgd3d_diffspecular_tmap_setup(grs_bitmap* bm)
{
	if (!pcFrame)
		return;

	lgd3d_set_texture(bm);
	lgd3d_draw_poly_func = lgd3d_diffspecular_trifan;
	lgd3d_draw_poly_indexed_func = lgd3d_diffspecular_indexed_trifan;
}

void lgd3d_poly_setup()
{
	if (!pcFrame)
		return;

	pcStates->set_texture_id(-1);
	lgd3d_draw_poly_func = lgd3d_poly;
	lgd3d_draw_poly_indexed_func = lgd3d_indexed_poly;
}

void lgd3d_spoly_setup()
{
	if (!pcFrame)
		return;

	pcStates->set_texture_id(-1);
	lgd3d_draw_poly_func = lgd3d_spoly;
	lgd3d_draw_poly_indexed_func = lgd3d_indexed_spoly;
}

void lgd3d_rgb_poly_setup()
{
	if (!pcFrame)
		return;

	pcStates->set_texture_id(-1);
	lgd3d_draw_poly_func = lgd3d_rgb_poly;
	lgd3d_draw_poly_indexed_func = lgd3d_rgb_indexed_poly;
}

void lgd3d_rgba_poly_setup()
{
	if (!pcFrame)
		return;

	pcStates->set_texture_id(-1);
	lgd3d_draw_poly_func = lgd3d_rgba_poly;
	lgd3d_draw_poly_indexed_func = lgd3d_rgba_indexed_poly;
}


int lgd3d_poly(int n, r3s_point** vpl)
{
	return pcRenderBuffer->Poly(n, vpl);
}

int lgd3d_indexed_poly(int n, r3s_point** vpl, r3ixs_info* info)
{
	return pcRenderBuffer->PolyInd(n, vpl, info);
}

int lgd3d_spoly(int n, r3s_point** vpl)
{
	return pcRenderBuffer->SPoly(n, vpl);
}

int lgd3d_indexed_spoly(int n, r3s_point** vpl, r3ixs_info* info)
{
	return pcRenderBuffer->SPolyInd(n, vpl, info);
}

int lgd3d_rgb_poly(int n, r3s_point** ppl)
{
	return pcRenderBuffer->RGB_Poly(n, ppl);
}

int lgd3d_rgb_indexed_poly(int n, r3s_point** ppl, r3ixs_info* info)
{
	return pcRenderBuffer->RGB_PolyInd(n, ppl, info);
}

int lgd3d_rgba_poly(int n, r3s_point** ppl)
{
	return pcRenderBuffer->RGBA_Poly(n, ppl);
}

int lgd3d_rgba_indexed_poly(int n, r3s_point** ppl, r3ixs_info* info)
{
	return pcRenderBuffer->RGBA_PolyInd(n, ppl, info);
}

int lgd3d_trifan(int n, r3s_point** vpl)
{
	return pcRenderBuffer->Trifan(n, vpl);
}

int lgd3d_indexed_trifan(int n, r3s_point** vpl, r3ixs_info* info)
{
	return pcRenderBuffer->TrifanInd(n, vpl, info);
}

int lgd3d_lit_trifan(int n, r3s_point** vpl)
{
	return pcRenderBuffer->LitTrifan(n, vpl);
}

int lgd3d_lit_indexed_trifan(int n, r3s_point** vpl, r3ixs_info* info)
{
	return pcRenderBuffer->LitTrifanInd(n, vpl, info);
}

int lgd3d_rgblit_trifan(int n, r3s_point** ppl)
{
	return pcRenderBuffer->RGBlitTrifan(n, ppl);
}

int lgd3d_rgblit_indexed_trifan(int n, r3s_point** ppl, r3ixs_info* info)
{
	return pcRenderBuffer->RGBlitTrifanInd(n, ppl, info);
}

int lgd3d_rgbalit_trifan(int n, r3s_point** ppl)
{
	return pcRenderBuffer->RGBAlitTrifan(n, ppl);
}

int lgd3d_rgbalit_indexed_trifan(int n, r3s_point** ppl, r3ixs_info* info)
{
	return pcRenderBuffer->RGBAlitTrifanInd(n, ppl, info);
}

int lgd3d_rgbafoglit_trifan(int n, r3s_point** ppl)
{
	return pcRenderBuffer->RGBAFogLitTrifan(n, ppl);
}


int lgd3d_rgbafoglit_indexed_trifan(int n, r3s_point** ppl, r3ixs_info* info)
{
	return pcRenderBuffer->RGBAFogLitTrifanInd(n, ppl, info);
}

int lgd3d_diffspecular_trifan(int n, r3s_point** ppl)
{
	return pcRenderBuffer->DiffuseSpecularLitTrifan(n, ppl);
}


int lgd3d_diffspecular_indexed_trifan(int n, r3s_point** ppl, r3ixs_info* info)
{
	return pcRenderBuffer->DiffuseSpecularLitTrifanInd(n, ppl, info);
}


void lgd3d_release_indexed_primitives()
{
	pcRenderBuffer->EndIndexedRun();
}


void lgd3d_set_texture_map_method(ulong flag)
{
	pcStates->SetTextureMapMode(flag);
}

void lgd3d_set_light_map_method(ulong flag)
{
	pcStates->SetLightMapMode(flag);
}


int lgd3d_TrifanMTD(int n, r3s_point** ppl, LGD3D_tex_coord** pptc)
{
	return pcRenderBuffer->TrifanMTD(n, ppl, pptc);
}

int lgd3d_LitTrifanMTD(int n, r3s_point** ppl, LGD3D_tex_coord** pptc)
{
	return pcRenderBuffer->LitTrifanMTD(n, ppl, pptc);
}

int lgd3d_RGBlitTrifanMTD(int n, r3s_point** ppl, LGD3D_tex_coord** pptc)
{
	return pcRenderBuffer->RGBlitTrifanMTD(n, ppl, pptc);
}

int lgd3d_RGBAlitTrifanMTD(int n, r3s_point** ppl, LGD3D_tex_coord** pptc)
{
	return pcRenderBuffer->RGBAlitTrifanMTD(n, ppl, pptc);
}

int lgd3d_RGBAFoglitTrifanMTD(int n, r3s_point** ppl, LGD3D_tex_coord** pptc)
{
	return pcRenderBuffer->RGBAFogLitTrifanMTD(n, ppl, pptc);
}

int lgd3d_DiffuseSpecularMTD(int n, r3s_point** ppl, LGD3D_tex_coord** pptc)
{
	return pcRenderBuffer->DiffuseSpecularLitTrifanMTD(n, ppl, pptc);
}

int lgd3d_g2UTrifanMTD(int n, g2s_point** vpl, LGD3D_tex_coord** vptc)
{
	return pcRenderBuffer->g2UTrifanMTD(n, vpl, vptc);
}

int lgd3d_g2TrifanMTD(int n, g2s_point** vpl, LGD3D_tex_coord** vptc)
{
	return pcRenderBuffer->g2TrifanMTD(n, vpl, vptc);
}
