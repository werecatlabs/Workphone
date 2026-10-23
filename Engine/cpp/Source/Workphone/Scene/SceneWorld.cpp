#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/SceneWorld.hpp>
#include <Workphone/Scene/GameActorUtil.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/PropertiesBinarySerializer.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/ApplicationUtil.hpp>
#include <WorkphoneGraphics/workphone_graphics_mesh.h>
#include <WorkphoneGraphics/workphone_graphics_mesh_reader.h>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <unordered_map>
#include <cmath>
#include <limits>
#ifdef _WIN32
#include <Windows.h>
#endif

namespace workphone::scene
{
    namespace
    {
        namespace fs = std::filesystem;
        bool finite( const Vector3F &v )
        {
            return std::isfinite( v.x ) && std::isfinite( v.y ) && std::isfinite( v.z );
        }
        bool cellFile( const String &name )
        {
            const std::string path( name.c_str() );
            return !path.empty() && path.size() < 160 && path.find( ".." ) == std::string::npos &&
                   path.find_first_of( "/\\:" ) == std::string::npos &&
                   fs::path( path ).extension() == ".wpcell";
        }
        Array<u8> read( const fs::path &path, u64 limit )
        {
            std::ifstream input( path, std::ios::binary | std::ios::ate );
            const auto length = input ? input.tellg() : std::streampos( -1 );
            if( length < 0 || static_cast<u64>( length ) > limit )
                throw std::runtime_error( "World file missing or exceeds its admission limit" );
            Array<u8> bytes( static_cast<size_t>( length ) );
            input.seekg( 0 );
            if( !input.read( reinterpret_cast<char *>( bytes.data() ), bytes.size() ) )
                throw std::runtime_error( "Incomplete world file" );
            return bytes;
        }
        void write( const fs::path &path, const Array<u8> &bytes )
        {
            std::ofstream output( path, std::ios::binary | std::ios::trunc );
            output.write( reinterpret_cast<const char *>( bytes.data() ), bytes.size() );
            output.close();
            if( !output ) throw std::runtime_error( "Could not write cooked world file" );
        }
        SmartPtr<Properties> clone( const Properties &source )
        {
            auto copy = make_ptr<Properties>();
            auto bytes = PropertiesBinarySerializer::serialize( source );
            String error;
            if( !PropertiesBinarySerializer::deserialize( bytes, *copy, &error ) )
                throw std::runtime_error( error.c_str() );
            return copy;
        }
        void visit( const Properties &node, const std::function<void( const Properties & )> &fn )
        {
            fn( node );
            for( const auto &child : node.getChildren() ) visit( *child, fn );
        }
        u32 countActors( const Properties &root )
        {
            u32 count = 0;
            visit( root, [&]( const Properties &p ) {
                if( p.getName() == GameActorUtil::childStr || p.getName() == ApplicationUtil::actorsStr )
                    ++count;
            } );
            return count;
        }
        bool renderOnly( const Properties &node )
        {
            bool safe = true, mesh = false;
            visit( node, [&]( const Properties &p ) {
                bool smooth = false;
                p.getPropertyValue( GameActorUtil::smoothMotionStr, smooth );
                if( smooth ) safe = false;
                String type;
                if( p.getPropertyValue( GameActorUtil::componentTypeStr, type ) && !type.empty() )
                {
                    const bool isMesh = type == "Mesh" || type == "workphone::scene::Mesh";
                    mesh |= isMesh;
                    safe &= isMesh || type == "MeshRenderer" || type == "Material" ||
                            type == "workphone::scene::MeshRenderer" || type == "workphone::scene::Material";
                }
            } );
            return safe && mesh;
        }
        Transform3F localTransform( const Properties &actor )
        {
            Transform3F result;
            if( auto data = actor.getChild( GameActorUtil::localTransformStr ) )
            {
                Vector3F position = Vector3F::zero(), scale = Vector3F::unit();
                QuaternionF rotation = QuaternionF::identity();
                data->getPropertyValue( "Position", position );
                data->getPropertyValue( "Orientation", rotation );
                data->getPropertyValue( "Scale", scale );
                result = Transform3F( position, rotation, scale );
            }
            return result;
        }
        Transform3F compose( const Transform3F &parent, const Transform3F &local )
        {
            return Transform3F( parent.getPosition() + parent.getOrientation() *
                                  ( parent.getScale() * local.getPosition() ),
                                parent.getOrientation() * local.getOrientation(),
                                parent.getScale() * local.getScale() );
        }
        void setRootTransform( Properties &actor, const Transform3F &world )
        {
            for( auto name : { GameActorUtil::localTransformStr, GameActorUtil::worldTransformStr } )
            {
                auto data = actor.getChild( name );
                if( !data ) { data = make_ptr<Properties>(); data->setName( name ); actor.addChild( data ); }
                data->setProperty( "Position", world.getPosition() );
                data->setProperty( "Orientation", world.getOrientation() );
                data->setProperty( "Scale", world.getScale() );
            }
        }
    }

