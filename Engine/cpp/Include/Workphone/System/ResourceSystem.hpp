#ifndef WPResourceSystem_h__
#define WPResourceSystem_h__

#include <Workphone/Interface/Database/IResourceCompilationDatabase.hpp>
#include <Workphone/Interface/Resource/IResourceSystem.hpp>
#include <functional>

namespace workphone { class StreamBudgetLease; class WorldStreamingScheduler; }

namespace workphone::resource
{
    /** Durable authored identity mapped to one member of a pinned install closure. */
    struct WPCore_API RuntimeResourceBinding
    {
        String uuid;
        ResourceID resourceId;
    };

    struct WPCore_API RuntimePackageContext
    {
        String sourceRoot, target;
        Array<ResourceID> resources;
        std::shared_ptr<const std::map<String,ResourceID>> capturedAssetBindings;
    };

    struct WPCore_API RuntimePackagePreviewEntry
    {
        String uuid;
        ResourceID resourceId;
        u64 payloadBytes = 0;
        bool root = false;
        Array<ResourceID> installDependencies;
    };

    struct WPCore_API RuntimePackagePatchResult
    {
        u32 changedArtifacts = 0, reusedArtifacts = 0, removedResources = 0;
        u64 changedArtifactBytes = 0, targetArtifactBytes = 0;
    };

    struct WPCore_API RuntimePackageStageRecoveryResult
    {
        Array<String> recoverable, recovered, skipped, warnings;
    };

    /** A checked exporter retains admission until publication or failure. The
     * resolver must bind every closure member, acquire external admission in
     * resource-system -> catalog -> connection order, and never reenter this
     * ResourceSystem. The lifetime token releases that admission on this thread.
     */
    struct WPCore_API RuntimePackageAdmission
    {
        Array<RuntimeResourceBinding> bindings;
        std::shared_ptr<void> lifetime;
    };

    using RuntimePackageResolver =
        std::function<bool( const RuntimePackageContext &, RuntimePackageAdmission &, String & )>;

    /** Acquire publication admission after cooking, with the resource operation lock
     * held. The returned token must keep admission through the index transaction.
     * The callback must not reenter resource operations. */
    using CompilationPublicationGuard =
        std::function<bool( const String &sourceRoot, std::shared_ptr<void> &lifetime, String &error )>;

    /** Read-only cooked mount. Limits apply to each unique install dependency closure. */
    struct WPCore_API RuntimeResourceConfig
    {
        String compiledRoot;
        String manifestPath;
        String target = "pc";
        u64 maxPayloadBytes = 1024ull * 1024ull * 1024ull;
        u64 maxClosurePayloadBytes = 1024ull * 1024ull * 1024ull;
        u32 maxClosureResources = 4096;
        u32 maxDependencyDepth = 64;
    };

    /** Ordered package composition. Earlier entries have higher precedence.
     * Changed fingerprints may shadow only under PreferEarlier; every visible
     * parent's dependency pins must still match the selected complete variant. */
    enum class RuntimeMountCollisionPolicy { Reject, IdenticalOnly, PreferEarlier };
    struct WPCore_API RuntimeResourceMountConfig
    {
        String name;
        RuntimeResourceConfig config;
    };
    struct WPCore_API RuntimeResourceProvenance
    {
        std::shared_ptr<StreamBudgetLease> admission;
        ResourceID resourceId;
        String mountName,compiledRoot,manifestPath;
        u64 manifestSourceHash=0,manifestPayloadHash=0;
        u32 precedence=0,shadowedMounts=0,manifestVersion=0;
    };

    /** Manifest-sealed corruption-check directory; hashes are not authentication. */
    struct WPCore_API RuntimeResourceRangeInfo
    {
        std::shared_ptr<StreamBudgetLease> admission;
        CompiledResourceHeader header;
        u32 manifestVersion=0,chunkBytes=65536;
        u64 chunkCount=0,metadataBytes=0;
        bool independentlyVerified=false;
    };
    struct WPCore_API RuntimeResourceChunk
    {
        std::shared_ptr<StreamBudgetLease> admission;
        ResourceID resourceId;
        u64 offset=0,payloadSize=0,hash=0,payloadBytesRead=0;
        bool legacyWholePayloadFallback=false;
        Array<u8> bytes;
    };

