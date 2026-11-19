# DDS and optional PBR textures

DDS replacements use the native image loader. Companion PBR maps are used by
the modern Vulkan renderer (`renderer_mode=modern`). Every map is optional:
keep the original base image and add any subset, or add no maps at all.

Keep the original texture's folder and basename. For a texture named `stone`:

| Filename | Content | Missing-map default |
| --- | --- | --- |
| `stone.dds` | Base RGB and native cutout/transparency alpha | Existing base image |
| `stone_normal.dds` | Tangent-space OpenGL **+Y** normal; XY in RG, positive Z reconstructed | Mesh normals |
| `stone_roughness.dds` | R: black = smooth, white = rough | Native scalar, normally **0.8** |
| `stone_metallic.dds` | R: black = dielectric, white = metal | **0** |
| `stone_ao.dds` | R: white = unoccluded | **1** |
| `stone_emissive.dds` | Emitted RGB, added to native emission | Native emission, usually zero |
| `stone_orm.dds` | **R = AO, G = roughness, B = metallic** | Defaults above |

Companions also accept `.png`. Separate scalar maps override corresponding ORM
channels. Use three scalar maps or one packed ORM map. Base images have **no
suffix**; `_basecolor` and `_albedo` are not discovery aliases.

Base color/emissive sample as sRGB. Normal/scalar/ORM maps sample as linear UNORM.
Invert green when converting DirectX (-Y) normals; DXT5nm alpha swizzles and
displacement/parallax maps are not supported. AO attenuates ambient and indirect
diffuse illumination, preserving direct illumination and emission.

Example loose mod layout for an existing Gold texture:

```text
my-pbr-mod/
  fam/Core/COBL2.dds
  fam/Core/COBL2_normal.dds
  fam/Core/COBL2_orm.dds
  fam/Core/COBL2_emissive.dds
```

Model maps go beside the model texture, for example `obj/txt/FRAME_normal.png`.
Discovery follows canonical resource folders, including archive-backed images
and terrain mipmap copies. Normal mod/FM search precedence applies. Use
`content=configured mod_path=/path/to/my-pbr-mod` for a loose mod directory;
explicit `content=vanilla` excludes mod/patch overrides.

## DDS formats

Supported: legacy DXT1/3/5 (BC1/2/3), ATI1/ATI2 (BC4/5 UNORM), DX10 BC1/2/3/4/5/7
UNORM/sRGB variants where applicable, legacy RGB, and common DX10 R8, RG8, RGBA8,
BGRA8/BGRX8. Exact 8-bit channels reach modern rendering; legacy drawing retains
its compatible 16-bit bitmap.

Recommended: **BC7** for base color, emissive and ORM; **BC5 UNORM** for normals;
**BC4 UNORM** for individual scalar maps.

Only the top mip is decoded, into RGBA8. Source compression and DDS mip chains
are not retained on the GPU, so DDS loading does not imply compressed VRAM
savings. Arrays, cubes, volumes, signed BC4/5 and BC6H/HDR are rejected explicitly.
Initialize the pinned decoder with
`git submodule update --init 3rdparty/bcdec`; its license is in that submodule.

## Verification

The standalone `dds` test covers known BC1/2/3/4/5/7 values, exact channels,
partial blocks, pitch and malformed input. The SDK GPU fixture checks normals,
linear scalar maps, ORM precedence, sRGB emission, displayed HDR and restoration
of missing-map defaults.
It also compares equivalent scalar and texture-based emission on secondary GI
hits, ensuring that emission maps contribute beyond the primary raster surface.
Grazing normal-map tests also place emissive geometry behind an opaque receiver:
geometric-normal ray offsets and hemisphere checks must reject both diffuse and
specular GI, independently of the mapped shading normal.
The direct-light variant puts its source in front of the emissive panel to avoid
masking receiver leaks with another occluder. RTXDI rejects the receiver's
geometric backface even when the mapped normal points toward that source.
The transparent forward path has a separate regression: with a grazing normal
map and a source behind its face, it must contribute only ambient reflection
and the alpha-blended background. It uses geometric hemisphere checks and ray
offsets while retaining the mapped normal for BRDF evaluation.

The native test creates a private Gold overlay with BC7 terrain replacement,
six DDS companions and a model texture with only two PNG companions:

```sh
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation VK_LAYER_VALIDATE_SYNC=1 \
  python3 tools/rtx_sdk_test/native_pbr.py /path/to/thief_gold \
  --log /tmp/dark-native-pbr.log
```

It checks actual resource discovery, terrain mip provenance, SDK rendering and
diagnostics. `world-pbr` trace rows give source names and map counts. Missing maps
use defaults without missing-texture warnings. Discovery is cached per loaded
source and refreshed by resource reload or scene reset.
