# Native graphics runtime sample

This Windows console smoke application loads a cooked WPM1 mesh, runs pooled
transforms and shared main/shadow culling, and draws into an offscreen software
buffer through native update/render. It prints its result and exits. It does not
open a graphics window or exercise a GPU backend, the C++ engine/editor or catalog.

Extract the runtime ZIP and run from its root in PowerShell:

```powershell
./bin/WorkphoneGraphicsSample.exe ./content/triangle.wpm
```

A successful run exits 0. Missing or corrupt meshes exit 1 with a diagnostic;
incorrect arguments exit 2. The package includes the executable, cooked mesh,
linker PDB, license notices and a file-hash manifest. No SDK headers/libraries,
source files, compiler or CMake are needed to launch it.
The packager checks the executable's PE machine field against the declared
architecture before creating a distributable runtime archive.

Match the architecture in runtime-manifest.json. Windows Direct3D system DLLs
and the Microsoft Visual C++ release DLL runtime must already be installed; the
manifest records the SDK compiler and prerequisite policy. No Windows/Microsoft
DLLs are bundled. The sample may import backend DLLs even though it draws with
the software renderer. Read-only payload use is checked by the validation runner.

Local validation launches the extracted package from a fresh temporary directory
with a restricted process environment. It records actual loaded module paths and
requires each module to be the application executable or reside under Windows.
All payload files have their read-only attributes set during the launch tests,
and file hashes/counts must match afterward. The isolation directory must be
outside the checkout, SDK and consumer build trees.
This verifies local dependency closure, not execution on a clean machine or
hardware/recovery acceptance. Hashes detect accidental changes; they are not a
signature or an authenticity guarantee. releaseCertified remains false.
