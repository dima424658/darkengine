# Media playback probes

These diagnostics check the two independent parts of movie playback:

- `media_decode_probe` verifies fast decoding and changing decoded frames.
- `mf_session_present_probe` uses the game's clocked Media Foundation/EVR
  presentation path and verifies real-time progress of actually presented
  frames. Audio is decoded independently so it cannot throttle the video
  presentation clock.

They do not require Thief 2 data in the repository.

These standalone probes are not part of the CMake game/editor build and
currently have no CMake targets.

The decode probe processes five seconds without displaying it. The presentation
probe opens a window for ten seconds and obtains images from EVR after
presentation. It fails unless presented timestamps advance at near real time
and at least 100 of its 20 Hz samples contain different frames. A successful run
ends with `PASS`.