    u64 SceneWorld::keyFor( const String &text )
    {
        u64 hash = 14695981039346656037ull;
        for( auto c : text ) { hash ^= static_cast<u8>( c ); hash *= 1099511628211ull; }
        return hash ? hash : 1;
    }

    double WorldCell::distanceSquared( const Vector3F &p ) const
    {
        const double x = std::max( { double( minimum.x ) - p.x, 0.0, double( p.x ) - maximum.x } );
        const double z = std::max( { double( minimum.z ) - p.z, 0.0, double( p.z ) - maximum.z } );
        return x * x + z * z;
    }

    bool SceneWorld::cook( const Properties &scene, const String &directory,
                           const WorldCookOptions &options, String &manifestPath, String &error )
    {
        manifestPath.clear(); error.clear();
        try
        {
            if( !std::isfinite( options.cellSize ) || options.cellSize <= 0 ||
                !options.maximumActorsPerCell || options.maximumActorsPerCell > WP_MAX_ACTORS ||
                !options.maximumCellBytes || options.maximumCellBytes > ( 64ull << 20 ) )
                throw std::runtime_error( "Invalid world cook limits" );
            fs::path output( directory.c_str() );
            fs::create_directories( output );
            auto manifest = make_ptr<Properties>();
            manifest->setProperty( "version", WorldManifest::version );
            manifest->setProperty( "cellSize", options.cellSize );
            manifest->setProperty( "generation", StringUtil::getUUID() );
            // References across actor groups pin those groups in the persistent
            // layer. Do not replace a working UUID graph with unresolved pointers.
            std::unordered_map<std::string, u32> references;
            visit( scene, [&]( const Properties &p ) {
                String type;
                if( p.getPropertyValue( GameActorUtil::componentTypeStr, type ) &&
                    ( type == "WorldStreaming" || type == "workphone::scene::WorldStreaming" ) )
                    throw std::runtime_error( "Cook the source scene, not an existing world streaming bootstrap" );
                for( const auto &property : p.getPropertiesAsArray() )
                    if( property.getName() != GameActorUtil::uuidStr )
                        ++references[property.getValue().c_str()];
            } );
            std::map<std::pair<int,int>, Array<SmartPtr<Properties>>> groups;
            std::map<const Properties *, std::pair<Vector3F, Vector3F>> bounds;
            Array<SmartPtr<Properties>> persistent;
            std::function<void( const Properties &, const Transform3F & )> partition;
            partition = [&]( const Properties &actor, const Transform3F &parent ) {
                const auto world = compose( parent, localTransform( actor ) );
                if( !finite( world.getPosition() ) || !finite( world.getScale() ) )
                    throw std::runtime_error( "Nonfinite world transform" );
                bool stationary = false, referenced = false;
                actor.getPropertyValue( GameActorUtil::staticStr, stationary );
                visit( actor, [&]( const Properties &p ) {
                    String uuid;
                    if( p.getPropertyValue( GameActorUtil::uuidStr, uuid ) && !uuid.empty() &&
                        references.count( uuid.c_str() ) ) referenced = true;
                } );
                const auto children = actor.getChildrenByName( GameActorUtil::childStr );
                bool enabled = true, visible = true, smooth = false;
                actor.getPropertyValue( GameActorUtil::enabledStr, enabled );
                actor.getPropertyValue( "visible", visible );
                actor.getPropertyValue( GameActorUtil::smoothMotionStr, smooth );
                const bool componentless = actor.getChildrenByName( GameActorUtil::componentStr ).empty() &&
                                           actor.getChildrenByName( GameActorUtil::componentsStr ).empty();
                // Explicit conversion may flatten render-only grouping nodes;
                // unknown components, references and gameplay groups stay intact.
                if( options.convertRenderOnlyLeaves && componentless && !children.empty() && !referenced &&
                    enabled && visible && !smooth )
                {
                    for( const auto &child : children ) partition( *child, world );
                    return;
                }
                auto data = clone( actor );
                data->setName( ApplicationUtil::actorsStr );
                setRootTransform( *data, world );
                Vector3F low = world.getPosition(), high = low;
                bool knownBounds = true, hasGeometryBounds = false;
                std::function<void( const Properties &, const Transform3F & )> measure;
                measure = [&]( const Properties &node, const Transform3F &pose ) {
                    for( const auto &component : node.getChildrenByName( GameActorUtil::componentStr ) )
                    {
                        String meshPath;
                        if( !component->getPropertyValue( "Mesh Path", meshPath ) || meshPath.empty() ) continue;
                        try
                        {
                            auto file = fs::path( meshPath.c_str() );
                            if( !file.is_absolute() ) file = fs::path( options.assetRoot.c_str() ) / file;
                            auto bytes = read( file, 64ull << 20 );
                            auto mesh = wp_graphics_mesh_read_from_buffer( bytes.data(), static_cast<wp_u32>( bytes.size() ) );
                            if( !mesh ) { knownBounds = false; continue; }
                            const auto box = wp_graphics_mesh_get_local_aabb( mesh );
                            wp_graphics_mesh_destroy( mesh );
                            for( unsigned corner = 0; corner < 8; ++corner )
                            {
                                const Vector3F local( corner & 1 ? box.max.x : box.min.x,
                                                       corner & 2 ? box.max.y : box.min.y,
                                                       corner & 4 ? box.max.z : box.min.z );
                                const auto p = pose.getPosition() + pose.getOrientation() * ( pose.getScale() * local );
                                if( !hasGeometryBounds ) { low = high = p; hasGeometryBounds = true; }
                                low.x = std::min( low.x, p.x ); low.y = std::min( low.y, p.y ); low.z = std::min( low.z, p.z );
                                high.x = std::max( high.x, p.x ); high.y = std::max( high.y, p.y ); high.z = std::max( high.z, p.z );
                            }
                        }
                        catch( ... ) { knownBounds = false; }
                    }
                    for( const auto &child : node.getChildrenByName( GameActorUtil::childStr ) )
                        measure( *child, compose( pose, localTransform( *child ) ) );
                };
                measure( actor, world );
                bounds[data.get()] = { low, high };
                if( !referenced && renderOnly( actor ) &&
                    knownBounds && ( stationary || options.convertRenderOnlyLeaves ) )
                {
                    visit( *data, [&]( const Properties &node ) {
                        if( node.getName() == GameActorUtil::childStr || &node == data.get() )
                            const_cast<Properties &>( node ).setProperty( GameActorUtil::staticStr, true );
                    } );
                    // Imported city meshes often bake positions into vertices and
                    // leave every actor at the origin. Partition by geometry,
                    // otherwise those buildings all land in one origin cell.
                    const auto x = std::floor( ( double( low.x ) + high.x ) * 0.5 / options.cellSize );
                    const auto z = std::floor( ( double( low.z ) + high.z ) * 0.5 / options.cellSize );
                    if( std::abs( x ) > 10000000 || std::abs( z ) > 10000000 )
                        throw std::runtime_error( "World coordinate exceeds cell index range" );
                    groups[{ int( x ), int( z ) }].push_back( data );
                }
                else persistent.push_back( data );
            };
            for( const auto &actor : scene.getChildrenByName( ApplicationUtil::actorsStr ) )
                partition( *actor, Transform3F::identity() );
            if( scene.hasProperty( GameActorUtil::uuidStr ) )
                partition( scene, Transform3F::identity() );

            auto emit = [&]( const String &id, const Array<SmartPtr<Properties>> &actors,
                             Vector3F minimum, Vector3F maximum ) {
                auto payload = make_ptr<Properties>();
                payload->setProperty( "worldCellVersion", WorldManifest::version );
                payload->setProperty( "id", id );
                u32 count = 0;
                for( const auto &actor : actors ) { payload->addChild( actor ); count += countActors( *actor ); }
                for( const auto &actor : actors )
                {
                    const auto &box = bounds.at( actor.get() );
                    minimum.x = std::min( minimum.x, box.first.x ); minimum.y = std::min( minimum.y, box.first.y ); minimum.z = std::min( minimum.z, box.first.z );
                    maximum.x = std::max( maximum.x, box.second.x ); maximum.y = std::max( maximum.y, box.second.y ); maximum.z = std::max( maximum.z, box.second.z );
                }
                if( !count ) return;
                if( count > options.maximumActorsPerCell )
                    throw std::runtime_error( "An indivisible actor group exceeds the cell actor limit" );
                auto bytes = PropertiesBinarySerializer::serialize( *payload );
                if( bytes.size() > options.maximumCellBytes )
                    throw std::runtime_error( "An indivisible actor group exceeds the cell byte limit" );
                const auto file = id + "_" + StringUtil::getUUID() + ".wpcell";
                write( output / file.c_str(), bytes );
                auto entry = make_ptr<Properties>();
                entry->setName( "cell" );
                entry->setProperty( "id", id ); entry->setProperty( "file", file );
                entry->setProperty( "minimum", minimum ); entry->setProperty( "maximum", maximum );
                entry->setProperty( "bytes", u64( bytes.size() ) );
                entry->setProperty( "decodedBytes", u64( bytes.size() ) * 8 + u64( count ) * 4096 );
                entry->setProperty( "actors", count );
                manifest->addChild( entry );
            };
            // Persistent actors are admitted together so cross-root references resolve.
            emit( "persistent", persistent, Vector3F::zero(), Vector3F::zero() );
            for( const auto &group : groups )
            {
                Array<SmartPtr<Properties>> batch;
                u32 count = 0, page = 0;
                u64 batchBytes = 0;
                const auto x = group.first.first, z = group.first.second;
                auto flush = [&] {
                    if( batch.empty() ) return;
                    emit( "cell_" + StringUtil::toString( x ) + "_" + StringUtil::toString( z ) + "_" +
                              StringUtil::toString( page++ ), batch,
                          Vector3F( x * options.cellSize, -10000000, z * options.cellSize ),
                          Vector3F( ( x + 1 ) * options.cellSize, 10000000, ( z + 1 ) * options.cellSize ) );
                    batch.clear(); count = 0; batchBytes = 0;
                };
                for( const auto &actor : group.second )
                {
                    const auto n = countActors( *actor );
                    const auto actorBytes = PropertiesBinarySerializer::serialize( *actor ).size();
                    // Leave room for the cell envelope and identity fields.
                    if( count + n > options.maximumActorsPerCell ||
                        batchBytes + actorBytes + 1024 > options.maximumCellBytes ) flush();
                    batch.push_back( actor ); count += n; batchBytes += actorBytes;
                }
                flush();
            }
            if( manifest->getChildren().empty() || manifest->getChildren().size() > 65536 )
                throw std::runtime_error( "World must contain between one and 65536 cells" );
            const auto temporary = output / ( "world_" + std::string( StringUtil::getUUID().c_str() ) + ".tmp" );
            const auto destination = output / "world.wpworld";
            write( temporary, PropertiesBinarySerializer::serialize( *manifest ) );
#ifdef _WIN32
            if( !MoveFileExW( temporary.c_str(), destination.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH ) )
                throw std::runtime_error( "Could not publish world manifest" );
#else
            fs::rename( temporary, destination );
#endif
            manifestPath = destination.generic_string().c_str();
            return true;
        }
        catch( const std::exception &e ) { error = e.what(); return false; }
    }

