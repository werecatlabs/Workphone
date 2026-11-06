#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/Resources/ClawTextureStream.hpp>
#include <WPGraphics/Resources/GraphicsResourceCompiler.hpp>
#include <WPGraphics/Resources/GraphicsResourceFormats.hpp>
#include <Workphone/System/ResourceChunkStream.hpp>
#include <Workphone/System/ResourceLeafProvider.hpp>
#include <Workphone/Graphics/DdsTexture.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <workphone_graphics_renderer_dx11.h>
#include <workphone_graphics_texture_mips.h>
#include "../ClawAssetBudget.hpp"
#include <array>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <mutex>
#include <thread>
#include <limits>
#include <stdexcept>

namespace workphone::render
{
    namespace
    {
        constexpr u32 maximumMips = 16;
        constexpr u64 maximumBootstrap = 64ull << 10;

        void require( bool value, const char *message )
        {
            if( !value )
            {
                throw std::runtime_error( message );
            }
        }

        struct Mip
        {
            u32 width = 0, height = 0;
            u64 offset = 0, size = 0, hash = 0;
        };

        struct Plan final : resource::RuntimeResourceStreamData
        {
            StreamBudget budget;
            resource::ResourceID id;
            u64 sourceHash = 0, payloadHash = 0, payloadSize = 0, compilerVersion = 0;
            String target;
            bool packaged = false, independentlyVerified = false, wholeLegacy = false;
            TextureMipSettings settings;
            u32 count = 0, format = 0, bootstrapFirst = 0;
            std::array<Mip, maximumMips> mips{};
            std::vector<u8> bootstrap;
        };

        struct Reader
        {
            const u8 *bytes;
            u64 size, at = 0;

            u32 word()
            {
                require( at <= size && size - at >= 4, "Truncated runtime texture plan" );
                const auto p = bytes + at;
                at += 4;
                return u32( p[0] ) | u32( p[1] ) << 8 | u32( p[2] ) << 16 | u32( p[3] ) << 24;
            }

            u64 wide()
            {
                const auto low = word();
                return low | ( u64( word() ) << 32 );
            }

            f32 number()
            {
                const auto bits = word();
                f32 result;
                std::memcpy( &result, &bits, 4 );
                return result;
            }

            void skip( u64 amount )
            {
                require( at <= size && amount <= size - at, "Texture mip range exceeds its payload" );
                at += amount;
            }
        };

        struct Parsed
        {
            char target[129]{};
            bool packaged = false;
            TextureMipSettings settings;
            u32 count = 0, format = 0;
            std::array<Mip, maximumMips> mips{};
        };

        u32 bootstrapFirst( const Parsed &parsed )
        {
            if( !parsed.count )
            {
                return 0;
            }
            auto first = parsed.count - 1;
            // A BC resource's top-level dimensions must be block aligned, even
            // though its subordinate mip levels may be smaller than one block.
            if( dds::blockBytes( parsed.format ) )
            {
                while( first && ( parsed.mips[first].width % 4 || parsed.mips[first].height % 4 ) )
                {
                    --first;
                }
            }
            require( !dds::blockBytes( parsed.format ) ||
                         ( !( parsed.mips[first].width % 4 ) && !( parsed.mips[first].height % 4 ) ),
                     "DDS has no block-aligned native mip base" );
            return first;
        }

        u64 bootstrapSize( const Parsed &parsed )
        {
            if( !parsed.count )
            {
                return 0;
            }
            const auto &last = parsed.mips[parsed.count - 1];
            return last.offset + last.size - parsed.mips[bootstrapFirst( parsed )].offset;
        }

