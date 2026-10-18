# Media playback probes

These diagnostics check the two independent parts of movie playback:

- `media_decode_probe` verifies fast decoding and changing decoded frames.
- `mf_session_present_probe` uses the game's clocked Media Foundation/EVR
  presentation path and verifies real-time progress of actually presented
  frames. Audio is decoded independently so it cannot throttle the video
  presentation clock.

They do not require Thief 2 data in the repository.

From a Visual Studio developer prompt:

```powershell
msbuild tools\media_decode_probe\media_decode_probe.vcxproj /p:Configuration=Release /p:Platform=Win32
msbuild tools\media_decode_probe\mf_session_present_probe.vcxproj /p:Configuration=Release /p:Platform=Win32
tools\media_decode_probe\Release\media_decode_probe.exe "D:\Games\Thief2\MOVIES\INTRO.mp4"
tools\media_decode_probe\Release\mf_session_present_probe.exe "D:\Games\Thief2\MOVIES\INTRO.mp4"
```

The decode probe processes five seconds without displaying it. The presentation
probe opens a window for ten seconds and obtains images from EVR after
presentation. It fails unless presented timestamps advance at near real time
and at least 100 of its 20 Hz samples contain different frames. A successful run
ends with `PASS`.
