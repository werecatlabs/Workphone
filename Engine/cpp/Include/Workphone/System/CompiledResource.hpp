#ifndef WPCompiledResource_h__
#define WPCompiledResource_h__

#include <Workphone/System/ResourceID.hpp>
#include <Workphone/Core/Array.hpp>

#include <memory>

namespace workphone::resource
{
    struct WPCore_API InstallDependencyArtifact
    {
        String path;
        u64 sourceHash = 0;
        u64 payloadHash = 0;

        bool operator==( const InstallDependencyArtifact &other ) const
        {
            return path == other.path && sourceHash == other.sourceHash && payloadHash == other.payloadHash;
        }
    };

    struct WPCore_API CompiledResourceHeader
    {
        static constexpr u16 currentFormatVersion = 2;

        // Existing callers without generation pins write v1; authoring cooks select v2.
        u16 formatVersion = 1;

        ResourceID resourceId;
        ResourceTypeID resourceType;
        u64 compilerVersion = 0;
        u64 sourceHash = 0;
        u64 payloadHash = 0;
        u64 payloadSize = 0;
        Array<ResourceID> installDependencies;
        // V2 entries correspond positionally to installDependencies; paths are root-relative.
        Array<InstallDependencyArtifact> installArtifacts;

        bool isValid() const;
    };

    struct WPCore_API RuntimeResource
    {
        CompiledResourceHeader header;
        Array<u8> payload;
        Array<std::shared_ptr<const RuntimeResource>> dependencies;
    };

    /** Portable, bounds-checked reader/writer for the compiled resource container. */
    class WPCore_API CompiledResourceIO final
    {
    public:
        static bool hashFile( const String &path, u64 &hash, u64 &size, String &error );

        static bool writeAtomic( const String &outputPath, const CompiledResourceHeader &header,
                                 const String &payloadPath, String &error );

        static bool readHeader( const String &path, CompiledResourceHeader &header, String &error );

        /** Stream and validate the complete container without retaining its payload. */
        static bool validate( const String &path, CompiledResourceHeader &header, String &error );

        static bool read( const String &path, RuntimeResource &resource, String &error,
                          u64 maxPayloadBytes = 1024ull * 1024ull * 1024ull );
    };
}  // namespace workphone::resource

#endif  // WPCompiledResource_h__
