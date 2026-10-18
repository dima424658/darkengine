# D3D11 scene regression rig

This headless WARP test compiles the same shaders and creates the same blend
states as the production D3D11 presenter. It renders a moon behind a cloud
layer, checks exact pixels for texture-alpha and iterated vertex-alpha
composition, and writes a BMP for visual inspection.

```powershell
msbuild tools\render_regression\render_regression.vcxproj /p:Configuration=Release /p:Platform=Win32
Release\render_regression.exe .codex-build\render_regression.bmp
```

The process exits nonzero if any sampled pixel differs from the expected
source-alpha/inverse-source-alpha equation.