        Parsed parse( const resource::RuntimeResource &value )
        {
            Parsed out;
            Reader reader{ value.payload.data(), value.payload.size() };
            const auto magic = reader.word(), version = reader.word();
            require( ( magic == 0x58545057 || magic == 0x58445057 ) && ( version == 1 || version == 2 ),
                     "Unsupported range-streamed texture encoding" );
            const auto targetSize = reader.word();
            require( targetSize && targetSize <= 128, "Invalid runtime texture target" );
            require( targetSize <= reader.size - reader.at, "Truncated runtime texture target" );
            std::memcpy( out.target, reader.bytes + reader.at, targetSize );
            reader.skip( targetSize );
            require( std::memchr( out.target, 0, targetSize ) == nullptr,
                     "Invalid runtime texture target string" );
            const auto mode = reader.word();
            require( mode <= 1, "Invalid runtime texture build mode" );
            out.packaged = mode != 0;
            const auto filter = reader.word();
            require( filter <= static_cast<u32>( TextureMipFilter::Roughness ),
                     "Invalid runtime texture mip filter" );
            out.settings.filter = static_cast<TextureMipFilter>( filter );
            out.settings.atlasColumns = reader.word();
            out.settings.alphaCutoff = reader.number();
            require( out.settings.atlasColumns && std::isfinite( out.settings.alphaCutoff ) &&
                         out.settings.alphaCutoff > 0 && out.settings.alphaCutoff < 1,
                     "Invalid runtime texture mip settings" );
            const auto storage = version == 2 ? reader.word() : 0;
            if( magic == 0x58545057 )
            {
                require( storage == 0, "Portable streamed texture cannot use a DDS storage format" );
                out.count = reader.word();
                require( out.count && out.count <= maximumMips, "Invalid streamed texture mip count" );
                for( u32 i = 0; i < out.count; ++i )
                {
                    auto &mip = out.mips[i];
                    mip.width = reader.word();
                    mip.height = reader.word();
                    mip.size = reader.wide();
                    mip.offset = reader.at;
                    require( mip.width && mip.height && mip.width <= 16384 && mip.height <= 16384 &&
                                 mip.size == u64( mip.width ) * mip.height * 4 &&
                                 mip.size <= graphicsTextureByteLimit,
                             "Invalid streamed BGRA mip dimensions" );
                    reader.skip( mip.size );
                }
            }
            else
            {
                const auto ddsSize = reader.wide(), base = reader.at;
                require( ddsSize == reader.size - base && ddsSize >= 148,
                         "Invalid streamed DDS extent" );
                const auto word = [&]( u64 offset ) {
                    Reader header{ reader.bytes + base, ddsSize, offset };
                    return header.word();
                };
                require( word( 0 ) == 0x20534444 && word( 4 ) == 124 && word( 76 ) == 32 &&
                             word( 80 ) == 4 && word( 84 ) == 0x30315844 && word( 24 ) == 0 &&
                             word( 112 ) == 0 && word( 116 ) == 0 && word( 120 ) == 0 &&
                             word( 132 ) == 3 && word( 136 ) == 0 && word( 140 ) == 1 &&
                             word( 144 ) <= 3,
                         "Unsupported streamed DDS topology/header" );
                out.format = word( 128 );
                require( dds::supported( out.format ) && ( !storage || storage == out.format ),
                         "Unsupported streamed DDS format" );
                auto width = word( 16 ), height = word( 12 );
                out.count = word( 28 );
                require(
                    width && height && width <= 16384 && height <= 16384 && out.count && out.count <= 15,
                    "Invalid streamed DDS dimensions" );
                reader.skip( 148 );
                const auto block = dds::blockBytes( out.format );
                for( u32 i = 0; i < out.count; ++i )
                {
                    auto &mip = out.mips[i];
                    mip.width = width;
                    mip.height = height;
                    mip.offset = reader.at;
                    mip.size = u64( block ? ( ( width + 3 ) / 4 ) * block : width * 4 ) *
                               ( block ? ( height + 3 ) / 4 : height );
                    reader.skip( mip.size );
                    width = std::max( width / 2, 1u );
                    height = std::max( height / 2, 1u );
                }
            }
            require( reader.at == reader.size, "Unexpected trailing streamed texture bytes" );
            require( out.mips[0].width % out.settings.atlasColumns == 0,
                     "Invalid streamed texture atlas" );
            for( u32 i = 0; i < out.count; ++i )
            {
                auto &mip = out.mips[i];
                u32 nextWidth = 0, nextHeight = 0;
                const bool last = out.settings.filter == TextureMipFilter::None ||
                                  !wp_graphics_texture_next_mip_extent( mip.width, mip.height,
                                      out.settings.atlasColumns, &nextWidth, &nextHeight );
                require( last == ( i + 1 == out.count ), "Incomplete or excessive streamed mip chain" );
                if( !last )
                {
                    require( out.mips[i + 1].width == nextWidth && out.mips[i + 1].height == nextHeight,
                             "Inconsistent streamed mip chain" );
                }
                mip.hash =
                    resource::hashBytes( reader.bytes + mip.offset, static_cast<size_t>( mip.size ) );
            }
            return out;
        }

        bool matches( const Plan &plan, const resource::RuntimeResource &value )
        {
            return value.header.isValid() && !value.header.resourceId.isSubResource() &&
                   ( plan.wholeLegacy
                         ? ( value.payload.size() == plan.payloadSize &&
                             resource::hashBytes( value.payload.data(), value.payload.size() ) ==
                                 plan.payloadHash )
                         : value.payload.empty() ) &&
                   value.dependencies.empty() &&
                   value.header.resourceType == resource::ResourceTypeID( "texres" ) &&
                   value.header.resourceId == plan.id && value.header.sourceHash == plan.sourceHash &&
                   value.header.payloadHash == plan.payloadHash &&
                   value.header.payloadSize == plan.payloadSize &&
                   value.header.compilerVersion == plan.compilerVersion &&
                   value.header.installDependencies.empty();
        }
    }  // namespace

    bool isRuntimeTextureSnapshot( const resource::RuntimeResource &value )
    {
        const auto plan = std::dynamic_pointer_cast<const Plan>( value.streamData );
        return plan && !plan->wholeLegacy && matches( *plan, value );
    }

    bool describeRuntimeTextureSnapshot( const resource::RuntimeResource &value,
                                         CookedTextureData &output, String &error )
    {
        output = CookedTextureData{};
        error.clear();
        try
        {
            const auto plan = std::dynamic_pointer_cast<const Plan>( value.streamData );
            require( plan && matches( *plan, value ),
                     "Unrecognized or mismatched sparse runtime texture plan" );
            output.target = plan->target;
            output.packagedBuild = plan->packaged;
            output.mipSettings = plan->settings;
            output.storageFormat = plan->format;
            output.levels.resize( plan->count );
            for( u32 i = 0; i < plan->count; ++i )
            {
                output.levels[i].width = plan->mips[i].width;
                output.levels[i].height = plan->mips[i].height;
            }
            return true;
        }
        catch( const std::exception &failure )
        {
            error = failure.what();
            return false;
        }
    }

    bool sampleRuntimeTextureSnapshotBGRA( const resource::RuntimeResource &value, u32 x, u32 y,
                                           std::array<u8, 4> &output, String &error )
    {
        output = {};
        error.clear();
        try
        {
            const auto plan = std::dynamic_pointer_cast<const Plan>( value.streamData );
            require( plan && matches( *plan, value ) && !plan->wholeLegacy && plan->format == 0 &&
                         plan->count == 1,
                     "Texture sample requires a private one-mip portable snapshot" );
            const auto &mip = plan->mips[0];
            require( x < mip.width && y < mip.height &&
                         plan->bootstrap.size() == u64( mip.width ) * mip.height * 4,
                     "Texture sample exceeds its verified bootstrap" );
            const auto offset = ( u64( y ) * mip.width + x ) * 4;
            for( size_t channel = 0; channel < 4; ++channel )
            {
                output[channel] = plan->bootstrap[size_t( offset ) + channel];
            }
            return true;
        }
        catch( const std::exception &failure )
        {
            error = failure.what();
            return false;
        }
    }

