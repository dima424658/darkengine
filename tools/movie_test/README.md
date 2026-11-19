# Movie player test

Build from the repository root using the existing engine build configuration:

```sh
cmake --build build-linux --target movie_test -j "$(nproc)"
./build-linux/movie_test /path/to/movie.avi
```

The target is excluded from the default build. It uses `IMoviePlayer1`, a real
render window and an OpenAL mixer. A display and working audio output are
required. The test checks the initial state, opening, synchronous playback,
return to the ready state, replay and closing. It plays the supplied clip twice.
Success returns zero and prints `PASS`; a missing or undecodable movie returns
one. Input can skip playback.

The player prefers a matching `.mp4` sibling, including when the named `.avi`
is absent. To check that behavior with a short generated clip:

```sh
ffmpeg -f lavfi -i testsrc2=size=320x240:rate=30 \
  -f lavfi -i sine=frequency=440:sample_rate=48000 -t 1 \
  -c:v libx264 -pix_fmt yuv420p -c:a aac /tmp/dark-movie-test.mp4
./build-linux/movie_test /tmp/dark-movie-test.avi
```

The game executable no longer handles the `movie_test=` argument.
