# Fixed-point construction regression

From the repository root, compile and run with undefined-behavior checks:

```sh
cc -std=c17 -O2 -g -fsanitize=undefined -fno-sanitize-recover=all \
  -Ih -Ilibsrc/fix tools/fixed_point_test/main.c -o /tmp/dark-fixed-point-test
/tmp/dark-fixed-point-test
```

The test covers every representable integer part at five fractional values,
compile-time constants, unsigned wrap, and arguments with side effects.