    std::shared_ptr<const resource::RuntimeResource> snapshotRuntimeTexture(
        std::shared_ptr<const resource::RuntimeResource> value,
        std::shared_ptr<resource::ResourcePayloadMount> mount, String &error )
    {
        error.clear();
        try
        {
            require( value && mount, "Texture snapshot requires its immutable package mount" );
            if( value->header.resourceType != resource::ResourceTypeID( "texres" ) )
            {
                return value;
            }
            if( const auto plan = std::dynamic_pointer_cast<const Plan>( value->streamData );
                plan && matches( *plan, *value ) )
            {
                return value;
            }
            require( value->header.isValid() && !value->header.resourceId.isSubResource() &&
                         value->header.resourceId.type() == resource::ResourceTypeID( "texres" ) &&
                         value->header.installDependencies.empty() && value->dependencies.empty() &&
                         value->header.payloadSize == value->payload.size() &&
                         value->payload.size() <= graphicsTextureByteLimit + 4096 &&
                         resource::hashBytes( value->payload.data(), value->payload.size() ) ==
                             value->header.payloadHash,
                     "Texture snapshot requires a sealed complete texres resource" );
            const GraphicsResourceCompiler compiler;
            require( value->header.compilerVersion ==
                             compiler.versionFor( resource::ResourceTypeID( "texres" ) ) ||
                         value->header.compilerVersion == 2,
                     "Incompatible streamed texture compiler" );
            resource::RuntimeResourceRangeInfo info;
            require( mount->resourceInfo( value->header.resourceId, info, error ),
                     "Texture snapshot is outside its captured package mount" );
            require( info.header.resourceId == value->header.resourceId &&
                         info.header.resourceType == value->header.resourceType &&
                         info.header.sourceHash == value->header.sourceHash &&
                         info.header.payloadHash == value->header.payloadHash &&
                         info.header.payloadSize == value->header.payloadSize &&
                         info.header.compilerVersion == value->header.compilerVersion,
                     "Texture snapshot does not match its captured package generation" );
            // Whole-container compression has no independently decodable mip ranges.
            if( value->payload.size() >= 4 && !std::memcmp( value->payload.data(), "WPZ1", 4 ) )
            {
                return value;
            }
            const auto parsed = parse( *value );
            const auto first = bootstrapFirst( parsed );
            const auto bootstrapBytes = bootstrapSize( parsed );
            if( bootstrapBytes > maximumBootstrap )
            {
                return value;  // Explicit legacy/no-mip fallback.
            }
            auto lease =
                mount->reserveOwner( StreamMemoryClass::SourceCpu, 8192 + bootstrapBytes, error );
            if( !lease )
            {
                return {};
            }
            auto plan = std::make_shared<Plan>();
            plan->budget = lease;
            plan->id = value->header.resourceId;
            plan->sourceHash = value->header.sourceHash;
            plan->payloadHash = value->header.payloadHash;
            plan->payloadSize = value->header.payloadSize;
            plan->compilerVersion = value->header.compilerVersion;
            plan->target = parsed.target;
            plan->packaged = parsed.packaged;
            plan->settings = parsed.settings;
            plan->count = parsed.count;
            plan->format = parsed.format;
            plan->mips = parsed.mips;
            plan->bootstrapFirst = first;
            plan->independentlyVerified = info.independentlyVerified;
            plan->bootstrap.assign( value->payload.data() + parsed.mips[first].offset,
                                    value->payload.data() + parsed.mips[first].offset + bootstrapBytes );

            struct Owner
            {
                StreamBudget budget;
                resource::RuntimeResource value;
            };

            auto snapshot = std::make_shared<Owner>();
            snapshot->budget = std::move( lease );
            snapshot->value.header = value->header;
            snapshot->value.streamData = plan;
            return std::shared_ptr<const resource::RuntimeResource>( snapshot, &snapshot->value );
        }
        catch( const std::exception &failure )
        {
            error = failure.what();
            return {};
        }
    }

