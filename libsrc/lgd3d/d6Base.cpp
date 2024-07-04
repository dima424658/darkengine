
#include <ddraw.h>
#include <d3d.h>

#include <lgd3d.h>
#include <lgassert.h>


static DWORD dwErrorCode;
static int hD3DError;

BOOL lgd3d_get_error(DWORD* pdwCode, DWORD* phResult)
{
	if (!dwErrorCode)
		return FALSE;

	*pdwCode = dwErrorCode;
	*phResult = hD3DError;

	return TRUE;
}

void SetLGD3DErrorCode(DWORD dwCode, DWORD hResult)
{
	dwErrorCode = dwCode;
	hD3DError = hResult;
}

const char* GetLgd3dErrorCode(DWORD dwErrorCode)
{
	Assert_(dwErrorCode != LGD3D_EC_OK);

	switch (dwErrorCode)
	{
	case LGD3D_EC_DD_KAPUT:
		return "Could not obtain DirectDraw from display";
	case LGD3D_EC_RESTORE_ALL_SURFS:
		return "Could not restore all surfaces";
	case LGD3D_EC_QUERY_D3D:
		return "QueryInterface for D3D failed";
	case LGD3D_EC_GET_DD_CAPS:
		return " GetCaps for DirectDraw failed";
	case LGD3D_EC_ZBUFF_ENUMERATION:
		return " Could not enumerate Zbuffer formats";
	case LGD3D_EC_CREATE_3D_DEVICE:
		return "Create D3D device failed";
	case LGD3D_EC_CREATE_VIEWPORT:
		return "Creation of the viewport failed";
	case LGD3D_EC_ADD_VIEWPORT:
		return " Addition of the viewport failed";
	case LGD3D_EC_SET_VIEWPORT:
		return "Setting of the viewport2 failed";
	case LGD3D_EC_SET_CURR_VP:
		return "Setting of current viewport failed";
	case LGD3D_EC_CREATE_BK_MATERIAL:
		return "Creation of the background material failed";
	case LGD3D_EC_SET_BK_MATERIAL:
		return "Setting of the background material failed";
	case LGD3D_EC_GET_BK_MAT_HANDLE:
		return "Could not obtain a material handle";
	case LGD3D_EC_GET_SURF_DESC:
		return "Could not get surface description";
	case LGD3D_EC_GET_3D_CAPS:
		return "Could not get D3D device caps";
	case LGD3D_EC_VD_MPASS_MT:
		return "ValidateDevice for multipass lightmaps failed";
	case LGD3D_EC_VD_S_DEFAULT:
		return "ValidateDevice for simple states failed";
	case LGD3D_EC_VD_SPASS_MT:
		return "ValidateDevice for single pass lightmaps failed";
	case LGD3D_EC_VD_M_DEFAULT:
		return "ValidateDevice for multitexture states failed";
	case LGD3D_EC_VD_SPASS_BLENDDIFFUSE: // maybe unused
		return "ValidateDevice for single pass blend diffuse failed";
	case LGD3D_EC_VD_MPASS_BLENDDIFFUSE: // maybe unused
		return "ValidateDevice for multipass blend diffuse failed";
	default:
		return "Unknown error";
	}
}

