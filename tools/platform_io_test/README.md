# Unix resource path regression and microbenchmark

After building `dark_platform`, run from the repository root:

```sh
c++ -O2 -std=c++17 -Ih tools/platform_io_test/main.cpp \
  build-linux/libdark_platform.a -lSDL3 -ldl -pthread -o /tmp/platform-io-test
/tmp/platform-io-test
```

The test uses an isolated temporary directory. It checks mixed-case resource
reads, exact spelling with ambiguous case matches, cached misses followed by
creation through both file APIs, deletion, and directory changes. It also times
20,000 batches of resource access, open, missing-file access, and reopening an
existing diagnostic file for append. This is a filesystem microbenchmark, not a
measurement of full mission loading or gameplay frame times.