    namespace
    {
        Parsed parseColdPrefix( const resource::RuntimeResourceChunk &prefix, u64 logicalSize,
                                bool &legacy, u64 &decodeBytes )
        {
            require( prefix.offset == 0 && prefix.bytes.size() >= 20, "Truncated cold texture prefix" );
            if( !std::memcmp( prefix.bytes.data(), "WPZ1", 4 ) )
            {
                Reader compressed{ prefix.bytes.data(), prefix.bytes.size(), 4 };
                decodeBytes = compressed.wide();
                require( decodeBytes && decodeBytes <= graphicsTextureByteLimit + 4096,
                         "Compressed cold texture exceeds its decode limit" );
                legacy = true;
                return {};
            }
            Parsed out;
            Reader reader{ prefix.bytes.data(), prefix.bytes.size() };
            const auto magic = reader.word(), version = reader.word();
            require( ( magic == 0x58545057 || magic == 0x58445057 ) && ( version == 1 || version == 2 ),
                     "Unsupported cold texture encoding" );
            const auto length = reader.word();
            require( length && length <= 128 && length <= reader.size - reader.at,
                     "Invalid cold texture target" );
            std::memcpy( out.target, reader.bytes + reader.at, length );
            reader.skip( length );
            for( u32 i = 0; i < length; ++i )
            {
                require( out.target[i] >= 33 && out.target[i] <= 126,
                         "Cold texture target must be printable ASCII" );
            }
            const auto mode = reader.word();
            require( mode <= 1, "Invalid cold texture build mode" );
            out.packaged = mode != 0;
            const auto filter = reader.word();
            require( filter <= static_cast<u32>( TextureMipFilter::Roughness ),
                     "Invalid cold texture mip filter" );
            out.settings.filter = static_cast<TextureMipFilter>( filter );
            out.settings.atlasColumns = reader.word();
            out.settings.alphaCutoff = reader.number();
            require( out.settings.atlasColumns && out.settings.atlasColumns <= 16384 &&
                         std::isfinite( out.settings.alphaCutoff ) && out.settings.alphaCutoff > 0 &&
                         out.settings.alphaCutoff < 1,
                     "Invalid cold texture mip settings" );
            const auto storage = version == 2 ? reader.word() : 0;
            u32 width = 0, height = 0;
            u64 offset = 0;
            if( magic == 0x58545057 )
            {
                require( !storage && !ddsFormatForTarget( out.target ),
                         "Portable texture cannot use a native DDS profile" );
                out.count = reader.word();
                width = reader.word();
                height = reader.word();
                const auto bytes = reader.wide();
                require( bytes == u64( width ) * height * 4, "Invalid cold texture base mip length" );
                offset = reader.at;
            }
            else
            {
                const auto ddsBytes = reader.wide(), base = reader.at;
                require( ddsBytes == logicalSize - base && reader.size - base >= 148,
                         "Invalid cold DDS extent" );
                const auto word = [&]( u64 at ) {
                    Reader header{ reader.bytes + base, reader.size - base, at };
                    return header.word();
                };
                require( word( 0 ) == 0x20534444 && word( 4 ) == 124 && word( 76 ) == 32 &&
                             word( 80 ) == 4 && word( 84 ) == 0x30315844 && word( 24 ) == 0 &&
                             word( 112 ) == 0 && word( 116 ) == 0 && word( 120 ) == 0 &&
                             word( 132 ) == 3 && word( 136 ) == 0 && word( 140 ) == 1,
                         "Unsupported cold DDS topology" );
                const auto flags = word( 8 ), caps = word( 108 );
                require( ( flags & 0x1007 ) == 0x1007 &&
                             !( flags & ~( 0x1007u | 8u | 0x80000u | 0x20000u ) ) && ( caps & 0x1000 ) &&
                             !( caps & ~( 0x1000u | 8u | 0x400000u ) ),
                         "Unsupported cold DDS flags" );
                out.format = word( 128 );
                require( dds::supported( out.format ) &&
                             out.format == ( storage ? storage : ddsFormatForTarget( out.target ) ),
                         "Cold DDS format/profile mismatch" );
                require( !storage || !std::strncmp( out.target, "windows", 7 ) ||
                             !std::strncmp( out.target, "dx11", 4 ) ||
                             !std::strncmp( out.target, "pc", 2 ) ||
                             !std::strncmp( out.target, "mac", 3 ),
                         "Cold DDS encoding is incompatible with target" );
                width = word( 16 );
                height = word( 12 );
                out.count = word( 28 );
                const auto block = dds::blockBytes( out.format );
                require( word( 144 ) == ( block == 8 ? 3u : 1u ), "Cold DDS alpha mode is invalid" );
                require( out.count == 1 || ( ( flags & 0x20000 ) && ( caps & 0x400008 ) == 0x400008 ),
                         "Cold DDS mip flags are missing" );
                require( block ? ( ( flags & 0x80000 ) && !( flags & 8 ) )
                               : ( ( flags & 8 ) && !( flags & 0x80000 ) ),
                         "Cold DDS pitch flags are inconsistent" );
                const u64 row = block ? u64( ( width + 3 ) / 4 ) * block : u64( width ) * 4;
                require( word( 20 ) == ( block ? row * ( ( height + 3 ) / 4 ) : row ),
                         "Cold DDS pitch is invalid" );
                const bool srgb =
                    out.format == 72 || out.format == 78 || out.format == 91 || out.format == 99;
                require( !block || ( width % 4 == 0 && height % 4 == 0 &&
                                     out.settings.filter != TextureMipFilter::Normal ),
                         "Unsupported compressed cold normal map/dimensions" );
                require( !srgb || out.settings.filter == TextureMipFilter::Colour ||
                             out.settings.filter == TextureMipFilter::Cutout ||
                             out.settings.filter == TextureMipFilter::None,
                         "Cold SRGB mip semantics are inconsistent" );
                offset = base + 148;
            }
            require( width && height && width <= 16384 && height <= 16384 &&
                         width % out.settings.atlasColumns == 0 && out.count && out.count <= 15,
                     "Invalid cold texture dimensions/mip count" );
            u64 total = 0;
            for( u32 i = 0; i < out.count; ++i )
            {
                auto &mip = out.mips[i];
                mip.width = width;
                mip.height = height;
                mip.offset = offset;
                const auto block = dds::blockBytes( out.format );
                mip.size = out.format ? u64( block ? ( ( width + 3 ) / 4 ) * block : width * 4 ) *
                                            ( block ? ( height + 3 ) / 4 : height )
                                      : u64( width ) * height * 4;
                require( mip.size <= graphicsTextureByteLimit - total && offset <= logicalSize &&
                             mip.size <= logicalSize - offset,
                         "Cold mip span exceeds captured payload" );
                total += mip.size;
                offset += mip.size;
                u32 nextWidth = 0, nextHeight = 0;
                const bool last =
                    out.settings.filter == TextureMipFilter::None ||
                    !wp_graphics_texture_next_mip_extent( width, height, out.settings.atlasColumns,
                                                         &nextWidth, &nextHeight );
                require( last == ( i + 1 == out.count ),
                         "Cold mip chain is incomplete or has extra levels" );
                if( i + 1 < out.count && !out.format )
                {
                    offset += 16;
                }
                width = nextWidth;
                height = nextHeight;
            }
            require( offset == logicalSize, "Cold texture has trailing or missing bytes" );
            decodeBytes = logicalSize;
            legacy = bootstrapSize( out ) > maximumBootstrap;
            return out;
        }

        class ColdTextureProvider final : public resource::ResourceLeafProvider
        {
            StreamBudget m_budget, m_planBudget;
            std::shared_ptr<resource::ResourcePayloadMount> m_mount;
            resource::CompiledResourceHeader m_header;
            std::unique_ptr<resource::ResourceChunkStream> m_stream;
            std::shared_ptr<Plan> m_plan;
            std::shared_ptr<const resource::RuntimeResource> m_ready;
            std::array<bool, maximumMips> m_headers{};
            u64 m_maximum = 8192 + maximumBootstrap, m_decodeBytes = 0;
            s32 m_priority = 0;
            u32 m_stage = 0;
            bool m_legacy = false, m_bootstrap = false, m_cancelled = false;

            void headerPlan( const Parsed &parsed )
            {
                m_plan = std::make_shared<Plan>();
                m_plan->budget = m_planBudget;
                m_plan->id = m_header.resourceId;
                m_plan->sourceHash = m_header.sourceHash;
                m_plan->payloadHash = m_header.payloadHash;
                m_plan->payloadSize = m_header.payloadSize;
                m_plan->compilerVersion = m_header.compilerVersion;
                m_plan->independentlyVerified = true;
                m_plan->target = parsed.target;
                m_plan->packaged = parsed.packaged;
                m_plan->settings = parsed.settings;
                m_plan->format = parsed.format;
                m_plan->count = parsed.count;
                m_plan->mips = parsed.mips;
                m_plan->bootstrapFirst = bootstrapFirst( parsed );
            }

