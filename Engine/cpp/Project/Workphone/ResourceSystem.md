# Resource system

The resource system is Workphone's path-based resource build and runtime layer. It contains the
compiler registry, dependency metadata interface, and compiled-resource container support.

## Resource IDs

Resources use canonical logical paths such as `data://materials/paint.matres`. The extension is
the resource type and must contain one to eight lowercase letters or digits. Sub-resources use
`data://characters/hero.char:lod0.mesh`; their compiled filename is flattened to
`characters/hero_lod0.mesh`. Absolute paths, traversal segments, and non-`data` URI schemes are
rejected.

## Registering a compiler

Implement `workphone::resource::IResourceCompiler`, declare each output type and version, and
register a shared instance:

```cpp
auto registry = std::make_shared<workphone::resource::ResourceCompilerRegistry>();
workphone::String error;
if (!registry->registerCompiler(std::make_shared<MyTextureCompiler>(), &error)) {
    // Report error.
}
```

`getDependencies()` reports raw source files, resource compile dependencies, and runtime install
dependencies. `compile()` writes only the payload; `ResourceSystem` writes the versioned container
header and atomically publishes it.

## Starting the system

Provide an implementation of `IResourceCompilationDatabase`. The WPSQLite module supplies
`ResourceCompilationDatabase`, but Workphone does not depend on that implementation.

```cpp
auto database = std::make_shared<workphone::resource::ResourceCompilationDatabase>();
workphone::resource::ResourceSystem resources(registry, database);
workphone::resource::ResourceSystemConfig config;
config.sourceRoot = projectDataPath;
config.compiledRoot = compiledDataPath;
config.target = "windows-x64";

workphone::String error;
if (resources.initialize(config, error)) {
    auto report = resources.compile(
        workphone::resource::ResourceID("data://textures/albedo.texres"));
}
```

If `databasePath` is empty, the compilation index is stored as `.resource_compilation.db` under
the compiled root. Database records are relocatable because output paths are stored relative to
that root.

## Operational guarantees

- Source and output paths are canonicalized and confined to their configured roots.
- Compiler versions, target, build mode, source contents, and all transitive dependencies
  participate in the incremental hash.
- Dependency cycles and compiler type collisions fail with diagnostics.
- Compiled files use a bounded, endian-stable header and a validated payload hash.
- Existing compiled outputs remain available when a compiler fails; successful replacements are
  atomic.
- Metadata and dependency edges are committed through the injected database interface.
- Runtime loads validate identity, size, and payload integrity, load install dependencies first,
  and share resources through a thread-safe weak cache.

The lightweight `WPResourceTests` target covers compilation, up-to-date detection, transitive
rebuilds, install dependency loading, path traversal rejection, registry collisions, payload
corruption, and forced repair.
