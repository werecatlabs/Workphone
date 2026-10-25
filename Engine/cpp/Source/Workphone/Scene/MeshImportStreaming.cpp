#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/MeshImportStreaming.hpp>
#include <Workphone/Scene/SceneWorld.hpp>
#include <Workphone/Scene/SceneWorldStreamer.hpp>
#include <Workphone/Scene/GameActorUtil.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/PropertiesBinarySerializer.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <algorithm>
#include <cmath>
#include <cwctype>
#include <filesystem>
#include <stdexcept>

namespace workphone::scene
{
    namespace
    {
        namespace fs = std::filesystem;

        SmartPtr<Properties> copyData( const Properties &source )
        {
            auto copy = make_ptr<Properties>();
            String error;
            const auto bytes = PropertiesBinarySerializer::serialize( source );
            if( !PropertiesBinarySerializer::deserialize( bytes, *copy, &error ) )
                throw std::runtime_error( error.c_str() );
            return copy;
        }

        bool samePathPart( const fs::path &a, const fs::path &b )
        {
#ifdef _WIN32
            auto left = a.native(), right = b.native();
            const auto lower = []( wchar_t value ) { return wchar_t( std::towlower( value ) ); };
            std::transform( left.begin(), left.end(), left.begin(), lower );
            std::transform( right.begin(), right.end(), right.begin(), lower );
            return left == right;
#else
            return a == b;
#endif
        }

        // Canonical paths also resolve existing junction/symlink parents of a
        // not-yet-created generation. Check components rather than text prefixes.
        bool strictlyBelow( const fs::path &root, const fs::path &path )
        {
            auto candidate = path.begin();
            for( const auto &part : root )
            {
                if( candidate == path.end() || !samePathPart( part, *candidate ) ) return false;
                ++candidate;
            }
            return candidate != path.end();
        }

        void normalizeRootTransform( Properties &source )
        {
            for( const auto &name : { GameActorUtil::localTransformStr, GameActorUtil::worldTransformStr } )
            {
                auto transform = source.getChild( name );
                if( !transform )
                {
                    transform = make_ptr<Properties>(); transform->setName( name ); source.addChild( transform );
                }
                transform->setProperty( "Position", Vector3F::zero() );
                transform->setProperty( "Orientation", QuaternionF::identity() );
                transform->setProperty( "Scale", Vector3F::unit() );
            }
        }

        Transform3F localTransform( const Properties &actor )
        {
            Transform3F result = Transform3F::identity();
            if( auto local = actor.getChild( GameActorUtil::localTransformStr ) ) result.setProperties( local );
            return result;
        }

        void rebaseWorldTransforms( Properties &actor, const Transform3F &parent );

        void rebaseChildData( Properties &data, const Transform3F &parent )
        {
            // The same direct-actor/container distinction used by GameActorUtil.
            if( data.getNumProperties() || data.hasChild( GameActorUtil::localTransformStr ) ||
                data.hasChild( GameActorUtil::worldTransformStr ) || data.hasChild( GameActorUtil::componentStr ) ||
                data.hasChild( GameActorUtil::componentsStr ) )
            {
                rebaseWorldTransforms( data, parent ); return;
            }
            for( const auto &child : data.getChildren() )
                if( child->getName() == GameActorUtil::childStr || child->getName() == GameActorUtil::childrenStr )
                    rebaseChildData( *child, parent );
        }

        void rebaseWorldTransforms( Properties &actor, const Transform3F &parent )
        {
            const auto local = localTransform( actor );
            const Transform3F world( parent.getPosition() + parent.getOrientation() *
                                        ( parent.getScale() * local.getPosition() ),
                                    parent.getOrientation() * local.getOrientation(),
                                    parent.getScale() * local.getScale() );
            auto data = actor.getChild( GameActorUtil::worldTransformStr );
            if( !data )
            {
                data = make_ptr<Properties>(); data->setName( GameActorUtil::worldTransformStr ); actor.addChild( data );
            }
            data->setProperty( "Position", world.getPosition() );
            data->setProperty( "Orientation", world.getOrientation() );
            data->setProperty( "Scale", world.getScale() );
            for( const auto &child : actor.getChildren() )
                if( child->getName() == GameActorUtil::childStr || child->getName() == GameActorUtil::childrenStr )
                    rebaseChildData( *child, world );
        }
    }