        public:
            ColdTextureProvider( StreamBudget budget,
                                 std::shared_ptr<resource::ResourcePayloadMount> mount,
                                 const resource::CompiledResourceHeader &header, s32 priority ) :
                m_budget( std::move( budget ) ),
                m_mount( std::move( mount ) ),
                m_header( header ),
                m_priority( priority )
            {
            }

            ~ColdTextureProvider() override
            {
                drain();
            }

            u64 maximumResidentBytes() const override
            {
                return m_maximum;
            }

            void setPriority( s32 priority ) override
            {
                m_priority = priority;
                if( m_stream )
                {
                    for( u64 key = 1; key < 32; ++key )
                    {
                        m_stream->setPriority( key, priority );
                    }
                }
            }

            void cancel() override
            {
                m_cancelled = true;
                try
                {
                    if( m_stream )
                    {
                        m_stream->cancel();
                    }
                }
                catch( ... )
                {
                }
            }

            void drain() override
            {
                try
                {
                    if( m_stream )
                    {
                        m_stream->retireAfterOwnerQuiesced();
                        m_stream.reset();
                    }
                }
                catch( ... )
                {
                }
            }

            resource::ResourceLeafStatus poll( std::shared_ptr<const resource::RuntimeResource> &result,
                                               String &error ) override
            {
                result.reset();
                error.clear();
                try
                {
                    if( m_cancelled )
                    {
                        throw std::runtime_error( "Cold texture request was cancelled" );
                    }
                    if( m_ready )
                    {
                        result = m_ready;
                        return resource::ResourceLeafStatus::Ready;
                    }
                    if( !m_stream )
                    {
                        m_stream = resource::ResourceChunkStream::create( m_mount, error );
                        require( m_stream != nullptr, "Cannot create cold texture range stream" );
                    }
                    if( m_stage == 0 )
                    {
                        require( m_stream->request( 1, m_header.resourceId, 0,
                                                    std::min<u64>( 4096, m_header.payloadSize ),
                                                    m_priority, m_priority, error ),
                                 "Cannot request cold texture prefix" );
                        m_stage = 1;
                        return resource::ResourceLeafStatus::Pending;
                    }
                    if( m_stage == 3 )
                    {  // Core has admitted the increased whole-payload estimate on this later poll.
                        require( m_stream->request( 2, m_header.resourceId, 0, m_header.payloadSize,
                                                    m_priority, m_priority, error ),
                                 "Cannot request checked legacy texture payload" );
                        m_stage = 4;
                        return resource::ResourceLeafStatus::Pending;
                    }
                    m_stream->process();
                    resource::ResourceChunkResult ready;
                    while( m_stream->take( ready ) )
                    {
                        require(
                            ready.resource && ready.resourceId == m_header.resourceId,
                            ready.error.empty() ? "Cold texture range failed" : ready.error.c_str() );
                        if( ready.key == 1 )
                        {
                            const auto parsed = parseColdPrefix( *ready.resource, m_header.payloadSize,
                                                                 m_legacy, m_decodeBytes );
                            if( m_legacy )
                            {
                                m_maximum = m_header.payloadSize + 8192;
                                m_stage = 3;
                                return resource::ResourceLeafStatus::Pending;
                            }
                            m_planBudget = m_mount->reserveOwner(
                                StreamMemoryClass::SourceCpu, 8192 + bootstrapSize( parsed ), error );
                            require( bool( m_planBudget ), "Cold texture plan admission failed" );
                            headerPlan( parsed );
                            m_headers.fill( false );
                            m_headers[0] = true;
                            for( u32 i = 1; i < parsed.count && !parsed.format; ++i )
                            {
                                require( m_stream->request( 10 + i, m_header.resourceId,
                                                            parsed.mips[i].offset - 16, 16, m_priority,
                                                            m_priority, error ),
                                         "Cannot request cold mip header" );
                            }
                            if( parsed.format )
                            {
                                m_headers.fill( true );
                            }
                            const auto &tail = parsed.mips[m_plan->bootstrapFirst];
                            require( m_stream->request( 2, m_header.resourceId, tail.offset,
                                                        bootstrapSize( parsed ), m_priority, m_priority,
                                                        error ),
                                     "Cannot request cold texture bootstrap" );
                            m_stage = 2;
                        }
                        else if( ready.key == 2 && m_stage == 4 )
                        {
                            require( ready.resource->bytes.size() == m_header.payloadSize &&
                                         resource::hashBytes( ready.resource->bytes.data(),
                                                              ready.resource->bytes.size() ) ==
                                             m_header.payloadHash,
                                     "Checked legacy texture integrity mismatch" );
                            auto budget = m_mount->reserveOwner( StreamMemoryClass::SourceCpu,
                                                                 m_header.payloadSize + 8192, error );
                            require( bool( budget ), "Legacy texture residency admission failed" );

                            struct Owner
                            {
                                StreamBudget budget;
                                resource::RuntimeResource value;
                            };

                            auto owner = std::make_shared<Owner>();
                            owner->budget = std::move( budget );
                            owner->value.header = m_header;
                            owner->value.payload = ready.resource->bytes;
                            auto scratch = m_mount->reserveOwner(
                                StreamMemoryClass::WorkingCpu,
                                2 * m_decodeBytes + m_header.payloadSize + 65536, error );
                            require( bool( scratch ), "Legacy texture decode scratch admission failed" );
                            CookedTextureData data;
                            require( decodeCookedTexture( owner->value, data, error ), error.c_str() );
                            m_planBudget =
                                m_mount->reserveOwner( StreamMemoryClass::SourceCpu, 8192, error );
                            require( bool( m_planBudget ),
                                     "Legacy texture descriptor admission failed" );
                            Parsed parsed;
                            headerPlan( parsed );
                            m_plan->target = data.target;
                            m_plan->packaged = data.packagedBuild;
                            m_plan->settings = data.mipSettings;
                            m_plan->format = data.storageFormat;
                            m_plan->wholeLegacy = true;
                            owner->value.streamData = m_plan;
                            m_ready =
                                std::shared_ptr<const resource::RuntimeResource>( owner, &owner->value );
                        }
                        else if( ready.key == 2 )
                        {
                            const auto &tail = m_plan->mips[m_plan->bootstrapFirst];
                            const auto &last = m_plan->mips[m_plan->count - 1];
                            require(
                                ready.offset == tail.offset && ready.resource->bytes.size() ==
                                                                   last.offset + last.size - tail.offset,
                                "Cold texture bootstrap extent mismatch" );
                            m_plan->bootstrap.assign( ready.resource->bytes.begin(),
                                                      ready.resource->bytes.end() );
                            m_bootstrap = true;
                        }
                        else if( ready.key > 10 && ready.key < 10 + m_plan->count )
                        {
                            const auto index = static_cast<u32>( ready.key - 10 );
                            const auto &mip = m_plan->mips[index];
                            Reader header{ ready.resource->bytes.data(), ready.resource->bytes.size() };
                            require( ready.offset == mip.offset - 16 && header.word() == mip.width &&
                                         header.word() == mip.height && header.wide() == mip.size &&
                                         header.at == header.size,
                                     "Cold texture mip header mismatch" );
                            m_headers[index] = true;
                        }
                    }
                    if( m_plan && !m_legacy && m_bootstrap )
                    {
                        bool complete = true;
                        for( u32 i = 0; i < m_plan->count; ++i )
                        {
                            complete &= m_headers[i];
                        }
                        if( complete )
                        {
                            struct Owner
                            {
                                StreamBudget budget;
                                resource::RuntimeResource value;
                            };

                            auto owner = std::make_shared<Owner>();
                            owner->budget = m_planBudget;
                            owner->value.header = m_header;
                            owner->value.streamData = m_plan;
                            m_ready =
                                std::shared_ptr<const resource::RuntimeResource>( owner, &owner->value );
                        }
                    }
                    if( m_ready )
                    {
                        result = m_ready;
                        return resource::ResourceLeafStatus::Ready;
                    }
                    return resource::ResourceLeafStatus::Pending;
                }
                catch( const std::exception &failure )
                {
                    error = failure.what();
                    return resource::ResourceLeafStatus::Error;
                }
            }
        };

