#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/ProceduralCityStreaming.hpp>
#include <Workphone/Procedural/ProceduralSceneGeneration.hpp>
#include <Workphone/Scene/Components/Collision.hpp>
#include <Workphone/Scene/Components/LODGroup.hpp>
#include <Workphone/Scene/Components/Mesh.hpp>
#include <Workphone/Scene/Components/MeshImposter.hpp>
#include <Workphone/Scene/Components/MeshRenderer.hpp>
#include <Workphone/Scene/Components/Rigidbody.hpp>
#include <Workphone/Scene/Systems/LODSystem.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/IO/FileDataStream.hpp>
#include <Workphone/IO/MemoryFile.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsObject.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Mesh/IIndexBuffer.hpp>
#include <Workphone/Interface/Mesh/IMeshResource.hpp>
#include <Workphone/Interface/Mesh/ISubMesh.hpp>
#include <Workphone/Interface/Mesh/IVertexBuffer.hpp>
#include <Workphone/Interface/Mesh/IVertexDeclaration.hpp>
#include <Workphone/Interface/Mesh/IVertexElement.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Mesh/Mesh.hpp>
#include <Workphone/Mesh/MeshSerializer.hpp>
#include <Workphone/System/SourceBudgetContainers.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace workphone::scene::procedural_scene
{
    namespace
    {
        namespace fs = std::filesystem;
        using Clock = std::chrono::steady_clock;
        constexpr size_t ioChunkSize = 64u << 10;
        constexpr u64 hashStart = 14695981039346656037ull;

        struct CachedMesh
        {
            fs::path path;
            u64 length = 0, digest = 0;
            u64 geometryDigest = 0, sourceBytes = 0;
        };

        struct CachedPatch
        {
            SourceBudgetVector<CachedMesh> files;
            u64 length = 0, largestFile = 0;
        };

        struct DecodedPatch
        {
            SourceBudgetVector<SmartPtr<IMesh>> meshes;
        };

        u64 appendDigest( u64 value, const u8 *bytes, size_t size )
        {
            for( size_t i = 0; i < size; ++i )
            {
                value = ( value ^ bytes[i] ) * 1099511628211ull;
            }
            return value;
        }

        struct MeshFingerprint
        {
            u64 digest = hashStart, bytes = 0;

            void field( u64 value )
            {
                digest = appendDigest( digest, reinterpret_cast<const u8 *>( &value ), sizeof( value ) );
            }

            void data( const void *data, u64 size )
            {
                if( size > std::numeric_limits<size_t>::max() || ( size && !data ) )
                {
                    throw std::runtime_error( "Invalid city stream geometry buffer." );
                }
                field( size );
                digest =
                    appendDigest( digest, static_cast<const u8 *>( data ), static_cast<size_t>( size ) );
                bytes += size;
            }

            void vertices( const SmartPtr<IVertexBuffer> &buffer )
            {
                if( !buffer )
                {
                    throw std::runtime_error( "Missing city stream vertex buffer." );
                }
                const auto declaration = buffer->getVertexDeclaration();
                if( !declaration )
                {
                    throw std::runtime_error( "Missing city stream vertex declaration." );
                }
                const auto &elements = declaration->getElements();
                field( elements.size() );
                for( const auto &element : elements )
                {
                    if( !element )
                    {
                        throw std::runtime_error( "Invalid city stream vertex element." );
                    }
                    field( element->getSource() );
                    field( element->getOffset() );
                    field( element->getSemantic() );
                    field( static_cast<u64>( element->getType() ) );
                    field( element->getIndex() );
                }
                field( buffer->getNumVertices() );
                const auto streams = buffer->getDataArray();
                for( size_t i = 0; i < streams.size(); ++i )
                {
                    if( !streams[i] )
                    {
                        continue;
                    }
                    field( i );
                    data( streams[i], u64( buffer->getNumVertices() ) *
                                          declaration->getSize( static_cast<u16>( i ) ) );
                }
            }
        };

        MeshFingerprint fingerprint( const IMesh &mesh )
        {
            MeshFingerprint value;
            value.field( mesh.getHasSharedVertexData() );
            if( mesh.getHasSharedVertexData() )
            {
                value.vertices( mesh.getSharedVertexBuffer() );
            }
            const auto submeshes = mesh.getSubMeshes();
            value.field( submeshes.size() );
            for( const auto &part : submeshes )
            {
                if( !part )
                {
                    throw std::runtime_error( "Missing city stream submesh." );
                }
                value.field( static_cast<u64>( part->getRenderOperationType() ) );
                value.field( part->getUseSharedVertices() );
                if( !part->getUseSharedVertices() )
                {
                    value.vertices( part->getVertexBuffer() );
                }
                const auto indices = part->getIndexBuffer();
                if( !indices )
                {
                    throw std::runtime_error( "Missing city stream index buffer." );
                }
                value.field( indices->getNumIndices() );
                value.field( indices->getIndexSize() );
                value.data( indices->getIndexData(),
                            u64( indices->getNumIndices() ) * indices->getIndexSize() );
            }
            return value;
        }

        CachedMesh writeCache( const fs::path &path, const SmartPtr<IMesh> &mesh, u64 byteLimit )
        {
            const auto source = dynamic_pointer_cast<workphone::Mesh>( mesh );
            if( !source || !source->getNumSubMeshes() || !source->isValid() )
            {
                throw std::runtime_error( "Invalid generated city mesh for the stream cache." );
            }
            const auto geometry = fingerprint( *source );

            {
                std::fstream file( path, std::ios::binary | std::ios::out | std::ios::trunc );
                if( !file )
                {
                    throw std::runtime_error( "Cannot create a city stream mesh." );
                }
                SmartPtr<IStream> stream = make_ptr<FileDataStream>( &file, false );
                MeshSerializer serializer;
                serializer.exportMesh( source.get(), stream );
                file.flush();
                if( !file )
                {
                    throw std::runtime_error( "Cannot finish writing a city stream mesh." );
                }
                stream->close();
            }

            const auto length = fs::file_size( path );
            if( !length || length > byteLimit )
            {
                throw std::runtime_error(
                    "A generated city patch exceeds the stream cache byte limit." );
            }

            std::ifstream file( path, std::ios::binary );
            std::array<u8, ioChunkSize> bytes;
            auto digest = hashStart;
            for( auto remaining = length; remaining; )
            {
                const auto count = static_cast<size_t>( std::min<uintmax_t>( remaining, bytes.size() ) );
                if( !file.read( reinterpret_cast<char *>( bytes.data() ), count ) )
                {
                    throw std::runtime_error( "Cannot verify a city stream mesh cache file." );
                }
                digest = appendDigest( digest, bytes.data(), count );
                remaining -= count;
            }
            return { path, length, digest, geometry.digest, geometry.bytes };
        }

        WorldStreamResult readPatch( const std::shared_ptr<const CachedPatch> &cache,
                                     const StreamCancellation &cancel )
        {
            WorldStreamResult result;
            if( cancel.cancelled() )
            {
                return result;
            }
            auto decoded = std::allocate_shared<DecodedPatch>( SourceBudgetAllocator<DecodedPatch>{} );
            decoded->meshes.reserve( cache->files.size() );
            MeshSerializer serializer;
            for( const auto &entry : cache->files )
            {
                if( cancel.cancelled() )
                {
                    return {};
                }
                std::ifstream file( entry.path, std::ios::binary | std::ios::ate );
                if( !file || file.tellg() != static_cast<std::streamoff>( entry.length ) )
                {
                    throw std::runtime_error( "A city stream mesh cache file changed size." );
                }
                file.seekg( 0 );
                // The scheduler reserves the largest encoded mesh as WorkingCpu for this job.
                // The actual decoded engine buffers are charged to SourceCpu by MeshSerializer.
                std::vector<u8> bytes( static_cast<size_t>( entry.length ) );
                auto digest = hashStart;
                for( size_t offset = 0; offset < bytes.size(); )
                {
                    if( cancel.cancelled() )
                    {
                        return {};
                    }
                    const auto size = std::min( ioChunkSize, bytes.size() - offset );
                    if( !file.read( reinterpret_cast<char *>( bytes.data() + offset ), size ) )
                    {
                        throw std::runtime_error( "Cannot read a city stream mesh cache file." );
                    }
                    digest = appendDigest( digest, bytes.data() + offset, size );
                    offset += size;
                }
                if( digest != entry.digest )
                {
                    throw std::runtime_error( "A city stream mesh cache file changed contents." );
                }
                SmartPtr<IStream> stream = make_ptr<MemoryFile>(
                    bytes.data(), static_cast<long>( bytes.size() ), String(), false );
                auto mesh = serializer.loadMesh( stream );
                if( !mesh || !mesh->getNumSubMeshes() || !mesh->isValid() )
                {
                    throw std::runtime_error( "Cannot decode a city stream mesh." );
                }
                const auto geometry = fingerprint( *mesh );
                if( geometry.digest != entry.geometryDigest || geometry.bytes != entry.sourceBytes )
                {
                    throw std::runtime_error( "The decoded city stream mesh changed its geometry." );
                }
                mesh->updateAABB( true );
                decoded->meshes.push_back( std::move( mesh ) );
            }
            if( cancel.cancelled() )
            {
                return {};
            }
            result.bytes = cache->length;
            result.payload = std::move( decoded );
            return result;
        }

        bool nativeReady( const SmartPtr<MeshRenderer> &renderer )
        {
            const auto object = renderer->getGraphicsObject();
            const auto node = renderer->getMeshNode();
            if( !renderer->isLoaded() || !object || !object->isLoaded() || !node || !node->isLoaded() )
            {
                return false;
            }
            const auto owner = object->getOwner();
            return !owner || owner == node;
        }

        bool belongsTo( SmartPtr<IGameActor> actor, const SmartPtr<IGameActor> &owner )
        {
            for( ; actor; actor = actor->getParent() )
            {
                if( actor == owner )
                {
                    return true;
                }
            }
            return false;
        }
    }  // namespace

    struct CityStreaming::Impl
    {
        enum class Phase
        {
            Resident,
            Retiring,
            Absent,
            Queued,
            Installing
        };

        struct DetailMesh
        {
            SmartPtr<MeshRenderer> renderer;
            SmartPtr<IMeshResource> resource;
            SmartPtr<render::IGraphicsObject> retiring;
            u32 visibilityFlags = render::IGraphicsObject::SceneFlag;
        };

        struct Patch
        {
            SmartPtr<IGameActor> actor;
            SmartPtr<LODGroup> lod;
            SourceBudgetVector<DetailMesh> details;
            SourceBudgetVector<SmartPtr<MeshRenderer>> fallback;
            std::shared_ptr<const CachedPatch> cache;
            s32 fallbackLevel = 0, originalForcedLOD = -1;
            f32 radius = 0;
            Phase phase = Phase::Absent;
            bool requested = false, countedResident = false;
            Clock::time_point retryAfter{};
            Clock::time_point installDeadline{};
        };

        fs::path directory;
        SourceBudgetVector<fs::path> outputFiles;
        SourceBudgetVector<Patch> patches;
        std::shared_ptr<WorldStreamProducer> producer;
        CityStreamingStats totals;

        ~Impl()
        {
            if( producer )
            {
                producer->drain();
            }
            std::error_code ignored;
            for( const auto &file : outputFiles )
            {
                fs::remove( file, ignored );
            }
            if( !directory.empty() )
            {
                fs::remove( directory, ignored );
            }
        }

        void selectFallback( Patch &patch )
        {
            patch.lod->setForcedLOD( patch.fallbackLevel );
            for( auto &renderer : patch.fallback )
            {
                renderer->setLODVisible( true );
            }
            for( auto &detail : patch.details )
            {
                detail.renderer->setLODVisible( false );
                detail.renderer->setEnabled( false );
            }
        }

        void retire( Patch &patch )
        {
            selectFallback( patch );
            patch.phase = Phase::Retiring;
        }

        void reclaim( Patch &patch )
        {
            const auto app = core::IApplicationManager::instancePtr();
            const auto graphics = app ? app->getGraphicsSystemPtr() : nullptr;
            const auto scene = graphics ? graphics->getGraphicsScenePtr() : nullptr;
            if( !scene )
            {
                return;
            }

            bool pending = false;
            for( auto &detail : patch.details )
            {
                if( auto object = detail.renderer->getGraphicsObject() )
                {
                    // Retire the graphics object without unloading the component: its actor,
                    // component FSM and event registrations are still part of the scene.
                    if( !detail.retiring )
                    {
                        detail.visibilityFlags = object->getVisibilityFlags();
                        detail.retiring = object;
                    }
                    auto node = detail.renderer->getMeshNode();
                    if( node )
                    {
                        node->detachObject( object );
                    }
                    if( !scene->removeGraphicsObject( object ) )
                    {
                        pending = true;
                        continue;
                    }
                    if( node )
                    {
                        if( auto parent = node->getParent() )
                        {
                            parent->removeChild( node );
                        }
                        scene->removeSceneNode( node );
                    }
                    detail.renderer->setGraphicsObject( nullptr );
                    detail.renderer->setMeshNode( nullptr );
                }
                if( detail.renderer->getGraphicsObject() ||
                    ( detail.retiring && detail.retiring->getLoadingState() != LoadingState::Unloaded ) )
                {
                    pending = true;
                    continue;
                }
                if( detail.retiring )
                {
                    detail.retiring = nullptr;
                    ++totals.nativeMeshUnloads;
                }
                if( detail.resource->getMesh() )
                {
                    detail.resource->setMesh( nullptr );
                    ++totals.meshPayloadEvictions;
                }
            }
            if( !pending )
            {
                patch.phase = Phase::Absent;
                if( patch.countedResident )
                {
                    ++totals.evictedPatches;
                }
                patch.countedResident = false;
            }
        }

        void install( Patch &patch, const DecodedPatch &decoded )
        {
            if( decoded.meshes.size() != patch.details.size() )
            {
                throw std::runtime_error( "A decoded city patch has an incomplete mesh inventory." );
            }
            patch.phase = Phase::Installing;
            patch.installDeadline = Clock::now() + std::chrono::seconds( 30 );
            for( size_t i = 0; i < patch.details.size(); ++i )
            {
                patch.details[i].resource->setMesh( decoded.meshes[i] );
            }
            for( auto &detail : patch.details )
            {
                detail.renderer->setEnabled( true );
                detail.renderer->setLODVisible( false );
                detail.renderer->updateMesh();
                auto object = detail.renderer->getGraphicsObject();
                const auto actor = detail.renderer->getActor();
                auto node = detail.renderer->getMeshNode();
                if( !detail.renderer->isLoaded() || !object || !actor || !node )
                {
                    throw std::runtime_error( "Cannot publish all renderers in a streamed city patch." );
                }
                object->setVisible( false );
                object->setCastShadows( detail.renderer->getCastShadows() !=
                                        Renderer::CastShadows::Off );
                object->setReceiveShadows( detail.renderer->getRecieveShadows() ==
                                           Renderer::RecieveShadows::On );
                object->setZOrder( detail.renderer->getZOrder() );
                object->setVisibilityFlags( detail.visibilityFlags );
                node->setStatic( actor->isStatic() );
                // The new graphics node has no sampled actor state yet. Seed its
                // pose directly, including when component-state updates are deferred.
                node->setTransform( actor->getWorldTransform() );
                detail.renderer->updateMaterials();
            }
        }

        void publish( Patch &patch )
        {
            patch.lod->setForcedLOD( patch.originalForcedLOD );
            patch.phase = Phase::Resident;
            patch.countedResident = true;
            ++totals.installedPatches;
            // A streamed install restores a mesh cached before initial scene publication.
            ++totals.reloadedPatches;
            totals.meshPayloadReloads += patch.details.size();
            totals.nativeMeshLoads += patch.details.size();
        }

        void fail( Patch &patch, const String &reason, String &error )
        {
            if( patch.phase == Phase::Installing )
            {
                retire( patch );
            }
            else
            {
                patch.phase = Phase::Absent;
            }
            patch.retryAfter = Clock::now() + std::chrono::seconds( 2 );
            ++totals.failedPatches;
            if( error.empty() )
            {
                error = reason;
            }
        }
    };

    CityStreaming::CityStreaming( const CityStreamingOptions &options ) : m_options( options )
    {
    }

    CityStreaming::~CityStreaming() = default;

    bool CityStreaming::initialize( SceneAssets &assets, SmartPtr<IGameActor> owner,
                                    const String &cacheDirectory, String &error )
    {
        std::lock_guard<std::mutex> lock( m_mutex );
        error.clear();
        if( m_impl )
        {
            error = "Close the existing city stream before replacing it.";
            return false;
        }
        try
        {
            if( !owner || !owner->getScene() || !assets.openCity || cacheDirectory.empty() ||
                !std::isfinite( m_options.loadRadius ) || !std::isfinite( m_options.retainRadius ) ||
                m_options.loadRadius < 0 || m_options.retainRadius < m_options.loadRadius ||
                !m_options.maximumResidentPatches || !m_options.maximumPendingPatches ||
                !m_options.maximumInstallsPerUpdate || !m_options.maximumPatchBytes ||
                m_options.maximumPatchBytes > static_cast<u64>( std::numeric_limits<long>::max() ) )
            {
                throw std::runtime_error( "Invalid generated city streaming setup." );
            }

            auto candidate = std::make_unique<Impl>();
            SourceBudgetMap<const IMeshResource *, u32> meshUsers;
            for( const auto &actor : assets.actors )
            {
                const auto mesh = actor ? actor->getComponent<scene::Mesh>() : nullptr;
                const auto resource = mesh ? mesh->getMeshResource() : nullptr;
                if( resource )
                {
                    ++meshUsers[resource.get()];
                }
            }
            for( const auto &actor : assets.actors )
            {
                if( !actor || actor->getName() != "City LOD patch" || !belongsTo( actor, owner ) )
                {
                    continue;
                }
                const auto lod = actor->getComponent<LODGroup>();
                if( !lod || !lod->isLODEnabled() || lod->getForcedLOD() >= 0 )
                {
                    continue;
                }
                const auto levels = lod->getLevels();
                if( levels.size() < 2 || levels.back().renderers.empty() )
                {
                    continue;
                }
                const bool imposterBacked =
                    std::any_of( levels.back().renderers.begin(), levels.back().renderers.end(),
                                 []( const auto &item ) {
                                     const auto renderer = dynamic_pointer_cast<MeshRenderer>( item );
                                     const auto child = renderer ? renderer->getActor() : nullptr;
                                     return child && child->getComponent<MeshImposter>();
                                 } );
                if( !imposterBacked )
                {
                    continue;
                }
                Impl::Patch patch;
                patch.actor = actor;
                patch.lod = lod;
                patch.originalForcedLOD = lod->getForcedLOD();
                patch.fallbackLevel = static_cast<s32>( levels.size() - 1 );
                patch.radius = lod->getSize() * .5f;
                for( const auto &bound : lod->getDetailBounds() )
                {
                    patch.radius = std::max(
                        patch.radius, static_cast<f32>( bound.centre.length() ) + bound.diameter * .5f );
                }

                bool eligible = true;
                for( size_t level = 0; level < levels.size() && eligible; ++level )
                {
                    for( const auto &item : levels[level].renderers )
                    {
                        const auto renderer = dynamic_pointer_cast<MeshRenderer>( item );
                        const auto child = renderer ? renderer->getActor() : nullptr;
                        const auto mesh = child ? child->getComponent<scene::Mesh>() : nullptr;
                        const auto resource = mesh ? mesh->getMeshResource() : nullptr;
                        if( !child || child->getParent() != actor || !resource || !resource->getMesh() ||
                            child->getComponent<Collision>() || child->getComponent<Rigidbody>() )
                        {
                            eligible = false;
                            break;
                        }
                        const bool duplicate = std::any_of(
                            patch.details.begin(), patch.details.end(),
                            [&]( const auto &detail ) { return detail.resource == resource; } );
                        if( duplicate )
                        {
                            eligible = false;
                            break;
                        }
                        if( level + 1 == levels.size() )
                        {
                            patch.fallback.push_back( renderer );
                            continue;
                        }
                        const auto users = meshUsers.find( resource.get() );
                        if( users == meshUsers.end() || users->second != 1 )
                        {
                            eligible = false;
                            break;
                        }
                        patch.details.push_back( { renderer, resource, nullptr } );
                    }
                }
                if( eligible && !patch.details.empty() )
                {
                    candidate->patches.push_back( std::move( patch ) );
                }
                else if( imposterBacked )
                {
                    throw std::runtime_error(
                        "Cannot stream every detail level of a city imposter patch." );
                }
            }
            if( candidate->patches.empty() )
            {
                candidate->totals.initialized = true;
                m_impl = std::move( candidate );
                return true;
            }

            const auto cacheRoot = fs::u8path( cacheDirectory.c_str() );
            fs::create_directories( cacheRoot );
            const auto directory = cacheRoot / fs::u8path( StringUtil::getUUID().c_str() );
            if( !fs::create_directory( directory ) )
            {
                throw std::runtime_error(
                    "Cannot reserve a private generated city stream cache directory." );
            }
            candidate->directory = directory;
            for( auto &patch : candidate->patches )
            {
                auto cache = std::allocate_shared<CachedPatch>( SourceBudgetAllocator<CachedPatch>{} );
                cache->files.reserve( patch.details.size() );
                for( const auto &detail : patch.details )
                {
                    const auto path = candidate->directory /
                                      ( std::to_string( candidate->outputFiles.size() ) + ".mesh" );
                    candidate->outputFiles.push_back( path );
                    auto file = writeCache( path, detail.resource->getMesh(),
                                            m_options.maximumPatchBytes - cache->length );
                    cache->length += file.length;
                    cache->largestFile = std::max( cache->largestFile, file.length );
                    cache->files.push_back( std::move( file ) );
                }
                candidate->totals.sourceDiskBytes += cache->length;
                candidate->totals.fallbackMeshes += static_cast<u32>( patch.fallback.size() );
                patch.cache = std::move( cache );
            }
            WorldStreamProducerLimits limits;
            limits.maximumPending = m_options.maximumPendingPatches;
            limits.maximumActive = 1;
            limits.maximumReady = 1;
            limits.maximumReadyBytes = m_options.maximumPatchBytes;
            candidate->producer = WorldStreamingScheduler::shared()->createProducer( limits, error );
            if( !candidate->producer )
            {
                return false;
            }
            for( auto &patch : candidate->patches )
            {
                // No detail tier may have a native graphics object at cold start.
                // Reject a late publication instead of reporting false residency.
                for( const auto &detail : patch.details )
                {
                    if( detail.renderer->getGraphicsObject() )
                    {
                        throw std::runtime_error( "City detail loaded before imposter-only startup." );
                    }
                }
                candidate->selectFallback( patch );
                for( auto &detail : patch.details )
                {
                    detail.resource->setMesh( nullptr );
                    ++candidate->totals.meshPayloadEvictions;
                }
            }
            candidate->totals.initialized = true;
            candidate->totals.totalPatches = static_cast<u32>( candidate->patches.size() );
            m_impl = std::move( candidate );
            return true;
        }
        catch( const std::exception &failure )
        {
            error = failure.what();
            return false;
        }
    }

    bool CityStreaming::updateFromActiveCamera( String &error )
    try
    {
        SmartPtr<LODSystem> lodSystem;
        {
            std::lock_guard<std::mutex> lock( m_mutex );
            if( !m_impl )
            {
                error.clear();
                return false;
            }
            for( const auto &patch : m_impl->patches )
            {
                lodSystem = dynamic_pointer_cast<LODSystem>( patch.lod->getComponentSystem() );
                if( lodSystem )
                {
                    break;
                }
            }
        }

        // Do not hold the residency lock while acquiring the LOD system's view lock.
        // Raw render cameras publish a view override; managed cameras are resolved live.
        Array<Vector3F> observers{ Allocator<Vector3F>( sourceBudget ) };
        LODSystem::View view;
        if( lodSystem && lodSystem->getView( view ) )
        {
            const Vector3F position{ f32( view.position.x ), f32( view.position.y ),
                                     f32( view.position.z ) };
            if( position.isFinite() )
            {
                observers.push_back( position );
            }
        }
        return update( observers, error );
    }
    catch( const std::exception &failure )
    {
        error = failure.what();
        return false;
    }

    bool CityStreaming::update( const Array<Vector3F> &observers, String &error )
    try
    {
        std::lock_guard<std::mutex> lock( m_mutex );
        error.clear();
        if( !m_impl )
        {
            return false;
        }
        auto &state = *m_impl;
        if( state.patches.empty() )
        {
            return true;
        }
        // Losing the camera during a scene or editor transition must preserve existing demand.
        if( std::any_of( observers.begin(), observers.end(),
                         []( const auto &p ) { return p.isFinite(); } ) )
        {
            struct Demand
            {
                size_t index;
                double distance;
            };

            SourceBudgetVector<Demand> nearest;
            nearest.reserve( state.patches.size() );
            for( size_t i = 0; i < state.patches.size(); ++i )
            {
                auto &patch = state.patches[i];
                const auto pose = patch.actor->getWorldTransform();
                const auto p = pose.getPosition(), scale = pose.getScale();
                const double extent = patch.radius * std::max( { std::abs( double( scale.x ) ),
                                                                 std::abs( double( scale.y ) ),
                                                                 std::abs( double( scale.z ) ) } );
                double distance = std::numeric_limits<double>::infinity();
                for( const auto &observer : observers )
                {
                    if( observer.isFinite() )
                    {
                        distance =
                            std::min( distance, std::max( 0., std::hypot( double( observer.x ) - p.x,
                                                                          double( observer.z ) - p.z ) -
                                                                  extent ) );
                    }
                }
                const auto radius = patch.requested || patch.phase == Impl::Phase::Resident
                                        ? m_options.retainRadius
                                        : m_options.loadRadius;
                patch.requested = false;
                if( distance <= radius )
                {
                    nearest.push_back( { i, distance } );
                }
            }
            std::sort( nearest.begin(), nearest.end(), []( const Demand &a, const Demand &b ) {
                return a.distance != b.distance ? a.distance < b.distance : a.index < b.index;
            } );
            if( nearest.size() > m_options.maximumResidentPatches )
            {
                nearest.resize( m_options.maximumResidentPatches );
            }
            for( const auto &entry : nearest )
            {
                state.patches[entry.index].requested = true;
            }

            for( size_t i = 0; i < state.patches.size(); ++i )
            {
                auto &patch = state.patches[i];
                if( !patch.requested && patch.phase == Impl::Phase::Queued )
                {
                    state.producer->cancel( i + 1 );
                    patch.phase = Impl::Phase::Absent;
                }
                if( !patch.requested &&
                    ( patch.phase == Impl::Phase::Resident || patch.phase == Impl::Phase::Installing ) &&
                    std::all_of( patch.fallback.begin(), patch.fallback.end(), nativeReady ) )
                {
                    state.retire( patch );
                }
            }

            Array<WorldStreamRequest> requests{ Allocator<WorldStreamRequest>( sourceBudget ) };
            const auto queue = state.producer->stats();
            const auto pending = queue.pending + queue.active + queue.ready;
            const bool awaitingRetirement =
                std::any_of( state.patches.begin(), state.patches.end(),
                             []( const auto &patch ) { return patch.phase == Impl::Phase::Retiring; } );
            for( const auto &entry : nearest )
            {
                if( awaitingRetirement || pending + requests.size() >= m_options.maximumPendingPatches )
                {
                    break;
                }
                const auto &patch = state.patches[entry.index];
                if( patch.phase != Impl::Phase::Absent || patch.retryAfter > Clock::now() ||
                    state.producer->contains( entry.index + 1 ) )
                {
                    continue;
                }
                WorldStreamRequest request;
                request.key = entry.index + 1;
                request.priority = static_cast<s32>( nearest.size() - requests.size() );
                request.workingBytes = patch.cache->largestFile;
                request.maximumResultBytes = patch.cache->length;
                request.execute = [cache = patch.cache]( const StreamCancellation &cancel ) {
                    return readPatch( cache, cancel );
                };
                requests.push_back( std::move( request ) );
            }
            if( !requests.empty() && state.producer->enqueue( requests, error ) )
            {
                for( const auto &request : requests )
                {
                    state.patches[request.key - 1].phase = Impl::Phase::Queued;
                }
            }
        }

        for( auto &patch : state.patches )
        {
            if( patch.phase == Impl::Phase::Retiring )
            {
                state.reclaim( patch );
            }
            if( patch.phase != Impl::Phase::Installing )
            {
                continue;
            }
            const bool failed =
                std::any_of( patch.details.begin(), patch.details.end(), []( const auto &detail ) {
                    const auto object = detail.renderer->getGraphicsObject();
                    return !object || object->getLoadingState() == LoadingState::Error;
                } );
            if( std::all_of( patch.details.begin(), patch.details.end(),
                             []( const auto &detail ) { return nativeReady( detail.renderer ); } ) )
            {
                state.publish( patch );
            }
            else if( failed || Clock::now() >= patch.installDeadline )
            {
                state.fail( patch,
                            "A streamed city mesh could not load natively within its install window.",
                            error );
            }
        }

        WorldStreamResult result;
        for( u32 i = 0; i < m_options.maximumInstallsPerUpdate && state.producer->take( result ); ++i )
        {
            if( !result.key || result.key > state.patches.size() )
            {
                continue;
            }
            auto &patch = state.patches[result.key - 1];
            if( patch.phase != Impl::Phase::Queued )
            {
                continue;
            }
            if( !result.payload || !result.error.empty() )
            {
                state.fail( patch,
                            result.error.empty() ? "The city stream did not return its mesh payload."
                                                 : result.error,
                            error );
                continue;
            }
            try
            {
                state.install( patch, *std::static_pointer_cast<const DecodedPatch>( result.payload ) );
            }
            catch( const std::exception &failure )
            {
                state.fail( patch, failure.what(), error );
            }
        }
        for( size_t i = 0; i < state.patches.size(); ++i )
        {
            auto &patch = state.patches[i];
            if( patch.phase == Impl::Phase::Queued && !state.producer->contains( i + 1 ) )
            {
                state.fail( patch, "A city stream request expired before its mesh data was available.",
                            error );
            }
        }
        return error.empty();
    }
    catch( const std::exception &failure )
    {
        error = failure.what();
        return false;
    }

    void CityStreaming::close()
    {
        std::unique_ptr<Impl> retired;
        {
            std::lock_guard<std::mutex> lock( m_mutex );
            retired = std::move( m_impl );
        }
        // Worker drain and native reference releases can run after publication is cleared.
    }

    bool CityStreaming::isInitialized() const
    {
        std::lock_guard<std::mutex> lock( m_mutex );
        return m_impl != nullptr;
    }

    CityStreamingStats CityStreaming::stats() const
    {
        std::lock_guard<std::mutex> lock( m_mutex );
        if( !m_impl )
        {
            return {};
        }
        auto result = m_impl->totals;
        for( const auto &patch : m_impl->patches )
        {
            if( patch.phase == Impl::Phase::Queued || patch.phase == Impl::Phase::Installing )
            {
                ++result.pendingPatches;
            }
            if( patch.phase == Impl::Phase::Retiring )
            {
                ++result.retiringPatches;
            }
            if( patch.phase == Impl::Phase::Resident )
            {
                ++result.residentPatches;
            }
            for( size_t i = 0; i < patch.details.size(); ++i )
            {
                if( patch.details[i].resource->getMesh() )
                {
                    ++result.residentMeshes;
                    result.residentSourceBytes += patch.cache->files[i].sourceBytes;
                }
            }
        }
        return result;
    }

    bool CityStreaming::isStreamedOut( const MeshRenderer *renderer ) const
    {
        std::lock_guard<std::mutex> lock( m_mutex );
        if( !m_impl || !renderer )
        {
            return false;
        }
        for( const auto &patch : m_impl->patches )
        {
            if( patch.phase != Impl::Phase::Resident &&
                std::any_of( patch.details.begin(), patch.details.end(),
                             [&]( const auto &detail ) { return detail.renderer.get() == renderer; } ) )
            {
                return true;
            }
        }
        return false;
    }

    bool CityStreaming::hasPending() const
    {
        std::lock_guard<std::mutex> lock( m_mutex );
        if( !m_impl )
        {
            return false;
        }
        return std::any_of( m_impl->patches.begin(), m_impl->patches.end(), []( const auto &patch ) {
            return patch.phase == Impl::Phase::Queued || patch.phase == Impl::Phase::Retiring ||
                   patch.phase == Impl::Phase::Installing ||
                   ( patch.phase == Impl::Phase::Absent && patch.requested );
        } );
    }
}  // namespace workphone::scene::race
