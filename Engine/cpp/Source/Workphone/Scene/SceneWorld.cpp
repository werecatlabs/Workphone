#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/SceneWorld.hpp>
#include <Workphone/Scene/GameActorUtil.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/PropertiesBinarySerializer.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/IO/FileDataStream.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/ApplicationUtil.hpp>
#include <Workphone/Mesh/Mesh.hpp>
#include <Workphone/Mesh/MeshUtil.hpp>
#include <Workphone/Mesh/MeshSerializer.hpp>
#include <WorkphoneGraphics/workphone_graphics_mesh.h>
#include <WorkphoneGraphics/workphone_graphics_mesh_reader.h>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <unordered_map>
#include <cmath>
#include <limits>
#include <memory>
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
                   fs::u8path( path ).extension() == ".wpcell";
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
        void addUniqueDefinition( Array<const Properties *> &definitions, const Properties &data )
        {
            if( std::find( definitions.begin(), definitions.end(), &data ) == definitions.end() )
                definitions.push_back( &data );
        }
        void addComponentDefinitions( Array<const Properties *> &definitions, const Properties &data )
        {
            if( data.hasProperty( GameActorUtil::componentTypeStr ) )
            {
                addUniqueDefinition( definitions, data );
                return;
            }
            for( const auto &child : data.getChildrenByName( GameActorUtil::componentStr ) )
                addComponentDefinitions( definitions, *child );
        }
        void addActorDefinitions( Array<const Properties *> &definitions, const Properties &data )
        {
            if( data.getNumProperties() > 0 || data.hasChild( GameActorUtil::localTransformStr ) ||
                data.hasChild( GameActorUtil::worldTransformStr ) || data.hasChild( GameActorUtil::componentStr ) ||
                data.hasChild( GameActorUtil::componentsStr ) )
            {
                addUniqueDefinition( definitions, data );
                return;
            }
            for( const auto &child : data.getChildren() )
                if( child->getName() == GameActorUtil::childStr || child->getName() == GameActorUtil::childrenStr )
                    addActorDefinitions( definitions, *child );
        }
        // Match GameActorUtil::createInstanceData's definition schema. Nested
        // resource data may also contain uuid fields and must never be indexed.
        void visitDefinitions( const Properties &actor,
                               const std::function<void( const Properties &, bool )> &fn )
        {
            fn( actor, true );
            Array<const Properties *> components, children;
            for( const auto &child : actor.getChildren() )
                if( child->getName() == GameActorUtil::componentStr || child->getName() == GameActorUtil::componentsStr )
                    addComponentDefinitions( components, *child );
                else if( child->getName() == GameActorUtil::childStr || child->getName() == GameActorUtil::childrenStr )
                    addActorDefinitions( children, *child );
            for( const auto *component : components ) fn( *component, false );
            for( const auto *child : children ) visitDefinitions( *child, fn );
        }
        bool validObjectId( const String &id )
        {
            if( id.empty() || id.size() > 256 ) return false;
            for( const auto c : id )
                if( static_cast<unsigned char>( c ) <= 32 || c == ';' || c == 127 ) return false;
            return true;
        }
        void collectIdentities( const Properties &root, Array<String> &actorIds, Array<String> &objectIds,
                                std::set<String> &definitions )
        {
            visitDefinitions( root, [&]( const Properties &definition, bool actor ) {
                const auto uuid = definition.hasProperty( GameActorUtil::uuidStr ) ?
                    definition.getPropertyObject( GameActorUtil::uuidStr ).getValue() : String();
                if( !validObjectId( uuid ) || !definitions.insert( uuid ).second )
                    throw std::runtime_error( "Missing, invalid or duplicate world object identity" );
                if( actor ) actorIds.push_back( uuid );
                objectIds.push_back( uuid );
            } );
        }
        void readIdentities( const Properties &entry, const char *name, Array<String> &identities )
        {
            const auto encoded = entry.getPropertyObject( name ).getValue();
            // Properties string arrays are semicolon-terminated. Reject a
            // malformed tail rather than silently dropping its last identity.
            if( encoded.empty() || encoded.back() != ';' )
                throw std::runtime_error( "Invalid world identity inventory encoding" );
            entry.getPropertyValue( name, identities );
            for( const auto &id : identities )
                if( !validObjectId( id ) )
                    throw std::runtime_error( "Invalid world object identity" );
        }
        u32 countActors( const Properties &root )
        {
            u32 count = 0;
            visitDefinitions( root, [&]( const Properties &, bool actor ) { if( actor ) ++count; } );
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
        void writeProxyMesh( const fs::path &file,
                             const Array<std::pair<Vector3F, Vector3F>> &boxes )
        {
            Array<Vector3<real_Num>> vertices, normals;
            Array<Vector2<real_Num>> uvs;
            Array<u32> indices;
            // Four independent vertices per face keep the building corners hard.
            const unsigned faces[6][4] = { {0,4,6,2}, {1,3,7,5}, {0,1,5,4},
                                           {2,6,7,3}, {0,2,3,1}, {4,5,7,6} };
            const Vector3<real_Num> faceNormals[6] = { {-1,0,0}, {1,0,0}, {0,-1,0},
                                                       {0,1,0}, {0,0,-1}, {0,0,1} };
            for( const auto &box : boxes )
                for( unsigned face = 0; face < 6; ++face )
                {
                    const auto first = u32( vertices.size() );
                    for( unsigned corner = 0; corner < 4; ++corner )
                    {
                        const auto bits = faces[face][corner];
                        vertices.emplace_back( bits & 1 ? box.second.x : box.first.x,
                                               bits & 2 ? box.second.y : box.first.y,
                                               bits & 4 ? box.second.z : box.first.z );
                        normals.push_back( faceNormals[face] );
                        uvs.emplace_back( corner == 1 || corner == 2 ? 1.f : 0.f, corner >= 2 ? 1.f : 0.f );
                    }
                    for( auto index : { 0u, 1u, 2u, 0u, 2u, 3u } ) indices.push_back( first + index );
                }
            auto mesh = dynamic_pointer_cast<workphone::Mesh>( MeshUtil::createMesh( vertices, normals, uvs, indices ) );
            if( !mesh ) throw std::runtime_error( "Could not create HLOD proxy mesh" );
            mesh->updateAABB( true );
            // Use the established cooked serializer: both the CPU resource
            // manager and the native renderer can reload this file directly.
            // The filename overload opens a narrow fstream. Open the native
            // filesystem path here, then retain the established mesh wire codec.
            auto output = std::make_unique<std::fstream>( file, std::ios::binary | std::ios::out | std::ios::trunc );
            if( !*output ) throw std::runtime_error( "Could not open HLOD proxy mesh" );
            auto stream = make_ptr<FileDataStream>( String( file.generic_u8string().c_str() ), output.get(), 0, true );
            output.release();  // FileDataStream owns the successfully wrapped stream.
            MeshSerializer serializer;
            try { serializer.exportMesh( mesh.get(), stream ); stream->close(); }
            catch( ... ) { stream->close(); throw; }
            if( !fs::exists( file ) || !fs::file_size( file ) )
                throw std::runtime_error( "Could not write HLOD proxy mesh" );
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
            auto output = fs::u8path( directory.c_str() );
            fs::path proxyDirectory;
            if( options.generateHlod )
            {
                if( options.assetRoot.empty() ) throw std::runtime_error( "HLOD cooking requires an asset root" );
                proxyDirectory = fs::absolute( output ).lexically_normal().lexically_relative(
                    fs::absolute( fs::u8path( options.assetRoot.c_str() ) ).lexically_normal() );
                if( proxyDirectory.empty() || *proxyDirectory.begin() == ".." )
                    throw std::runtime_error( "HLOD output must be inside the asset root" );
            }
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
                    {
                        ++references[property.getValue().c_str()];
                        if( property.getValue().find( ';' ) != String::npos )
                        {
                            Array<String> values;
                            StringUtil::parseArray( property.getValue(), values );
                            for( const auto &value : values ) ++references[value.c_str()];
                        }
                    }
            } );
            std::map<std::pair<int,int>, Array<SmartPtr<Properties>>> groups;
            std::map<const Properties *, std::pair<Vector3F, Vector3F>> bounds;
            std::set<const Properties *> proxyCandidates;
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
                            auto file = fs::u8path( meshPath.c_str() );
                            if( !file.is_absolute() ) file = fs::u8path( options.assetRoot.c_str() ) / file;
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
                    bool proxyVisible = enabled && visible;
                    visit( actor, [&]( const Properties &node ) {
                        bool on = true, shown = true;
                        node.getPropertyValue( GameActorUtil::enabledStr, on ); node.getPropertyValue( "visible", shown );
                        proxyVisible &= on && shown;
                    } );
                    if( proxyVisible && hasGeometryBounds && finite( low ) && finite( high ) &&
                        high.x > low.x && high.y > low.y && high.z > low.z )
                        proxyCandidates.insert( data.get() );
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

            std::set<String> definitions;
            std::function<void( const String &, const Array<SmartPtr<Properties>> &,
                                Vector3F, Vector3F, const String & )> emit;
            emit = [&]( const String &id, const Array<SmartPtr<Properties>> &actors,
                        Vector3F minimum, Vector3F maximum, const String &proxyOf ) {
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
                Array<String> actorIds, objectIds;
                for( const auto &actor : actors )
                    collectIdentities( *actor, actorIds, objectIds, definitions );
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
                entry->setProperty( "actorIds", actorIds ); entry->setProperty( "objectIds", objectIds );
                if( !proxyOf.empty() ) entry->setProperty( "proxyOf", proxyOf );
                manifest->addChild( entry );
                if( options.generateHlod && id != "persistent" && proxyOf.empty() )
                {
                    Array<std::pair<Vector3F, Vector3F>> boxes;
                    for( const auto &actor : actors )
                    {
                        // A cell containing unbounded/degenerate content gets no
                        // proxy: replacing only part of it would hide content.
                        if( !proxyCandidates.count( actor.get() ) ) return;
                        boxes.push_back( bounds.at( actor.get() ) );
                    }
                    const auto meshFile = "hlod_" + id + "_" + StringUtil::getUUID() + ".fbmeshbin";
                    writeProxyMesh( output / meshFile.c_str(), boxes );
                    auto proxy = make_ptr<Properties>(); proxy->setName( ApplicationUtil::actorsStr );
                    proxy->setProperty( "label", "HLOD " + id );
                    proxy->setProperty( GameActorUtil::uuidStr, StringUtil::getUUID() );
                    proxy->setProperty( GameActorUtil::staticStr, true );
                    setRootTransform( *proxy, Transform3F::identity() );
                    for( const auto &type : { String( "Mesh" ), String( "MeshRenderer" ) } )
                    {
                        auto component = make_ptr<Properties>(); component->setName( GameActorUtil::componentStr );
                        component->setProperty( GameActorUtil::uuidStr, StringUtil::getUUID() );
                        component->setProperty( GameActorUtil::componentTypeStr, "workphone::scene::" + type );
                        if( type == "Mesh" ) component->setProperty( "Mesh Path",
                            ( proxyDirectory / meshFile.c_str() ).generic_u8string().c_str() );
                        proxy->addChild( component );
                    }
                    bounds[proxy.get()] = { minimum, maximum };
                    emit( "hlod_" + id, { proxy }, minimum, maximum, id );
                }
            };
            // Persistent actors are admitted together so cross-root references resolve.
            emit( "persistent", persistent, Vector3F::zero(), Vector3F::zero(), "" );
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
                          Vector3F( ( x + 1 ) * options.cellSize, 10000000, ( z + 1 ) * options.cellSize ), "" );
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
            const auto manifestBytes = PropertiesBinarySerializer::serialize( *manifest );
            if( manifestBytes.size() > ( 8ull << 20 ) )
                throw std::runtime_error( "World manifest exceeds its admission limit" );
            write( temporary, manifestBytes );