        class ColdTextureFactory final : public resource::ResourceLeafProviderFactory
        {
            StreamBudget m_budget;

        public:
            explicit ColdTextureFactory( StreamBudget budget ) : m_budget( std::move( budget ) )
            {
            }

            bool handles( const resource::ResourceID &id ) const override
            {
                return id.type() == resource::ResourceTypeID( "texres" );
            }

            std::shared_ptr<resource::ResourceLeafProvider> create(
                std::shared_ptr<resource::ResourcePayloadMount> mount, const resource::ResourceID &id,
                const resource::RuntimeResourceRangeInfo &info, s32 priority, String &error ) override
            {
                error.clear();
                try
                {
                    const GraphicsResourceCompiler compiler;
                    require( mount && info.independentlyVerified && id == info.header.resourceId &&
                                 id.type() == resource::ResourceTypeID( "texres" ) &&
                                 info.header.resourceType == id.type() && !id.isSubResource() &&
                                 info.header.isValid() && info.header.installDependencies.empty() &&
                                 info.header.payloadSize >= 20 &&
                                 info.header.payloadSize <= graphicsTextureByteLimit + 4096 &&
                                 ( info.header.compilerVersion == 2 ||
                                   info.header.compilerVersion == compiler.versionFor( id.type() ) ),
                             "Unsupported cold texture identity/version/dependencies" );
                    auto budget = mount->reserveOwner(
                        StreamMemoryClass::SourceCpu,
                        sizeof( ColdTextureProvider ) + info.metadataBytes + 8192, error );
                    if( !budget )
                    {
                        return {};
                    }
                    return std::make_shared<ColdTextureProvider>(
                        std::move( budget ), std::move( mount ), info.header, priority );
                }
                catch( const std::exception &failure )
                {
                    error = failure.what();
                    return {};
                }
            }
        };
    }  // namespace

    std::shared_ptr<resource::ResourceLeafProviderFactory> createRuntimeTextureLeafProviderFactory(
        std::shared_ptr<resource::ResourcePayloadMount> mount, String &error )
    {
        error.clear();
        try
        {
            require( mount != nullptr, "Cold texture factory requires its captured package mount" );
            auto budget = mount->reserveOwner( StreamMemoryClass::SourceCpu,
                                               sizeof( ColdTextureFactory ) + 1024, error );
            if( !budget )
            {
                return {};
            }
            return std::make_shared<ColdTextureFactory>( std::move( budget ) );
        }
        catch( const std::exception &failure )
        {
            error = failure.what();
            return {};
        }
    }

    struct ClawTextureStream::Data
    {
        StreamBudget budget;
        mutable std::mutex mutex;
        std::shared_ptr<const Plan> plan;
        std::shared_ptr<resource::ResourcePayloadMount> mount;
        std::unique_ptr<resource::ResourceChunkStream> stream;
        std::thread::id owner;
        std::array<std::shared_ptr<const resource::RuntimeResourceChunk>, maximumMips> ready{};
        std::array<bool, maximumMips> pending{};
        u32 first = 0, desired = 0;
        s32 priority = 0, residencyPriority = 0;
        bool dirty = true, failed = false, manual = false, priorityDirty = false;
        u32 policyKey = std::numeric_limits<u32>::max();
        void *fallback = nullptr, *current = nullptr, *device = nullptr;
        u64 gpuBytes = 0;

        ~Data()
        {
            retire();
            releaseViews();
        }

        void retire()
        {
            if( stream )
            {
                stream->retireAfterOwnerQuiesced();
                stream.reset();
            }
            pending.fill( false );
        }

        void releaseViews()
        {
            if( current && current != fallback )
            {
                wp_renderer_dx11_destroy_texture_native( current );
            }
            if( fallback )
            {
                wp_renderer_dx11_destroy_texture_native( fallback );
            }
            current = fallback = device = nullptr;
            gpuBytes = 0;
        }