    /**
     * Dependency-aware build and runtime resource service.
     *
     * Compilation is serialized per ResourceSystem instance, while registry,
     * metadata queries, and runtime cache reads are thread-safe. Compilers can be
     * registered dynamically before or after initialization.
     * Authoring starts in Editor mode. A successful compile (or wholly successful
     * nonempty compileAffected batch) selects its build mode for later loads and
     * manifest exports. Each load/export closure uses that one mode throughout.
     * Authoring holds an OS writer lease for its canonical cooked root and target
     * until shutdown. Distinct targets may coexist. Each compile/affected batch
     * publishes its complete changed index set in a single SQLite transaction.
     */
    class WPCore_API ResourceSystem final : public IResourceSystem
    {
    public:
        ResourceSystem( std::shared_ptr<IResourceCompilerRegistry> registry,
                        std::shared_ptr<IResourceCompilationDatabase> database );
        ~ResourceSystem() override;

        ResourceSystem( const ResourceSystem & ) = delete;
        ResourceSystem &operator=( const ResourceSystem & ) = delete;

        bool initialize( const ResourceSystemConfig &config, String &error ) override;
        /** Requires no source tree, compiler registry or compilation database. Never writes. */
        bool initializeRuntime( const RuntimeResourceConfig &config, String &error );
        /** Adopt dedicated manifest admissions held by a shared streaming mount.
         * Nonnull leases must have the correct class and sufficient bytes; an
         * omitted admission is reserved internally before parsing. */
        bool initializeRuntime(const RuntimeResourceConfig &,String &error,
            std::shared_ptr<StreamBudgetLease> manifestAdmission,
            std::shared_ptr<StreamBudgetLease> workingAdmission = {});
        /** Captures 1..32 named packages once. No mount mutation or source
         * fallback; rejected composition leaves this instance uninitialized. */
        static bool compositionMemoryRequirements(const Array<RuntimeResourceMountConfig> &,
            u64 &sourceBytes,u64 &workingBytes,String &error);
        bool initializeRuntimeComposition(const Array<RuntimeResourceMountConfig> &,
            RuntimeMountCollisionPolicy,String &error,std::shared_ptr<StreamBudgetLease> sourceAdmission={},
            std::shared_ptr<StreamBudgetLease> workingAdmission={});
        bool runtimeProvenance(const ResourceID &,RuntimeResourceProvenance &,String &error,
            std::shared_ptr<WorldStreamingScheduler> admissionScheduler={}) const;

        /** Export the committed cooked install closure, pinning hashes, versions and target.
         * Roots must first pass compile() (including UpToDate) in this authoring session.
         * The manifest is a portable WPRS container; it does not copy resource payloads.
         * Callers publish an immutable copy of the matching cooked files alongside it.
         */
        bool writeRuntimeManifest( const String &path, const Array<ResourceID> &roots, String &error );
        /** Manifest v3 adds explicit UUID bindings. UUIDs use canonical dashed hex
         * spelling (input case is normalized); nil, duplicate UUIDs, aliased resource
         * identities and resources outside the exported closure are rejected.
         * Unbound resources remain accessible by ResourceID. No catalog is inferred.
         */
        bool writeRuntimeManifest( const String &path, const Array<ResourceID> &roots,
                                   const Array<RuntimeResourceBinding> &bindings, String &error );
        /** Export only the verified pinned install closure and runtime.wprm to a
         * new directory. Requires successful package-mode compilation. Stages on
         * the destination volume, validates copied containers, then publishes the
         * directory exclusively. Existing destinations and source/cooked overlap
         * are rejected. Does not package executables or certify typed consumers.
         */
        bool writeRuntimePackage( const String &directory, const Array<ResourceID> &roots,
                                  const Array<RuntimeResourceBinding> &bindings, String &error );
        /** Policy-aware export. Rejects absent admission or incomplete UUID coverage. */
        bool writeRuntimePackageChecked( const String &directory, const Array<ResourceID> &roots,
                                         const RuntimePackageResolver &resolver, String &error );
        /** Create a deterministic delta between two complete manifest-v4 packages.
         * All inputs are read-only. The patch owns its target manifest and changed
         * immutable artifacts. Publication requires a new, separate directory. */
        static bool writeRuntimePatch( const String &baseDirectory, const String &targetDirectory,
                                       const String &patchDirectory, const String &target,
                                       RuntimePackagePatchResult &, String &error );
        /** Verify the exact base generation and assemble the target into a new
         * directory. A failed or interrupted apply leaves the old package intact. */
        static bool applyRuntimePatch( const String &baseDirectory, const String &patchDirectory,
                                       const String &outputDirectory, const String &target,
                                       RuntimePackagePatchResult &, String &error );
        /** Inspect or quarantine abandoned, owned staging directories in one
         * publication parent. Live, unmarked and redirected entries are preserved.
         * Recovery renames exclusively and never deletes stage contents. */
        static bool recoverRuntimePackageStages( const String &publicationParent, bool inspectOnly,
                                                RuntimePackageStageRecoveryResult &, String &error );
        /** Cooks are performed by the caller. Inspects the same pinned, admitted
         * closure as export without publishing or touching a package destination.
         * A temporary manifest is owned under cooked storage and removed on return. */
        bool previewRuntimePackageChecked( const Array<ResourceID> &roots,
                                           const RuntimePackageResolver &resolver,
                                           Array<RuntimePackagePreviewEntry> &entries, String &error );
        /** Runtime-only UUID lookup. Failure clears output; no path fallback. */
        bool resolveRuntimeUUID( const String &uuid, ResourceID &output, String &error ) const;
        /** Resolves and loads under one mount lifetime fence, sharing load() cache/limits. */
        std::shared_ptr<const RuntimeResource> loadUUID( const String &uuid, String &error );
        void shutdown() override;
        bool isInitialized() const override;