#ifdef _WIN32
            if( !MoveFileExW( temporary.c_str(), destination.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH ) )
                throw std::runtime_error( "Could not publish world manifest" );
#else
            fs::rename( temporary, destination );
#endif
            manifestPath = destination.generic_u8string().c_str();
            return true;
        }
        catch( const std::exception &e ) { error = e.what(); return false; }
    }

    bool SceneWorld::readManifest( const String &path, WorldManifest &result, String &error )
    {
        error.clear();
        try
        {
            auto bytes = read( fs::u8path( path.c_str() ), 8ull << 20 );
            auto data = make_ptr<Properties>();
            if( !PropertiesBinarySerializer::deserialize( bytes, *data, &error ) ) return false;
            u32 version = 0;
            data->getPropertyValue( "version", version );
            WorldManifest candidate;
            data->getPropertyValue( "cellSize", candidate.cellSize );
            data->getPropertyValue( "generation", candidate.generation );
            if( ( version != 1 && version != WorldManifest::version ) || candidate.generation.empty() ||
                !std::isfinite( candidate.cellSize ) || candidate.cellSize <= 0 )
                throw std::runtime_error( "Unsupported or invalid world manifest" );
            std::set<u64> keys;
            std::set<String> definitions;
            std::map<u64, String> proxies;
            std::map<String, u32> byId;
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
                String proxyOf; entry->getPropertyValue( "proxyOf", proxyOf );
                if( !proxyOf.empty() )
                {
                    if( version < 2 || proxyOf == "persistent" || cell.id == "persistent" )
                        throw std::runtime_error( "Invalid HLOD proxy ownership" );
                    cell.detailKey = keyFor( proxyOf ); proxies.emplace( cell.key, proxyOf );
                }
                if( cell.id.empty() || !cellFile( cell.file ) || !finite( cell.minimum ) || !finite( cell.maximum ) ||
                    cell.minimum.x > cell.maximum.x || cell.minimum.y > cell.maximum.y || cell.minimum.z > cell.maximum.z ||
                    !cell.bytes || cell.bytes > ( 64ull << 20 ) || cell.decodedBytes < cell.bytes ||
                    cell.decodedBytes > ( 512ull << 20 ) || !cell.actors || cell.actors > WP_MAX_ACTORS ||
                    !keys.insert( cell.key ).second || candidate.cells.size() >= 65536 )
                    throw std::runtime_error( "Invalid or duplicate world cell" );
                const auto hasActorIds = entry->hasProperty( "actorIds" );
                const auto hasObjectIds = entry->hasProperty( "objectIds" );
                if( hasActorIds != hasObjectIds )
                    throw std::runtime_error( "Incomplete world identity inventories" );
                if( hasActorIds )
                {
                    readIdentities( *entry, "actorIds", cell.actorIds );
                    readIdentities( *entry, "objectIds", cell.objectIds );
                    if( cell.actorIds.size() != cell.actors || cell.objectIds.size() < cell.actorIds.size() )
                        throw std::runtime_error( "World identity inventory count mismatch" );
                    std::set<String> actors( cell.actorIds.begin(), cell.actorIds.end() );
                    std::set<String> objects( cell.objectIds.begin(), cell.objectIds.end() );
                    if( actors.size() != cell.actorIds.size() || objects.size() != cell.objectIds.size() )
                        throw std::runtime_error( "Duplicate world object identity" );
                    for( const auto &uuid : actors )
                        if( !objects.count( uuid ) )
                            throw std::runtime_error( "World actor identity is missing from its object inventory" );
                    for( const auto &uuid : objects )
                        if( !definitions.insert( uuid ).second )
                            throw std::runtime_error( "Duplicate world object identity across cells" );
                }
                byId.emplace( cell.id, u32( candidate.cells.size() ) );
                candidate.cells.push_back( std::move( cell ) );
            }
            if( candidate.cells.empty() ) throw std::runtime_error( "World manifest has no cells" );
            for( auto &proxy : candidate.cells ) if( proxy.detailKey )
            {
                auto found = byId.find( proxies.at( proxy.key ) );
                if( found == byId.end() ) throw std::runtime_error( "HLOD proxy references an unknown detail cell" );
                auto &detail = candidate.cells[found->second];
                if( detail.detailKey || detail.proxyKey ||
                    proxy.minimum != detail.minimum || proxy.maximum != detail.maximum )
                    throw std::runtime_error( "HLOD proxy must reference one unique detail cell with matching coverage" );
                detail.proxyKey = proxy.key;
            }
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
            auto bytes = read( fs::u8path( manifestPath.c_str() ).parent_path() / fs::u8path( cell.file.c_str() ), cell.bytes );
            if( bytes.size() != cell.bytes ) throw std::runtime_error( "Stale or truncated cell" );
            auto data = make_ptr<Properties>();
            if( !PropertiesBinarySerializer::deserialize( bytes, *data, &error ) ) return nullptr;
            u32 version = 0, count = 0;
            String id;
            data->getPropertyValue( "worldCellVersion", version ); data->getPropertyValue( "id", id );
            for( const auto &actor : data->getChildrenByName( ApplicationUtil::actorsStr ) ) count += countActors( *actor );
            if( ( version != 1 && version != WorldManifest::version ) || id != cell.id || count != cell.actors )
                throw std::runtime_error( "Cell identity, version or actor count mismatch" );
            if( !cell.actorIds.empty() || !cell.objectIds.empty() )
            {
                Array<String> actorIds, objectIds;
                std::set<String> definitions;
                for( const auto &actor : data->getChildrenByName( ApplicationUtil::actorsStr ) )
                    collectIdentities( *actor, actorIds, objectIds, definitions );
                if( actorIds.size() != cell.actorIds.size() || objectIds.size() != cell.objectIds.size() ||
                    std::set<String>( actorIds.begin(), actorIds.end() ) !=
                        std::set<String>( cell.actorIds.begin(), cell.actorIds.end() ) ||
                    std::set<String>( objectIds.begin(), objectIds.end() ) !=
                        std::set<String>( cell.objectIds.begin(), cell.objectIds.end() ) )
                    throw std::runtime_error( "Cell object identities do not match the manifest inventories" );
            }
            return data;
        }
        catch( const std::exception &e ) { error = e.what(); return nullptr; }
    }
}