        const u8 *bytes( u32 index ) const
        {
            return index >= plan->bootstrapFirst
                       ? plan->bootstrap.data() +
                             ( plan->mips[index].offset - plan->mips[plan->bootstrapFirst].offset )
                   : ready[index] ? ready[index]->bytes.data()
                                  : nullptr;
        }

        void *upload( wp_renderer_dx11 *renderer, u32 begin, String &error )
        {
            u64 size = 0;
            for( u32 i = begin; i < plan->count; ++i )
            {
                require( bytes( i ) != nullptr, "Streamed mip bytes are unavailable" );
                size += plan->mips[i].size;
            }
            auto gpu = mount->reserveOwner( StreamMemoryClass::Gpu, size, error );
            if( !gpu )
            {
                return nullptr;
            }
            void *view = nullptr;
            if( plan->format )
            {
                std::array<wp_texture_encoded_mip_dx11, maximumMips> levels{};
                for( u32 i = begin; i < plan->count; ++i )
                {
                    const auto &m = plan->mips[i];
                    levels[i - begin] = { bytes( i ), m.width, m.height, static_cast<wp_u32>( m.size ) };
                }
                view = wp_renderer_dx11_create_texture_encoded_mips_native(
                    renderer, levels.data(), plan->count - begin, plan->format );
            }
            else
            {
                std::array<wp_texture_mip_dx11, maximumMips> levels{};
                for( u32 i = begin; i < plan->count; ++i )
                {
                    const auto &m = plan->mips[i];
                    levels[i - begin] = { bytes( i ), m.width, m.height };
                }
                view = wp_renderer_dx11_create_texture_mips_native( renderer, levels.data(),
                                                                    plan->count - begin );
            }
            if( view && !asset_budget::retainTexture( view, std::move( gpu ) ) )
            {
                wp_renderer_dx11_destroy_texture_native( view );
                view = nullptr;
            }
            if( !view && error.empty() )
            {
                error = "Native texture mip-tail upload failed";
            }
            return view;
        }

        bool ensureFallback( wp_renderer_dx11 *renderer, String &error )
        {
            const auto nextDevice = renderer ? wp_renderer_dx11_get_device( renderer ) : nullptr;
            if( !nextDevice )
            {
                error = "Runtime texture stream requires an active DX11 device";
                return false;
            }
            if( device == nextDevice && fallback )
            {
                return true;
            }
            auto candidate = upload( renderer, plan->bootstrapFirst, error );
            if( !candidate )
            {
                return false;
            }
            releaseViews();
            device = nextDevice;
            current = fallback = candidate;
            first = plan->bootstrapFirst;
            gpuBytes = plan->bootstrap.size();
            return true;
        }

        bool ensureOwner( String &error )
        {
            const auto next = std::this_thread::get_id();
            if( stream && owner == next )
            {
                return true;
            }
            retire();
            auto local = mount->createReadOnlyOwnerView( error );
            if( !local )
            {
                return false;
            }
            stream = resource::ResourceChunkStream::create( std::move( local ), error );
            if( !stream )
            {
                return false;
            }
            owner = next;
            dirty = true;
            return true;
        }

        bool process( wp_renderer_dx11 *renderer, String &error )
        {
            error.clear();
            if( !ensureFallback( renderer, error ) || !ensureOwner( error ) )
            {
                return false;
            }
            if( dirty )
            {
                stream->cancel();
                pending.fill( false );
                failed = false;
                for( u32 i = 0; i < desired; ++i )
                {
                    ready[i].reset();
                }
                dirty = false;
            }
            if( priorityDirty )
            {
                for( u32 i = 0; i < plan->count; ++i )
                {
                    if( pending[i] )
                    {
                        stream->setPriority( i + 1, priority );
                        stream->setResidencyPriority( i + 1, residencyPriority );
                    }
                }
                priorityDirty = false;
            }
            if( desired == plan->bootstrapFirst )
            {
                if( current != fallback )
                {
                    wp_renderer_dx11_destroy_texture_native( current );
                }
                current = fallback;
                first = desired;
                gpuBytes = plan->bootstrap.size();
                for( auto &mip : ready )
                {
                    mip.reset();
                }
                return true;
            }
            if( failed )
            {
                return true;
            }
            for( u32 i = desired; i < plan->bootstrapFirst; ++i )
            {
                if( !ready[i] && !pending[i] )
                {
                    const auto &mip = plan->mips[i];
                    if( !stream->request( i + 1, plan->id, mip.offset, mip.size, priority,
                                          residencyPriority, error ) )
                    {
                        return false;
                    }
                    pending[i] = true;
                }
            }
            stream->process();
            resource::ResourceChunkResult result;
            while( stream->take( result ) )
            {
                if( !result.key || result.key >= plan->count )
                {
                    continue;
                }
                const auto i = static_cast<u32>( result.key - 1 );
                pending[i] = false;
                const auto &mip = plan->mips[i];
                if( i < desired )
                {
                    continue;
                }
                if( !result.resource || result.resourceId != plan->id || result.offset != mip.offset ||
                    result.resource->bytes.size() != mip.size ||
                    ( mip.hash && resource::hashBytes( result.resource->bytes.data(),
                                                       result.resource->bytes.size() ) != mip.hash ) )
                {
                    failed = true;
                    error = result.error.empty() ? "Streamed texture mip integrity/identity failure"
                                                 : result.error;
                    stream->cancel();
                    pending.fill( false );
                    for( u32 fine = 0; fine < first; ++fine )
                    {
                        ready[fine].reset();
                    }
                    return false;
                }
                ready[i] = std::move( result.resource );
            }
            for( u32 i = desired; i < plan->bootstrapFirst; ++i )
            {
                if( !ready[i] )
                {
                    return true;
                }
            }
            if( first == desired && current )
            {
                return true;
            }
            auto candidate = upload( renderer, desired, error );
            if( !candidate )
            {
                return false;
            }
            if( current && current != fallback )
            {
                wp_renderer_dx11_destroy_texture_native( current );
            }
            current = candidate;
            first = desired;
            gpuBytes = plan->bootstrap.size();
            for( u32 i = first; i < plan->count; ++i )
            {
                gpuBytes += plan->mips[i].size;
            }
            return true;
        }
    };

