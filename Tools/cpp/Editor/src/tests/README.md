# Editor play mode regression tests

Build and run from the repository root with editor tests enabled (`WP_EDITOR_TESTS=ON`):

```powershell
cmake --build project_x64 --config Debug --target Editor --parallel 6
ctest --test-dir project_x64 -C Debug -L playmode --output-on-failure
```

Each case runs in its own editor process. CTest registers editor tests only for Debug;
the play mode cases have a 60-second process timeout. The fixture uses bounded
predicate waits and application-task barriers to observe queued transitions.
Exceptions and failed preconditions fail the test instead of being logged and ignored.

The fixture loads the real editor, renderer, viewport, scene and camera manager.
It supplies a temporary plugin configuration matching the configured graphics backend
and enables the camera manager that the editor's debug loading path disables.
It does not reset cameras or repair flags after entering or stopping play mode.
Temporary scene files are isolated from project content and removed during cleanup.

Coverage includes:

| Area | Regression checks |
| --- | --- |
| Camera handoff | Application flags, actor/component state, renderer visibility, viewport activity and editor render target |
| Eligibility | Enabled cameras; disabled components, actors and parents; editor-only cameras; scenes without a game camera |
| Transitions | Stop, duplicate requests, repeated cycles, rapid requests, pause/restart and camera override/undo |
| Scene restoration | Runtime actor removal; transforms, camera settings, hierarchy, original path/label and editor save flag; disk content unchanged |
| Scene lifecycle | Saved and unsaved scenes, empty/nonempty cache paths, failed loads, missing current scene and runtime scene switching |
| Serialization | Modern and legacy editor-camera data, file and memory loaders, wrapped controller aliases and modern-data precedence |
| Ownership | Editor camera/controller identity, exactly one editor camera, no duplicate or unloaded camera registrations |
| Worker threads | Repeated/rapid requests and latest-request semantics for queued scene state changes |

The pre-play snapshot serializes actor/component settings. Shared resource objects
referenced by UUID (for example lighting resources) are not deeply copied by this
snapshot and are outside the restoration guarantees checked here.

## Full suite and shutdown diagnostics

Build and run the Debug editor tests from the repository root:

```powershell
cmake -S . -B project_x64 -DWP_EDITOR_TESTS=ON
cmake --build project_x64 --config Debug --target Editor --parallel 4
ctest --test-dir project_x64 -C Debug -R '^Editor\.' --output-on-failure
```

CTest runs each registered case separately and `Editor.all` runs the complete Boost.Test suite
in one process to catch problems across repeated application and plugin lifecycles. Tests
use the editor executable directory as their working directory and run serially.
The Debug runner also checks CRT heap integrity before returning its exit code.
Claw builds the bundled FreeImage codec even with `WP_BUILD_DEPENDENCIES=OFF`, so
the OpenEXR registry cleanup is included instead of linking an older prebuilt codec.

For allocation stacks in the Windows Debug build, set `WP_EDITOR_ALLOCATION_REPORT` to an
absolute report path before launching `Editor.exe` from its executable directory:

```powershell
$env:WP_EDITOR_ALLOCATION_REPORT = 'G:\workphone_master\lioncat\project_x64\editor-allocations.log'
.\Editor.exe --run_test=editor_repeated_lifecycle --log_level=error --report_level=detailed
Remove-Item Env:WP_EDITOR_ALLOCATION_REPORT
```

This opt-in diagnostic records up to two million allocation requests, with sixteen stack
frames each, in process-lifetime virtual memory (about 260 MiB). Normal test runs do not
allocate this diagnostic storage. The report contains CRT leak messages and module-relative
stack addresses that can be resolved using the matching Debug PDBs. Frames belonging to
plugins that have already unloaded are omitted; reports beyond the request limit have no
recorded stack. CRT exit reports can include buffers owned by static objects, so investigate
allocation ownership before interpreting every reported block as an application leak.
For long runs, set `WP_EDITOR_ALLOCATION_TAIL=1` to retain the most recent two million
allocation stacks instead; older stacks are overwritten while the CRT still reports all leaks.
`WP_EDITOR_ALLOCATION_SIZE` can restrict stack collection to one block size in bytes
from the CRT report, helping retain rare allocations during long runs.

When running under a debugger, `WP_EDITOR_BREAK_ALLOCATION` can specify a CRT allocation
request number from a previous report. Set it along with `WP_EDITOR_ALLOCATION_REPORT` to
break at that allocation and inspect frames before the originating plugin unloads.