const char* GetDDErrorMsg(long hRes)
{
	switch (hRes)
	{
	case DD_OK:
		return "I am OK";
	case DDERR_ALREADYINITIALIZED:
		return "This object is already initialized.";
	case DDERR_CANNOTATTACHSURFACE:
		return "This surface can not be attached to the requested surface.";
	case DDERR_CANNOTDETACHSURFACE:
		return "This surface can not be detached from the requested surface."; // new
	case DDERR_CURRENTLYNOTAVAIL:
		return "Support is currently not available.";
	case DDERR_EXCEPTION:
		return "An exception was encountered while performing the requested operation.";
	case DDERR_GENERIC:
		return "Generic failure.";
	case DDERR_HEIGHTALIGN:
		return "Height of rectangle provided is not a multiple of reqd alignment.";
	case DDERR_INCOMPATIBLEPRIMARY:
		return "Unable to match primary surface creation request with existing primary surface.";
	case DDERR_INVALIDCAPS:
		return "One or more of the caps bits passed to the callback are incorrect.";
	case DDERR_INVALIDCLIPLIST:
		return "DirectDraw does not support the provided cliplist.";
	case DDERR_INVALIDMODE:
		return "DirectDraw does not support the requested mode.";
	case DDERR_INVALIDOBJECT:
		return "DirectDraw received a pointer that was an invalid DIRECTDRAW object.";
	case DDERR_INVALIDPARAMS:
		return "One or more of the parameters passed to the function are incorrect.";
	case DDERR_INVALIDPIXELFORMAT:
		return "The pixel format was invalid as specified.";
	case DDERR_INVALIDRECT:
		return "Rectangle provided was invalid.";
	case DDERR_LOCKEDSURFACES:
		return "Operation could not be carried out because one or more surfaces are locked.";
	case DDERR_NO3D:
		return "There is no 3D present.";
	case DDERR_NOALPHAHW:
		return "Operation could not be carried out because there is no alpha accleration hardware present or available.";
	case DDERR_NOSTEREOHARDWARE:
		return "Operation could not be carried out because there is no stereo hardware present or available."; // new
	case DDERR_NOSURFACELEFT:
		return "Operation could not be carried out because there is no hardware present which supports stereo surfaces"; // new
	case DDERR_NOCLIPLIST:
		return "No cliplist available.";
	case DDERR_NOCOLORCONVHW:
		return "Operation could not be carried out because there is no color conversion hardware present or available.";
	case DDERR_NOCOOPERATIVELEVELSET:
		return "Create function called without DirectDraw object method SetCooperativeLevel being called.";
	case DDERR_NOCOLORKEY:
		return "Surface doesn't currently have a color key";
	case DDERR_NOCOLORKEYHW:
		return "Operation could not be carried out because there is no hardware support of the destination color key.";
	case DDERR_NODIRECTDRAWSUPPORT:
		return "No DirectDraw support possible with current display driver.";
	case DDERR_NOEXCLUSIVEMODE:
		return "Operation requires the application to have exclusive mode but the application does not have exclusive mode.";
	case DDERR_NOFLIPHW:
		return "Flipping visible surfaces is not supported.";
	case DDERR_NOGDI:
		return "There is no GDI present.";
	case DDERR_NOMIRRORHW:
		return "Operation could not be carried out because there is no hardware present or available.";
	case DDERR_NOTFOUND:
		return "Requested item was not found.";
	case DDERR_NOOVERLAYHW:
		return "Operation could not be carried out because there is no overlay hardware present or available.";
	case DDERR_OVERLAPPINGRECTS:
		return "Operation could not be carried out because the source and destination rectangles are on the same surface and overlap each other.";
	case DDERR_NORASTEROPHW:
		return "Operation could not be carried out because there is no appropriate raster op hardware present or available.";
	case DDERR_NOROTATIONHW:
		return "Operation could not be carried out because there is no rotation hardware present or available.";
	case DDERR_NOSTRETCHHW:
		return "Operation could not be carried out because there is no hardware support for stretching.";
	case DDERR_NOT4BITCOLOR:
		return "DirectDrawSurface is not in 4 bit color palette and the requested operation requires 4 bit color palette.";
	case DDERR_NOT4BITCOLORINDEX:
		return "DirectDrawSurface is not in 4 bit color index palette and the requested operation requires 4 bit color index palette.";
	case DDERR_NOT8BITCOLOR:
		return "DirectDrawSurface is not in 8 bit color mode and the requested operation requires 8 bit color.";
	case DDERR_NOTEXTUREHW:
		return "Operation could not be carried out because there is no texture mapping hardware present or available.";
	case DDERR_NOVSYNCHW:
		return "Operation could not be carried out because there is no hardware support for vertical blank synchronized operations.";
	case DDERR_NOZBUFFERHW:
		return "Operation could not be carried out because there is no hardware support for zbuffer blitting.";
	case DDERR_NOZOVERLAYHW:
		return "Overlay surfaces could not be z layered based on their BltOrder because the hardware does not support z layering of overlays.";
	case DDERR_OUTOFCAPS:
		return "The hardware needed for the requested operation has already been allocated.";
	case DDERR_OUTOFMEMORY:
		return "DirectDraw does not have enough memory to perform the operation.";
	case DDERR_OUTOFVIDEOMEMORY:
		return "DirectDraw does not have enough video memory to perform the operation.";
	case DDERR_OVERLAYCANTCLIP:
		return "The hardware does not support clipped overlays.";
	case DDERR_OVERLAYCOLORKEYONLYONEACTIVE:
		return "Can only have ony color key active at one time for overlays.";
	case DDERR_PALETTEBUSY:
		return "Access to this palette is being refused because the palette is already locked by another thread.";
	case DDERR_COLORKEYNOTSET:
		return "No src color key specified for this operation.";
	case DDERR_SURFACEALREADYATTACHED:
		return "This surface is already attached to the surface it is being attached to.";
	case DDERR_SURFACEALREADYDEPENDENT:
		return "This surface is already a dependency of the surface it is being made a dependency of.";
	case DDERR_SURFACEBUSY:
		return "Access to this surface is being refused because the surface is already locked by another thread.";
	case DDERR_CANTLOCKSURFACE:
		return "Access to this surface is being refused because no driver exists which can supply a pointer to the surface.";
	case DDERR_SURFACEISOBSCURED:
		return "Access to surface refused because the surface is obscured.";
	case DDERR_SURFACELOST:
		return "Access to this surface is being refused because the surface memory is gone. The DirectDrawSurface object representing this surface should have Restore called on it.";
	case DDERR_SURFACENOTATTACHED:
		return "The requested surface is not attached.";
	case DDERR_TOOBIGHEIGHT:
		return "Height requested by DirectDraw is too large.";
	case DDERR_TOOBIGSIZE:
		return "Size requested by DirectDraw is too large, but the individual height and width are OK.";
	case DDERR_TOOBIGWIDTH:
		return "Width requested by DirectDraw is too large.";
	case DDERR_UNSUPPORTED:
		return "Action not supported.";
	case DDERR_UNSUPPORTEDFORMAT:
		return "FOURCC format requested is unsupported by DirectDraw.";
	case DDERR_UNSUPPORTEDMASK:
		return "Bitmask in the pixel format requested is unsupported by DirectDraw.";
	case DDERR_INVALIDSTREAM:
		return "The specified stream contains invalid data.";
	case DDERR_VERTICALBLANKINPROGRESS:
		return "Vertical blank is in progress.";
	case DDERR_WASSTILLDRAWING:
		return "Informs DirectDraw that the previous Blt which is transfering information to or from this Surface is incomplete.";
	case DDERR_DDSCAPSCOMPLEXREQUIRED:
		return "The specified surface type requires specification of the COMPLEX flag"; // new
	case DDERR_XALIGN:
		return "Rectangle provided was not horizontally aligned on required boundary.";
	case DDERR_INVALIDDIRECTDRAWGUID:
		return "The GUID passed to DirectDrawCreate is not a valid DirectDraw driver identifier.";
	case DDERR_DIRECTDRAWALREADYCREATED:
		return "A DirectDraw object representing this driver has already been created for this process.";
	case DDERR_NODIRECTDRAWHW:
		return "A hardware-only DirectDraw object creation was attempted but the driver did not support any hardware.";
	case DDERR_PRIMARYSURFACEALREADYEXISTS:
		return "This process already has created a primary surface.";
	case DDERR_NOEMULATION:
		return "Software emulation not available.";
	case DDERR_REGIONTOOSMALL:
		return "Region passed to Clipper::GetClipList is too small.";
	case DDERR_CLIPPERISUSINGHWND:
		return "An attempt was made to set a cliplist for a clipper object that is already monitoring an hwnd.";
	case DDERR_NOCLIPPERATTACHED:
		return "No clipper object attached to surface object.";
	case DDERR_NOHWND:
		return "Clipper notification requires an HWND or no HWND has previously been set as the CooperativeLevel HWND.";
	case DDERR_HWNDSUBCLASSED:
		return "HWND used by DirectDraw CooperativeLevel has been subclassed, this prevents DirectDraw from restoring state.";
	case DDERR_HWNDALREADYSET:
		return "The CooperativeLevel HWND has already been set. It can not be reset while the process has surfaces or palettes created.";
	case DDERR_NOPALETTEATTACHED:
		return "No palette object attached to this surface.";
	case DDERR_NOPALETTEHW:
		return "No hardware support for 16 or 256 color palettes.";
	case DDERR_BLTFASTCANTCLIP:
		return "Return if a clipper object is attached to the source surface passed into a BltFast call.";
	case DDERR_NOBLTHW:
		return "No blitter hardware present.";
	case DDERR_NODDROPSHW:
		return "No DirectDraw ROP hardware.";
	case DDERR_OVERLAYNOTVISIBLE:
		return "Returned when GetOverlayPosition is called on a hidden overlay.";
	case DDERR_NOOVERLAYDEST:
		return "Returned when GetOverlayPosition is called on an overlay that UpdateOverlay has never been called on to establish a destination.";
	case DDERR_INVALIDPOSITION:
		return "Returned when the position of the overlay on the destination is no longer legal for that destination.";
	case DDERR_NOTAOVERLAYSURFACE:
		return "Returned when an overlay member is called for a non-overlay surface.";
	case DDERR_EXCLUSIVEMODEALREADYSET:
		return "An attempt was made to set the cooperative level when it was already set to exclusive.";
	case DDERR_NOTFLIPPABLE:
		return "An attempt has been made to flip a surface that is not flippable.";
	case DDERR_CANTDUPLICATE:
		return "Can't duplicate primary & 3D surfaces, or surfaces that are implicitly created.";
	case DDERR_NOTLOCKED:
		return "Surface was not locked. An attempt to unlock a surface that was not locked at all, or by this process, has been attempted.";
	case DDERR_CANTCREATEDC:
		return "Windows can not create any more DCs, or a DC was requested for a paltte-indexed surface when the surface had no palette AND the display mode was not palette-indexed.";
	case DDERR_NODC:
		return "No DC was ever created for this surface.";
	case DDERR_WRONGMODE:
		return "This surface can not be restored because it was created in a different mode.";
	case DDERR_IMPLICITLYCREATED:
		return "This surface can not be restored because it is an implicitly created surface.";
	case DDERR_NOTPALETTIZED:
		return "The surface being used is not a palette-based surface.";
	case DDERR_UNSUPPORTEDMODE:
		return "The display is currently in an unsupported mode.";
	case DDERR_NOMIPMAPHW:
		return "Operation could not be carried out because there is no mip-map texture mapping hardware present or available.";
	case DDERR_INVALIDSURFACETYPE:
		return "The requested action could not be performed because the surface was of the wrong type.";
	case DDERR_NOOPTIMIZEHW:
		return "Device does not support optimized surfaces, therefore no video memory optimized surfaces.";
	case DDERR_NOTLOADED:
		return "Surface is an optimized surface, but has not yet been allocated any memory.";
	case DDERR_NOFOCUSWINDOW:
		return "Attempt was made to create or set a device window without first setting the focus window.";
	case DDERR_NOTONMIPMAPSUBLEVEL:
		return "Attempt was made to set a palette on a mipmap sublevel"; // new
	case DDERR_DCALREADYCREATED:
		return "A DC has already been returned for this surface. Only one DC can be retrieved per surface.";
	case DDERR_NONONLOCALVIDMEM:
		return "An attempt was made to allocate non-local video memory from a device that does not support non-local video memory.";
	case DDERR_CANTPAGELOCK:
		return "The attempt to page lock a surface failed.";
	case DDERR_CANTPAGEUNLOCK:
		return "The attempt to page unlock a surface failed.";
	case DDERR_NOTPAGELOCKED:
		return "An attempt was made to page unlock a surface with no outstanding page locks.";
	case DDERR_MOREDATA:
		return "There is more data available than the specified buffer size could hold.";
	case DDERR_EXPIRED:
		return "The data has expired and is therefore no longer valid.";
	case DDERR_TESTFINISHED:
		return "The mode test has finished executing."; // new
	case DDERR_NEWMODE:
		return "The mode test has switched to a new mode."; // new
	case DDERR_D3DNOTINITIALIZED:
		return "D3D has not yet been initialized."; // new
	case DDERR_VIDEONOTACTIVE:
		return "The video port is not active.";
	case DDERR_NOMONITORINFORMATION:
		return "The monitor does not have EDID data."; // new
	case DDERR_NODRIVERSUPPORT:
		return "The driver does not enumerate display mode refresh rates."; // new
	case DDERR_DEVICEDOESNTOWNSURFACE:
		return "Surfaces created by one direct draw device cannot be used directly by another direct draw device.";
	case DDERR_NOTINITIALIZED:
		return "An attempt was made to invoke an interface member of a DirectDraw object created by CoCreateInstance() before it was initialized.";
	case D3DERR_BADMAJORVERSION:
		return "The service that you requested is unavailable in this major version of DirectX.";
	case D3DERR_BADMINORVERSION:
		return "The service that you requested is available in this major version of DirectX, but not in this minor version.";
	case D3DERR_INVALID_DEVICE:
		return "The requested device type is not valid.";
	case D3DERR_INITFAILED:
		return "A rendering device could not be created because the new device could not be initialized.";
	case D3DERR_DEVICEAGGREGATED:
		return "The SetRenderTarget method was called on a device that was retrieved from the render target surface.";
	case D3DERR_EXECUTE_CREATE_FAILED:
		return "The execute buffer could not be created. This typically occurs when no memory is available to allocate the execute buffer.";
	case D3DERR_EXECUTE_DESTROY_FAILED:
		return "The memory for the execute buffer could not be deallocated.";
	case D3DERR_EXECUTE_LOCK_FAILED:
		return "The execute buffer could not be locked.";
	case D3DERR_EXECUTE_UNLOCK_FAILED:
		return "D3DERR_EXECUTE_UNLOCK_FAILED";
	case D3DERR_EXECUTE_LOCKED:
		return "The execute buffer could not be unlocked.";
	case D3DERR_EXECUTE_NOT_LOCKED:
		return "The execute buffer could not be unlocked because it is not currently locked.";
	case D3DERR_EXECUTE_FAILED:
		return "The contents of the execute buffer are invalid and cannot be executed.";
	case D3DERR_EXECUTE_CLIPPED_FAILED:
		return "The execute buffer could not be clipped during execution.";
	case D3DERR_TEXTURE_NO_SUPPORT:
		return "The device does not support texture mapping.";
	case D3DERR_TEXTURE_CREATE_FAILED:
		return "The texture handle for the texture could not be retrieved from the driver.";
	case D3DERR_TEXTURE_DESTROY_FAILED:
		return "The device was unable to deallocate the texture memory.";
	case D3DERR_TEXTURE_LOCK_FAILED:
		return "The texture could not be locked.";
	case D3DERR_TEXTURE_UNLOCK_FAILED:
		return "The texture surface could not be unlocked.";
	case D3DERR_TEXTURE_LOAD_FAILED:
		return "The texture could not be loaded.";
	case D3DERR_TEXTURE_SWAP_FAILED:
		return "The texture handles could not be swapped.";
	case D3DERR_TEXTURE_LOCKED:
		return "The requested operation could not be completed because the texture surface is currently locked.";
	case D3DERR_TEXTURE_NOT_LOCKED:
		return "The requested operation could not be completed because the texture surface is not locked.";
	case D3DERR_TEXTURE_GETSURF_FAILED:
		return "The DirectDraw surface used to create the texture could not be retrieved.";
	case D3DERR_MATRIX_CREATE_FAILED:
		return "The matrix could not be created. This can occur when no memory is available to allocate for the matrix.";
	case D3DERR_MATRIX_DESTROY_FAILED:
		return "The memory for the matrix could not be deallocated.";
	case D3DERR_MATRIX_SETDATA_FAILED:
		return "The matrix data could not be set. This can occur when the matrix was not created by the current device.";
	case D3DERR_MATRIX_GETDATA_FAILED:
		return "The matrix data could not be retrieved. This can occur when the matrix was not created by the current device.";
	case D3DERR_SETVIEWPORTDATA_FAILED:
		return "The viewport parameters could not be set.";
	case D3DERR_INVALIDCURRENTVIEWPORT:
		return "The currently selected viewport is not valid.";
	case D3DERR_INVALIDPRIMITIVETYPE:
		return "The primitive type specified by the application is invalid.";
	case D3DERR_INVALIDVERTEXTYPE:
		return "The vertex type specified by the application is invalid.";
	case D3DERR_TEXTURE_BADSIZE:
		return "The dimensions of a current texture are invalid. This can occur when an application attempts to use a texture that has dimensions that are not a power of 2 with a device that requires them.";
	case D3DERR_INVALIDRAMPTEXTURE:
		return "Ramp mode is being used, and the texture handle in the current material does not match the current texture handle that is set as a render state.";
	case D3DERR_MATERIAL_CREATE_FAILED:
		return "The material could not be created. This typically occurs when no memory is available to allocate for the material.";
	case D3DERR_MATERIAL_DESTROY_FAILED:
		return "The memory for the material could not be deallocated.";
	case D3DERR_MATERIAL_SETDATA_FAILED:
		return "The material parameters could not be set.";
	case D3DERR_MATERIAL_GETDATA_FAILED:
		return "The material parameters could not be retrieved.";
	case D3DERR_INVALIDPALETTE:
		return "The palette associated with a surface is invalid.";
	case D3DERR_ZBUFF_NEEDS_SYSTEMMEMORY:
		return "The requested operation could not be completed because the specified device requires system-memory depth-buffer surfaces.";
	case D3DERR_ZBUFF_NEEDS_VIDEOMEMORY:
		return "The requested operation could not be completed because the specified device requires video-memory depth-buffer surfaces.";
	case D3DERR_SURFACENOTINVIDMEM:
		return "The device could not be created because the render target surface is not located in video memory.";
	case D3DERR_LIGHT_SET_FAILED:
		return "The attempt to set lighting parameters for a light object failed.";
	case D3DERR_LIGHTHASVIEWPORT:
		return "The requested operation failed because the light object is associated with another viewport.";
	case D3DERR_LIGHTNOTINTHISVIEWPORT:
		return "The requested operation failed because the light object has not been associated with this viewport.";
	case D3DERR_SCENE_IN_SCENE:
		return "Scene rendering could not begin because a previous scene was not completed by a call to the EndScene method.";
	case D3DERR_SCENE_NOT_IN_SCENE:
		return "Scene rendering could not be completed because a scene was not started by a previous call to the BeginScene method.";
	case D3DERR_SCENE_BEGIN_FAILED:
		return "Scene rendering could not begin.";
	case D3DERR_SCENE_END_FAILED:
		return "Scene rendering could not be completed.";
	case D3DERR_INBEGIN:
		return "The requested operation cannot be completed while scene rendering is taking place.";
	case D3DERR_NOTINBEGIN:
		return "The requested rendering operation could not be completed because scene rendering has not begun.";
	case D3DERR_NOVIEWPORTS:
		return "The requested operation failed because the device currently has no viewports associated with it.";
	case D3DERR_VIEWPORTDATANOTSET:
		return "The requested operation could not be completed because viewport parameters have not yet been set.";
	case D3DERR_VIEWPORTHASNODEVICE:
		return "This value is used only by the IDirect3DDevice3 interface and its predecessors. For the IDirect3DDevice7 interface, this error value is not used.";
	case D3DERR_NOCURRENTVIEWPORT:
		return "The viewport parameters could not be retrieved because none have been set.";
	case D3DERR_INVALIDVERTEXFORMAT:
		return "The combination of flexible vertex format flags specified by the application is not valid.";
	case D3DERR_COLORKEYATTACHED:
		return "Attempted to CreateTexture on a surface that had a color key.";
	case D3DERR_VERTEXBUFFEROPTIMIZED:
		return "The requested operation could not be completed because the vertex buffer is optimized.";
	case D3DERR_VBUF_CREATE_FAILED:
		return "The vertex buffer could not be created. This can happen when there is insufficient memory to allocate a vertex buffer.";
	case D3DERR_VERTEXBUFFERLOCKED:
		return "The requested operation could not be completed because the vertex buffer is locked.";
	case D3DERR_ZBUFFER_NOTPRESENT:
		return "The requested operation could not be completed because the render target surface does not have an attached depth buffer.";
	case D3DERR_STENCILBUFFER_NOTPRESENT:
		return "The requested stencil buffer operation could not be completed because there is no stencil buffer attached to the render target surface.";
	case D3DERR_WRONGTEXTUREFORMAT:
		return "The pixel format of the texture surface is not valid.";
	case D3DERR_UNSUPPORTEDCOLOROPERATION:
		return "The device does not support one of the specified texture-blending operations for color values.";
	case D3DERR_UNSUPPORTEDCOLORARG:
		return "The device does not support one of the specified texture-blending arguments for color values.";
	case D3DERR_UNSUPPORTEDALPHAOPERATION:
		return "The device does not support one of the specified texture-blending operations for the alpha channel.";
	case D3DERR_UNSUPPORTEDALPHAARG:
		return "The device does not support one of the specified texture-blending arguments for the alpha channel.";
	case D3DERR_TOOMANYOPERATIONS:
		return "The application is requesting more texture-filtering operations than the device supports.";
	case D3DERR_CONFLICTINGTEXTUREFILTER:
		return "The current texture filters cannot be used together.";
	case D3DERR_UNSUPPORTEDFACTORVALUE:
		return "The specified texture factor value is not supported by the device.";
	case D3DERR_CONFLICTINGRENDERSTATE:
		return "The currently set render states cannot be used together.";
	case D3DERR_UNSUPPORTEDTEXTUREFILTER:
		return "The specified texture filter is not supported by the device.";
	case D3DERR_TOOMANYPRIMITIVES:
		return "The device is unable to render the provided number of primitives in a single pass.";
	case D3DERR_INVALIDMATRIX:
		return "The requested operation could not be completed because the combination of the currently set world, view, and projection matrices is invalid";
	case D3DERR_TOOMANYVERTICES:
		return "D3DERR_TOOMANYVERTICES";
	case D3DERR_CONFLICTINGTEXTUREPALETTE:
		return "The current textures cannot be used simultaneously.";
	default:
		return "Unrecognized error value.";
	}
}