    ClawTextureStream::ClawTextureStream( std::unique_ptr<Data> data ) : m_data( std::move( data ) )
    {
    }

    ClawTextureStream::~ClawTextureStream() = default;

    std::unique_ptr<ClawTextureStream> ClawTextureStream::create(
        std::shared_ptr<const resource::RuntimeResource> value,
        std::shared_ptr<resource::ResourcePayloadMount> mount, const String &target, bool packaged,
        wp_renderer_dx11 *renderer, String &error )
    {
        error.clear();
        try
        {
            require( value && mount, "Runtime texture stream requires a captured resource/mount" );
            auto plan = std::dynamic_pointer_cast<const Plan>( value->streamData );
            require( plan && !plan->wholeLegacy && matches( *plan, *value ) && plan->target == target &&
                         plan->packaged == packaged,
                     "Runtime texture plan identity/target/build mismatch" );
            resource::RuntimeResourceRangeInfo pin;
            require( mount->resourceInfo( plan->id, pin, error ),
                     "Runtime texture plan is outside its mount" );
            require( pin.header.sourceHash == plan->sourceHash &&
                         pin.header.payloadHash == plan->payloadHash &&
                         pin.header.payloadSize == plan->payloadSize &&
                         pin.header.compilerVersion == plan->compilerVersion &&
                         ( !plan->independentlyVerified || pin.independentlyVerified ),
                     "Runtime texture stream mount generation does not match its plan" );
            auto budget =
                mount->reserveOwner( StreamMemoryClass::SourceCpu, sizeof( Data ) + 4096, error );
            if( !budget )
            {
                return {};
            }
            auto data = std::make_unique<Data>();
            data->budget = std::move( budget );
            data->plan = std::move( plan );
            data->mount = std::move( mount );
            data->first = data->desired = data->plan->bootstrapFirst;
            if( !data->ensureFallback( renderer, error ) )
            {
                return {};
            }
            return std::unique_ptr<ClawTextureStream>( new ClawTextureStream( std::move( data ) ) );
        }
        catch( const std::exception &failure )
        {
            error = failure.what();
            return {};
        }
    }

    bool ClawTextureStream::setDemand( u32 first, s32 priority, s32 residencyPriority, String &error )
    {
        error.clear();
        std::lock_guard lock( m_data->mutex );
        if( first >= m_data->plan->count )
        {
            error = "Runtime texture mip demand is outside its plan";
            return false;
        }
        first = std::min( first, m_data->plan->bootstrapFirst );
        if( m_data->desired != first || m_data->failed )
        {
            m_data->dirty = true;
        }
        if( m_data->priority != priority || m_data->residencyPriority != residencyPriority )
        {
            m_data->priorityDirty = true;
        }
        m_data->desired = first;
        m_data->priority = priority;
        m_data->residencyPriority = residencyPriority;
        m_data->manual = true;
        return true;
    }

    bool ClawTextureStream::process( wp_renderer_dx11 *renderer, String &error )
    {
        std::lock_guard lock( m_data->mutex );
        try
        {
            return m_data->process( renderer, error );
        }
        catch( const std::exception &failure )
        {
            error = failure.what();
            return false;
        }
    }

    void *ClawTextureStream::view( wp_renderer_dx11 *renderer, s32 quality, u32 maximum, String &error )
    {
        std::lock_guard lock( m_data->mutex );
        try
        {
            const auto key = static_cast<u32>( quality + 1 ) * 65536u + maximum;
            if( key != m_data->policyKey )
            {
                const bool initialize = m_data->policyKey == std::numeric_limits<u32>::max();
                m_data->policyKey = key;
                if( !initialize || !m_data->manual )
                {
                    const auto &base = m_data->plan->mips[0];
                    const auto first = wp_graphics_texture_first_mip(
                        base.width, base.height, m_data->plan->count, quality, maximum );
                    m_data->desired = std::min( first, m_data->plan->bootstrapFirst );
                    m_data->dirty = true;
                }
            }
            m_data->process( renderer, error );
            return renderer && m_data->device == wp_renderer_dx11_get_device( renderer )
                       ? m_data->current
                       : nullptr;
        }
        catch( const std::exception &failure )
        {
            error = failure.what();
            return renderer && m_data->device == wp_renderer_dx11_get_device( renderer )
                       ? m_data->current
                       : nullptr;
        }
    }

    void ClawTextureStream::unload()
    {
        std::lock_guard lock( m_data->mutex );
        m_data->retire();
        m_data->releaseViews();
        for( auto &mip : m_data->ready )
        {
            mip.reset();
        }
        m_data->first = m_data->desired = m_data->plan->bootstrapFirst;
        m_data->dirty = true;
        m_data->failed = false;
        m_data->policyKey = std::numeric_limits<u32>::max();
        m_data->manual = false;
    }

    Vector2I ClawTextureStream::originalSize() const
    {
        return { static_cast<s32>( m_data->plan->mips[0].width ),
                 static_cast<s32>( m_data->plan->mips[0].height ) };
    }

    Vector2I ClawTextureStream::residentSize() const
    {
        std::lock_guard lock( m_data->mutex );
        const auto &mip = m_data->plan->mips[m_data->first];
        return { static_cast<s32>( mip.width ), static_cast<s32>( mip.height ) };
    }

    RuntimeTextureStreamState ClawTextureStream::state() const
    {
        std::lock_guard lock( m_data->mutex );
        RuntimeTextureStreamState state;
        state.firstMip = m_data->first;
        state.requestedMip = m_data->desired;
        state.mipCount = m_data->plan->count;
        state.residentGpuBytes = m_data->gpuBytes;
        state.residentCpuBytes = m_data->plan->bootstrap.size();
        state.failed = m_data->failed;
        for( u32 i = 0; i < m_data->plan->count; ++i )
        {
            if( m_data->ready[i] )
            {
                state.residentCpuBytes += m_data->ready[i]->bytes.size();
            }
            state.pending |= m_data->pending[i];
        }
        return state;
    }
}  // namespace workphone::render
