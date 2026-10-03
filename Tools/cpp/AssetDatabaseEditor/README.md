# AssetDatabaseEditor (C++17)

A C++17 port of the original C# `Tools/csharp/AssetDatabaseTool` plus the
in-progress `Tools/cpp/Editor/src/ui/AssetDatabaseWindow.cpp` work in the
`Editor` project.  The tool re-uses the Workphone engine subsystems:

| Subsystem | Target / Header                                | Notes                                                     |
|-----------|------------------------------------------------|-----------------------------------------------------------|
| Renderer  | `Workphone` + `WPGraphics` (+ `WPGraphicsOgre` / `WPGraphicsOgreNext` / `WPGraphics`) | Provides the rendering pipeline and windowing.            |
| UI        | `WPImGui` (via `Workphone/UI/*` interfaces)     | Supplies `IUIWindow`, `IUITreeCtrl`, `IUIDataGrid`, `IUITabBar`, etc. |
| Database  | `WPSQLite` (via `Workphone/Database/*`)         | Hosts `AssetDatabaseManager` and the SQLite query helpers. |
| Importer  | `WPAssimp` (when `WP_USE_ASSET_IMPORT`)        | Optional dependency for asset previews.                   |

## Layout

```
Tools/cpp/AssetDatabaseEditor/
├── CMakeLists.txt
├── README.md
└── src/
    ├── AssetDatabaseEditorPrerequisites.hpp
    ├── AssetDatabaseEditorApplication.{hpp,cpp}
    ├── Main.cpp
    ├── application/
    │   ├── linux/    (optional platform entry point)
    │   ├── macOS/    (optional platform entry point)
    │   └── win32/    (optional platform entry point)
    └── ui/
        ├── AssetDatabaseEditorMainWindow.{hpp,cpp}
        ├── AssetDatabaseWindow.hpp   (mirrors Editor/src/ui/AssetDatabaseWindow.hpp)
        └── AssetDatabaseWindow.cpp   (mirrors Editor/src/ui/AssetDatabaseWindow.cpp)
```

The `ui/AssetDatabaseWindow.{hpp,cpp}` pair mirrors the in-progress Editor
sources so the tool compiles in isolation.  Once the upstream work lands the
file can be replaced with a header-only include of the Editor target.

## Build

The target is wired into the existing top-level CMake build via
`Tools/cpp/CMakeLists.txt`.  Configure the engine as usual:

```sh
cmake -S . -B project_x64 -DWP_BUILD_CPP=ON -DWP_BUILD_ENGINE=ON \
    -DWP_BUILD_IMGUI=ON -DWP_BUILD_SQLITE=ON
cmake --build project_x64 --target AssetDatabaseEditor
```

The produced binary is placed under
`Bin/windows/<toolset>/<arch>/<CRT>/<Configuration>/AssetDatabaseEditor.exe`
on Windows and the equivalent locations on macOS / Linux.

## Status

* CMake project skeleton in place.
* Subsystem dependencies on `Workphone`, `WPImGui`, `WPSQLite` and the active
  renderer library are configured.
* Application + main window scaffolding and `Main.cpp` are in place as a
  starting point for the port.
