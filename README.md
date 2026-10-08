not ready for use just yet, do not use this project

Build `benchmark.cpp` or `parser_test.cpp` as a separate executable using the same
compiler settings and VTune dependency as `Source.cpp`. Do not link their `main`
functions into the normal application.

```bat
cl /std:c++20 /O2 /arch:AVX2 /EHsc /DNDEBUG benchmark.cpp /link OneCore.lib
cl /std:c++20 /O2 /arch:AVX2 /EHsc /DNDEBUG parser_test.cpp /link OneCore.lib
clang-cl /std:c++20 /O2 /arch:AVX2 /EHsc /DNDEBUG /clang:-O3 /clang:-march=skylake /clang:-mbranches-within-32B-boundaries benchmark.cpp /Febenchmark-clang.exe /link OneCore.lib
parser_test.exe
benchmark.exe C:\path\to\maps 20001 7
```

The benchmark preloads sorted `.osu` files, warms the parser, pins logical core 2,
and times four corpus repetitions per pass. Arguments are the map directory,
map limit (`0` for all), pass count, optional binary result dump (`-` to skip),
and optional input offset (`0`–`31`). Result dumps contain parsed fields rather
than pointers or structure padding and can be compared between revisions.
Input padding, loading, and result serialization are outside timed passes.