    bool SceneWorld::readManifest( const String &path, WorldManifest &result, String &error )
    {
        error.clear();
        try
        {
            auto bytes = read( fs::path( path.c_str() ), 8ull << 20 );
            auto data = make_ptr<Properties>();
            if( !PropertiesBinarySerializer::deserialize( bytes, *data, &error ) ) return false;
            u32 version = 0;
            data->getPropertyValue( "version", version );
            WorldManifest candidate;
            data->getPropertyValue( "cellSize", candidate.cellSize );
            data->getPropertyValue( "generation", candidate.generation );
            if( version != WorldManifest::version || candidate.generation.empty() ||
                !std::isfinite( candidate.cellSize ) || candidate.cellSize <= 0 )
                throw std::runtime_error( "Unsupported or invalid world manifest" );
            std::set<u64> keys;
            for( const auto &entry : data->getChildrenByName( "cell" ) )
            {
                WorldCell cell;
                entry->getPropertyValue( "id", cell.id ); entry->getPropertyValue( "file", cell.file );
                if( !entry->getPropertyValue( "minimum", cell.minimum ) ||
                    !entry->getPropertyValue( "maximum", cell.maximum ) )
                    throw std::runtime_error( "World cell bounds are missing" );
                String encoded, decoded;
                entry->getPropertyValue( "bytes", encoded ); entry->getPropertyValue( "decodedBytes", decoded );
                auto parseSize = []( const String &value ) -> u64 {
                    if( value.empty() || value.find_first_not_of( "0123456789" ) != String::npos )
                        throw std::runtime_error( "Invalid world byte count" );
                    return std::stoull( value.c_str() );
                };
                cell.bytes = parseSize( encoded ); cell.decodedBytes = parseSize( decoded );
                entry->getPropertyValue( "actors", cell.actors ); cell.key = keyFor( cell.id );
                if( cell.id.empty() || !cellFile( cell.file ) || !finite( cell.minimum ) || !finite( cell.maximum ) ||
                    cell.minimum.x > cell.maximum.x || cell.minimum.y > cell.maximum.y || cell.minimum.z > cell.maximum.z ||
                    !cell.bytes || cell.bytes > ( 64ull << 20 ) || cell.decodedBytes < cell.bytes ||
                    cell.decodedBytes > ( 512ull << 20 ) || !cell.actors || cell.actors > WP_MAX_ACTORS ||
                    !keys.insert( cell.key ).second || candidate.cells.size() >= 65536 )
                    throw std::runtime_error( "Invalid or duplicate world cell" );
                candidate.cells.push_back( std::move( cell ) );
            }
            if( candidate.cells.empty() ) throw std::runtime_error( "World manifest has no cells" );
            result = std::move( candidate );
            return true;
        }
        catch( const std::exception &e ) { error = e.what(); return false; }
    }

    SmartPtr<Properties> SceneWorld::readCell( const String &manifestPath, const WorldCell &cell, String &error )
    {
        error.clear();
        try
        {
            if( !cellFile( cell.file ) ) throw std::runtime_error( "Invalid cell file" );
            auto bytes = read( fs::path( manifestPath.c_str() ).parent_path() / cell.file.c_str(), cell.bytes );
            if( bytes.size() != cell.bytes ) throw std::runtime_error( "Stale or truncated cell" );
            auto data = make_ptr<Properties>();
            if( !PropertiesBinarySerializer::deserialize( bytes, *data, &error ) ) return nullptr;
            u32 version = 0, count = 0;
            String id;
            data->getPropertyValue( "worldCellVersion", version ); data->getPropertyValue( "id", id );
            for( const auto &actor : data->getChildrenByName( ApplicationUtil::actorsStr ) ) count += countActors( *actor );
            if( version != WorldManifest::version || id != cell.id || count != cell.actors )
                throw std::runtime_error( "Cell identity, version or actor count mismatch" );
            return data;
        }
        catch( const std::exception &e ) { error = e.what(); return nullptr; }
    }
}