    bool MeshImportStreaming::buildPrefab( const Properties &sourceActorData,
        const MeshResourceDirector::StreamingOptions &options, const String &projectRoot,
        const String &outputGenerationDirectory, SmartPtr<Properties> &prefabData,
        String &manifestPath, String &error )
    {
        error.clear();
        try
        {
            auto source = copyData( sourceActorData );
            if( !options.enabled )
            {
                prefabData = source; manifestPath.clear(); return true;
            }
            if( !std::isfinite( options.cellSize ) || options.cellSize <= 0 || options.cellSize > 1000000 ||
                !options.maxActorsPerCell || options.maxActorsPerCell >= WP_MAX_ACTORS ||
                !std::isfinite( options.loadRadius ) || options.loadRadius < 0 || options.loadRadius > 1000000 ||
                !std::isfinite( options.retainRadius ) || options.retainRadius < options.loadRadius ||
                options.retainRadius > 1000000 )
                throw std::runtime_error( "Invalid mesh import streaming limits" );
            if( !source->hasProperty( GameActorUtil::uuidStr ) ||
                source->getPropertyObject( GameActorUtil::uuidStr ).getValue().empty() )
                throw std::runtime_error( "Mesh import streaming requires serialized actor-root data with a UUID" );

            auto root = fs::u8path( projectRoot.c_str() );
            auto output = fs::u8path( outputGenerationDirectory.c_str() );
            if( root.empty() || output.empty() || !root.is_absolute() || !output.is_absolute() ||
                !fs::is_directory( root ) )
                throw std::runtime_error( "Streaming output requires an absolute project root and generation directory" );
            root = fs::canonical( root ); output = fs::weakly_canonical( output );
            const auto cache = fs::weakly_canonical( root / "Cache" );
            if( !strictlyBelow( root, cache ) || !strictlyBelow( cache, output ) )
                throw std::runtime_error( "Streaming generation must be inside the project's Cache directory" );
            if( fs::exists( output ) )
                throw std::runtime_error( "Streaming generation directory already exists; use a fresh generation" );

            auto wrapper = make_ptr<Properties>();
            wrapper->setName( source->getName() );
            wrapper->setPropertiesAsArray( source->getPropertiesAsArray() );
            wrapper->setProperty( "meshImportStreaming", true );
            for( const auto &name : { GameActorUtil::localTransformStr, GameActorUtil::worldTransformStr } )
                if( auto transform = source->getChild( name ) ) wrapper->addChild( copyData( *transform ) );
            // Instance World installs cell roots below this wrapper. Keeping the
            // authored root transform in both layers would apply it twice.
            normalizeRootTransform( *source );
            // Persistent subtrees retain their descendants. Import also reads
            // serialized world poses, so remove the old root placement there.
            rebaseWorldTransforms( *source, Transform3F::identity() );

            WorldCookOptions cook;
            cook.cellSize = options.cellSize; cook.maximumActorsPerCell = options.maxActorsPerCell;
            cook.assetRoot = root.generic_u8string().c_str();
            cook.convertRenderOnlyLeaves = true; cook.generateHlod = options.enableHlod;
            SceneWorldStreamOptions runtime;
            // Cooked decoded-size admission includes per-actor staging overhead.
            // Avoid publishing a cell that the generated runtime defaults reject.
            if( u64( options.maxActorsPerCell ) * 4096 >= runtime.maximumDecodedCellBytes )
                throw std::runtime_error( "Mesh import actor limit exceeds decoded cell staging capacity" );
            cook.maximumCellBytes = ( std::min )( cook.maximumCellBytes,
                ( runtime.maximumDecodedCellBytes - u64( options.maxActorsPerCell ) * 4096 ) / 8 );
            String cookedManifest, cookError;
            if( !SceneWorld::cook( *source, output.generic_u8string().c_str(), cook, cookedManifest, cookError ) )
                throw std::runtime_error( cookError.c_str() );
            const auto relativeManifest = fs::u8path( cookedManifest.c_str() ).lexically_relative( root );
            if( relativeManifest.empty() || relativeManifest.is_absolute() || *relativeManifest.begin() == ".." )
                throw std::runtime_error( "Cooked world manifest is outside the project" );

            auto streaming = make_ptr<Properties>(); streaming->setName( GameActorUtil::componentStr );
            streaming->setProperty( GameActorUtil::componentTypeStr, "workphone::scene::WorldStreaming" );
            streaming->setProperty( GameActorUtil::uuidStr, StringUtil::getUUID() );
            streaming->setProperty( "enabled", true );
            streaming->setProperty( "World Manifest", relativeManifest.generic_u8string().c_str() );
            streaming->setProperty( "Load Radius", options.loadRadius );
            streaming->setProperty( "Retain Radius", options.retainRadius );
            streaming->setProperty( "Enable HLOD", options.enableHlod );
            const auto proxyStep = ( std::max )( options.cellSize, 256.0f );
            const auto proxyLoad = ( std::max )( runtime.proxyLoadRadius, options.retainRadius + 2 * proxyStep );
            streaming->setProperty( "Proxy Load Radius", proxyLoad );
            streaming->setProperty( "Proxy Retain Radius", proxyLoad + proxyStep );
            streaming->setProperty( "Maximum Resident Actors",
                ( std::max )( runtime.maximumResidentActors, options.maxActorsPerCell ) );
            streaming->setProperty( "Maximum Resident Cells", runtime.maximumResidentCells );
            streaming->setProperty( "Install Milliseconds", runtime.installMilliseconds );
            streaming->setProperty( "Preview Editor Camera", options.previewEditorCamera );
            streaming->setProperty( "Follow Active Camera", true );
            streaming->setProperty( "Instance World", true );
            wrapper->addChild( streaming );

            static_assert( noexcept( manifestPath.swap( cookedManifest ) ),
                           "Prefab manifest publication must not allocate or throw" );
            static_assert( noexcept( prefabData = wrapper ),
                           "Prefab pointer publication must not throw" );
            // Keep the previous owner alive through the commit so pointer
            // assignment only changes reference counts. All new storage is
            // already admitted; exchanging the completed path cannot allocate.
            [[maybe_unused]] const auto previousPrefab = prefabData;
            manifestPath.swap( cookedManifest );
            prefabData = wrapper;
            return true;
        }
        catch( const std::exception &exception )
        {
            error = String( "Mesh import streaming: " ) + exception.what(); return false;
        }
    }
}
