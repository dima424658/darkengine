# ARCHITECTURE CONTEXT: THIEF TRILOGY FOR PS VITA PORT

## 1. CORE TECH STACK
- **Engine Source:** Fork of `dima424658/darkengine` (C++ codebase, Android ARM/SDL3 branch).
- **Target Platform:** Sony PlayStation Vita (ARMv7, VitaSDK, vitaGL, SDL2/SDL3 customization, OpenAL Soft).
- **Asset Paths:** Multi-VPK structure targeting separate LiveArea bubbles:
  * Thief: The Dark Project -> TitleID: THFG00001 -> Path: ux0:data/thief1/
  * Thief Gold -> TitleID: THFG00002 -> Path: ux0:data/thiefgold/
  * Thief II: The Metal Age -> TitleID: THFG00003 -> Path: ux0:data/thief2/

## 2. HARD CONSTRAINTS & CONFIG STACK
- **Hardware Profile:** Optimized for PS Vita 2000 Slim. NO OLED options, NO rumble, NO rear touchpad inputs.
- **Mod Loading & Audio:** Universal Mod Engine architecture where loose files override .crf packages. Polish dialog subtitles (.sub file parser with Windows-1250 encoding support). OpenAL Soft EFX Audio Ambient Reverb.
- **Control Strategy:** Temporary physical button layout mixed with discrete front-touchscreen corner icons for missing inputs (LB/RB triggers). No complex mapping until a stable build is achieved.
- **Menu Settings:** Brightness Slider (white point) + Gamma Shader Correction Slider (Vita Slim LCD shadow depth adjustments) + Spatial Upscaling selector saved under custom vita_options.cfg.
- **Spatial Upscaling:** 100% Native renders 960x544 with no FSR pass; 85% Scale renders 816x462; Uncharted Scale renders 720x408; Extreme Performance renders 640x368. Every resolution below 100% is automatically upscaled to 960x544 through the native Mobile GLES 2.0 AMD FSR 1.0 shader in the vitaGL pipeline.
