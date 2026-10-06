# Workphone Engine #

## Summary
Workphone is a game engine. It was made to explore procedural rendering and Ai integration.

## Supported Backends
* Direct3D 11
* OpenGL 3.3+
* Metal

## Supported Platforms

* Windows (7, 8, 10)
* Linux
* macOS
* iOS

### What is this repository for? ###

* Contains the code for the engine. Dependencies are available via google drive.

### How do I get set up? ###

* Use cmake build system

For the full Windows solution in `project_x64`:

```powershell
cmake --preset windows-x64
cmake --build --preset windows-x64-debug
```

The build preset runs four projects at a time, with at most four compiler
processes per MSVC project. It keeps samples, tools and tests in the full build.
Use `windows-x64-release` for Release, or `windows-x64-editor-debug` to build
only the editor and its dependencies while iterating. Override the project
parallelism with `--parallel N` if needed.
For machines with less available memory, configure with
`-DWP_MSVC_COMPILE_PROCESSES=2` to reduce compiler processes per project.

Bundled Boost builds are limited to the libraries Workphone uses, including
their transitive dependencies. To build the entire bundled Boost suite, configure
with `cmake --preset windows-x64 -DWP_BUILD_ALL_BOOST_LIBRARIES=ON`.
An explicit `BOOST_INCLUDE_LIBRARIES` setting takes precedence over the default
Workphone subset.

The engine and editor precompiled headers include common standard-library and
shared utility headers. Changing a PCH triggers a one-time rebuild of the source
files that use it; subsequent source edits reuse the compiled header.

### Contribution guidelines ###

* Uses loose hungarian notation. Generally try to match existing coding style.

### Who do I talk to? ###

* Zane Desir (zanedesir@gmail.com)
