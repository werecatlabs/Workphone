#ifndef IResourceCompilationDatabase_h__
#define IResourceCompilationDatabase_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/WorkphoneConfig.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <stdexcept>

namespace workphone::resource
{
    /** Compilation namespace. Compiler versions and input settings remain in the source hash. */
    struct WPCore_API CompilationVariant
    {
        String target;
        bool packagedBuild = false;
        String buildKey;

        /** SQLite binds the full byte length. NUL separates the legacy target
         * from the optional canonical key without aliases or a schema rewrite. */
        String indexTarget() const
        {
            if(target.find('\0')!=String::npos)
                throw std::invalid_argument("Compilation target contains NUL");
            if(buildKey.empty()) return target;
            String result=target; result.push_back('\0'); result+=buildKey; return result;
        }
    };

    /** Durable metadata for a successfully compiled resource. */
    struct WPCore_API CompiledResourceRecord
    {
        String resourceId;
        String resourceType;
        String outputPath;
        u64 compilerVersion = 0;
        u64 sourceHash = 0;
        u64 outputHash = 0;

        bool isValid() const
        {
            return !resourceId.empty() && !resourceType.empty() && !outputPath.empty() &&
                   compilerVersion != 0 && sourceHash != 0 && outputHash != 0;
        }

        void clear()
        {
            *this = CompiledResourceRecord();
        }
    };

    /** A source path that participates in a resource's compilation hash. */
    struct WPCore_API CompileDependencyRecord
    {
        String path;
        bool isResource = false;

        bool operator==( const CompileDependencyRecord &other ) const
        {
            return path == other.path && isResource == other.isResource;
        }
    };

    /** One member of an all-or-nothing compilation-index publication. */
    struct WPCore_API CompilationCommit
    {
        CompiledResourceRecord record;
        CompilationVariant variant;
        Array<CompileDependencyRecord> dependencies;
    };

    /**
     * Thread-safe SQLite index used by the resource compiler.
     *
     * Records and their dependency edges are committed in one transaction. The
     * database uses WAL mode, prepared statements, foreign keys, and a schema
     * version so interrupted builds cannot leave a partially updated graph.
     */
    class WPCore_API IResourceCompilationDatabase
    {
    public:
        virtual ~IResourceCompilationDatabase() = default;

        virtual bool connect( const String &databasePath ) = 0;
        virtual void disconnect() = 0;
        virtual bool isConnected() const = 0;

        virtual String getLastError() const = 0;
        virtual bool reset() = 0;

        // Legacy operations: getRecord rejects multiple variants; dependency queries
        // aggregate variants; removeRecord removes all variants. Legacy commits use
        // the reserved empty-target namespace. Production cooking uses explicit variants.
        virtual bool getRecord( const String &resourceId, CompiledResourceRecord &record ) const = 0;
        virtual bool commitCompilation( const CompiledResourceRecord &record,
                                        const Array<CompileDependencyRecord> &dependencies ) = 0;
        virtual bool removeRecord( const String &resourceId ) = 0;

        virtual bool getDependencies( const String &resourceId,
                                      Array<CompileDependencyRecord> &dependencies ) const = 0;
        virtual bool getDependents( const String &dependencyPath, Array<String> &resourceIds ) const = 0;

        // Explicit variants never fall back to another target or build mode.
        virtual bool getRecord( const String &resourceId, const CompilationVariant &variant,
                                CompiledResourceRecord &record ) const = 0;
        virtual bool commitCompilation( const CompiledResourceRecord &record,
                                        const CompilationVariant &variant,
                                        const Array<CompileDependencyRecord> &dependencies ) = 0;
        virtual bool removeRecord( const String &resourceId, const CompilationVariant &variant ) = 0;
        virtual bool getDependencies( const String &resourceId, const CompilationVariant &variant,
                                      Array<CompileDependencyRecord> &dependencies ) const = 0;
        virtual bool getDependents( const String &dependencyPath, const CompilationVariant &variant,
                                    Array<String> &resourceIds ) const = 0;
        /** Commit every record and edge in one transaction, or retain the previous set. */
        virtual bool commitCompilations( const Array<CompilationCommit> &compilations ) = 0;
    };
}  // namespace workphone::resource

#endif  // IResourceCompilationDatabase_h__