        CompilationReport compile( const ResourceID &resourceId,
                                   const CompilationOptions &options = {} ) override;

        /** Cook all roots from one input capture and publish them in one index transaction. */
        Array<CompilationReport> compileBatch( const Array<ResourceID> &roots,
                                               const CompilationOptions &options,
                                               const CompilationPublicationGuard &guard = {},
                                               const std::function<bool()> &isCancelled = {} );

        /** Recompile all transitive dependents of a changed data:// path. */
        Array<CompilationReport> compileAffected( const String &changedPath,
                                                  const CompilationOptions &options = {} ) override;

        std::shared_ptr<const RuntimeResource> load( const ResourceID &resourceId,
                                                     String &error ) override;
        /** Read one pinned payload without reading or retaining child payloads.
         * The returned dependencies array is empty; install dependencies and pins
         * remain in its validated header. Intended for runtime page manifests.
         * A shallow result never enters the complete-closure identity cache.
         */
        std::shared_ptr<const RuntimeResource> loadPayload( const ResourceID &, String &error,
                                                            u64 maximumPayloadBytes = 1024ull << 20 );
        /** Runtime-only admission plan from the captured manifest. Metadata is a
         * conservative allowance for retained header/object storage and bounded
         * reader temporaries; outputs reset on failure. Does not open artifacts. */
        bool payloadMemoryRequirements( const ResourceID &, u64 &payloadBytes,
                                        u64 &metadataBytes, String &error ) const;
        bool resourceInfo(const ResourceID &,RuntimeResourceRangeInfo &,String &error) const;
        /** V4 verifies one chunk against its manifest pin. Earlier manifests
         * validate the complete payload before slicing; no unchecked ranges. */
        std::shared_ptr<const RuntimeResourceChunk> loadChunk(const ResourceID &,u64 index,String &error,
                                                            bool workingAlreadyAdmitted=false);
        void unload( const ResourceID &resourceId ) override;
        void clearRuntimeCache() override;

        std::shared_ptr<IResourceCompilerRegistry> registry() const override;

    private:
        bool initializeRuntimeBounded(const RuntimeResourceConfig &,String &,
            std::shared_ptr<StreamBudgetLease>,std::shared_ptr<StreamBudgetLease>,u64 maximumContainerBytes);
        bool writeRuntimePackageImpl( const String &, const Array<ResourceID> &,
                                      const Array<RuntimeResourceBinding> &,
                                      const RuntimePackageResolver &, String &,
                                      Array<RuntimePackagePreviewEntry> *preview = nullptr );
        bool writeRuntimeManifestLocked( const String &, const Array<ResourceID> &,
                                         const Array<RuntimeResourceBinding> &, String & );
        class Impl;
        Impl *m_impl = nullptr;
    };
}  // namespace workphone::resource

#endif  // WPResourceSystem_h__
