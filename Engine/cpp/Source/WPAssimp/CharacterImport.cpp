#include <WPAssimp/WPAssimpPCH.hpp>
#include <WPAssimp/CharacterImport.hpp>

#if WP_ASSIMP_CHARACTER_IMPORT
#    include <WPAssimp/AnimationImport.hpp>
#    include <WPAssimp/AssimpLoader.hpp>
#    include <Workphone/Core/StringUtil.hpp>
#    include <Workphone/Interface/Animation/IAnimation.hpp>
#    include <Workphone/Interface/Mesh/IGraphicsBone.hpp>
#    include <Workphone/Mesh/XMLSkeletonSerializer.hpp>
#    include <Workphone/System/SkeletonResourceCompiler.hpp>
#    include <workphone_graphics_mesh_writer.h>
#    include <workphone_graphics_skinning.h>
#    include <rapidjson/stringbuffer.h>
#    include <rapidjson/writer.h>
#    include <cstdlib>
#    include <map>
#    include <memory>
#    include <set>
#    include <stdexcept>
#endif

namespace workphone::character_import
{
#if WP_ASSIMP_CHARACTER_IMPORT
    namespace
    {
        constexpr unsigned maximumVertices = 65536;
        constexpr unsigned maximumIndices = 4 * 1024 * 1024;

        class CharacterLoader final : public AssimpLoader
        {
        public:
            using AssimpLoader::createSkeleton;
        };

        void require( bool condition, const char *message )
        {
            if( !condition )
            {
                throw std::runtime_error( message );
            }
        }

        bool sameMatrix( const aiMatrix4x4 &a, const aiMatrix4x4 &b )
        {
            for( unsigned row = 0; row < 4; ++row )
            {
                for( unsigned column = 0; column < 4; ++column )
                {
                    const auto delta = a[row][column] - b[row][column];
                    if( !std::isfinite( delta ) || std::abs( delta ) > 0.0001f )
                    {
                        return false;
                    }
                }
            }
            return true;
        }

        bool rigid( const aiMatrix4x4 &transform )
        {
            aiVector3D scale, translation;
            aiQuaternion rotation;
            transform.Decompose( scale, rotation, translation );
            const aiVector3D unit( 1, 1, 1 );
            return animation_import::equivalent( scale, unit ) &&
                   animation_import::finite( rotation ) &&
                   sameMatrix( transform, aiMatrix4x4( unit, rotation, translation ) );
        }

        struct Node
        {
            const aiNode *source;
            aiMatrix4x4 model;
        };

        struct Hierarchy
        {
            std::map<std::string, Node> nodes;
            const aiNode *meshNode = nullptr;
        };

        void collectNodes( const aiNode &node, const aiMatrix4x4 &parent, unsigned depth,
                           Hierarchy &hierarchy )
        {
            require( depth <= 64 && hierarchy.nodes.size() < 8192,
                     "Character source exceeds the node or hierarchy depth limit" );
            const std::string name( node.mName.C_Str() );
            const auto model = parent * node.mTransformation;
            require( !name.empty() && hierarchy.nodes.emplace( name, Node{ &node, model } ).second,
                     "Character source requires unique, nonempty node names" );
            if( node.mNumMeshes )
            {
                require( !hierarchy.meshNode,
                         "Character source requires all mesh primitives on a single node" );
                hierarchy.meshNode = &node;
            }
            for( unsigned i = 0; i < node.mNumChildren; ++i )
            {
                collectNodes( *node.mChildren[i], model, depth + 1, hierarchy );
            }
        }

        void addRigChain( const aiNode *node, std::set<std::string> &rigNodes )
        {
            for( ; node && rigNodes.emplace( node->mName.C_Str() ).second; node = node->mParent )
            {
                require( rigNodes.size() <= WP_SKIN_MAX_JOINTS && rigid( node->mTransformation ),
                         "Character rig exceeds 256 joints or has a non-rigid local bind transform" );
            }
        }

        void validateClips( const aiScene &scene, const std::set<std::string> &rigNodes )
        {
            require( scene.mNumAnimations > 0 && scene.mNumAnimations <= 64,
                     "Character source requires one to 64 skeletal clips" );
            size_t totalTracks = 0, totalSourceKeys = 0;
            for( unsigned i = 0; i < scene.mNumAnimations; ++i )
            {
                const auto &clip = *scene.mAnimations[i];
                require( clip.mNumChannels > 0 && !clip.mNumMeshChannels && !clip.mNumMorphMeshChannels,
                         "Character clips must contain skeletal channels only" );
                totalTracks += clip.mNumChannels;
                require( totalTracks <= 2048, "Character clips exceed the track limit" );
                for( unsigned j = 0; j < clip.mNumChannels; ++j )
                {
                    const auto &channel = *clip.mChannels[j];
                    require( rigNodes.count( channel.mNodeName.C_Str() ) != 0,
                             "Character clip targets a node outside the deformation hierarchy" );
                    // Each combined key uses one or more source keys. Allow
                    // three fully populated channels at the cooked key limit.
                    totalSourceKeys += size_t( channel.mNumPositionKeys ) + channel.mNumRotationKeys +
                                       channel.mNumScalingKeys;
                    require( totalSourceKeys <= 3 * 16384, "Character clips exceed the key limit" );
                }
            }
        }

        void validateSkin( const aiScene &scene, const Hierarchy &hierarchy,
                           const aiMatrix4x4 &meshModel )
        {
            std::set<std::string> rigNodes;
            std::set<unsigned> meshIndices;
            size_t vertices = 0, indices = 0;
            for( unsigned i = 0; i < hierarchy.meshNode->mNumMeshes; ++i )
            {
                const auto index = hierarchy.meshNode->mMeshes[i];
                require( index < scene.mNumMeshes && meshIndices.insert( index ).second,
                         "Character node has invalid or repeated primitive references" );
                const auto &mesh = *scene.mMeshes[index];
                require( mesh.HasPositions() && mesh.HasNormals() && mesh.HasFaces() &&
                             mesh.HasBones() && !mesh.mNumAnimMeshes,
                         "Character primitives require weighted geometry without morph targets" );
                vertices += mesh.mNumVertices;
                indices += size_t( mesh.mNumFaces ) * 3;
                require( vertices <= maximumVertices && indices <= maximumIndices,
                         "Character geometry exceeds the cooked skin vertex or index limit" );
                std::set<std::string> boneNames;
                for( unsigned j = 0; j < mesh.mNumBones; ++j )
                {
                    const auto &bone = *mesh.mBones[j];
                    const std::string name( bone.mName.C_Str() );
                    const auto found = hierarchy.nodes.find( name );
                    require( found != hierarchy.nodes.end() && boneNames.insert( name ).second,
                             "Character primitive has a missing or duplicate joint" );
                    // The runtime derives inverse bind from the cooked rig.
                    // Verify the source offset before baking mesh space into
                    // model space, so skinning cannot silently change.
                    require( sameMatrix( found->second.model * bone.mOffsetMatrix, meshModel ),
                             "Character inverse bind differs from the mesh and joint bind hierarchy" );
                    addRigChain( found->second.source, rigNodes );
                }
            }
            require( meshIndices.size() == scene.mNumMeshes,
                     "Character source has unreferenced mesh primitives" );
            validateClips( scene, rigNodes );
        }

        wp_vec3f nativeVector( const aiVector3D &value )
        {
            require( animation_import::finite( value ), "Character mesh has non-finite vertex data" );
            return { value.x, value.y, value.z };
        }

        void addWeights( const aiMesh &mesh, const MeshSkeleton &rig, wp_skin_vertex *skin )
        {
            for( unsigned i = 0; i < mesh.mNumBones; ++i )
            {
                const auto &source = *mesh.mBones[i];
                const auto bone = rig.getBone( source.mName.C_Str() );
                require( bool( bone ), "Imported skin joint is missing from the serialized rig" );
                for( unsigned j = 0; j < source.mNumWeights; ++j )
                {
                    const auto &weight = source.mWeights[j];
                    require( weight.mVertexId < mesh.mNumVertices && std::isfinite( weight.mWeight ) &&
                                 weight.mWeight >= 0 && weight.mWeight <= 1,
                             "Character contains an invalid skin weight or vertex index" );
                    if( weight.mWeight == 0 )
                    {
                        continue;
                    }
                    auto &vertex = skin[weight.mVertexId];
                    unsigned slot = 0;
                    while( slot < WP_SKIN_INFLUENCES && vertex.weights[slot] > 0 )
                    {
                        require( vertex.joints[slot] != bone->getBoneHandle(),
                                 "Character vertex repeats a joint weight" );
                        ++slot;
                    }
                    require( slot < WP_SKIN_INFLUENCES, "Character vertex has more than four weights" );
                    vertex.joints[slot] = bone->getBoneHandle();
                    vertex.weights[slot] = weight.mWeight;
                }
            }
            for( unsigned i = 0; i < mesh.mNumVertices; ++i )
            {
                double sum = 0;
                for( auto weight : skin[i].weights )
                {
                    sum += weight;
                }
                require( sum > 0, "Character contains a vertex without a positive skin weight" );
                for( auto &weight : skin[i].weights )
                {
                    weight = static_cast<float>( weight / sum );
                }
            }
        }

        void writeGeometry( const aiScene &scene, const aiNode &node, const aiMatrix4x4 &meshModel,
                            const MeshSkeleton &rig, Sources &sources,
                            const resource::ResourceID &meshId, const resource::ResourceID &skeletonId )
        {
            std::vector<wp_graphics_mesh_vertex_pnt> vertices;
            std::vector<wp_u32> indices;
            std::vector<wp_skin_vertex> skin;
            std::unique_ptr<wp_graphics_mesh, decltype( &wp_graphics_mesh_destroy )> native(
                wp_graphics_mesh_create(), wp_graphics_mesh_destroy );
            require( bool( native ), "Cannot allocate character geometry" );
            const aiMatrix3x3 normalModel( meshModel );
            for( unsigned i = 0; i < node.mNumMeshes; ++i )
            {
                const auto &mesh = *scene.mMeshes[node.mMeshes[i]];
                const auto firstVertex = static_cast<wp_u32>( vertices.size() );
                const auto firstIndex = static_cast<wp_u32>( indices.size() );
                for( unsigned j = 0; j < mesh.mNumVertices; ++j )
                {
                    const auto uv = mesh.HasTextureCoords( 0 ) ? mesh.mTextureCoords[0][j] : aiVector3D();
                    require( animation_import::finite( uv ), "Character mesh has non-finite UVs" );
                    const auto position = nativeVector( meshModel * mesh.mVertices[j] );
                    const auto normal = nativeVector( normalModel * mesh.mNormals[j] );
                    vertices.push_back( { position, normal, { uv.x, uv.y } } );
                }
                for( unsigned j = 0; j < mesh.mNumFaces; ++j )
                {
                    const auto &face = mesh.mFaces[j];
                    require( face.mNumIndices == 3, "Character geometry must contain only triangles" );
                    for( unsigned k = 0; k < 3; ++k )
                    {
                        require( face.mIndices[k] < mesh.mNumVertices, "Character index is out of range" );
                        indices.push_back( firstVertex + face.mIndices[k] );
                    }
                }
                require( wp_graphics_mesh_add_submesh(
                             native.get(), firstIndex, static_cast<wp_u32>( indices.size() ) - firstIndex,
                             0 ) >= 0,
                         "Cannot store character primitive range" );
                skin.resize( vertices.size() );
                addWeights( mesh, rig, skin.data() + firstVertex );
            }
            require( wp_graphics_mesh_set_vertices( native.get(), WORKPHONE_VERTEX_FORMAT_PNT,
                                                    vertices.data(), static_cast<wp_u32>( vertices.size() ) ) &&
                         wp_graphics_mesh_set_indices_u32( native.get(), indices.data(),
                                                          static_cast<wp_u32>( indices.size() ) ),
                     "Cannot allocate character mesh buffers" );
            wp_graphics_mesh_compute_aabb( native.get() );
            void *bytes = nullptr;
            uint32_t size = 0;
            require( wp_graphics_mesh_write_to_buffer( native.get(), &bytes, &size ) != 0,
                     "Cannot serialize character WPM1 geometry" );
            const std::unique_ptr<void, decltype( &std::free )> buffer( bytes, std::free );
            const auto start = static_cast<const u8 *>( bytes );
            sources.mesh.assign( start, start + size );

            rapidjson::StringBuffer json;
            rapidjson::Writer<rapidjson::StringBuffer> writer( json );
            writer.StartObject();
            writer.Key( "version" );
            writer.Uint( 1 );
            writer.Key( "mesh" );
            writer.String( meshId.str().c_str() );
            writer.Key( "skeleton" );
            writer.String( skeletonId.str().c_str() );
            writer.Key( "vertices" );
            writer.StartArray();
            for( const auto &vertex : skin )
            {
                writer.StartObject();
                writer.Key( "joints" );
                writer.StartArray();
                for( const auto joint : vertex.joints )
                {
                    writer.Uint( joint );
                }
                writer.EndArray();
                writer.Key( "weights" );
                writer.StartArray();
                for( const auto weight : vertex.weights )
                {
                    writer.Double( weight );
                }
                writer.EndArray();
                writer.EndObject();
            }
            writer.EndArray();
            writer.EndObject();
            require( json.GetSize() <= ( 8u << 20 ), "Character skin exceeds the cooker source limit" );
            sources.skin.assign( json.GetString(), json.GetSize() );
        }
    }
#endif

    bool importSources( const String &inputPath, const resource::ResourceID &meshId,
                        const resource::ResourceID &skeletonId, Sources &output, String &error )
    {
#if WP_ASSIMP_CHARACTER_IMPORT
        error.clear();
        try
        {
            require( meshId.isValid() && !meshId.isSubResource() && meshId.str().size() <= 1024 &&
                         meshId.type() == resource::ResourceTypeID( "mesh" ) &&
                         skeletonId.isValid() && !skeletonId.isSubResource() &&
                         skeletonId.str().size() <= 1024 &&
                         skeletonId.type() == resource::ResourceTypeID( "skelres" ),
                     "Character import requires mesh and skelres output identities" );
            Assimp::Importer importer;
            const auto scene = importer.ReadFile( inputPath.c_str(), aiProcess_Triangulate |
                                                                       aiProcess_GenSmoothNormals |
                                                                       aiProcess_ValidateDataStructure );
            if( !scene )
            {
                throw std::runtime_error( std::string( "Assimp character import failed: " ) +
                                          importer.GetErrorString() );
            }
            require( scene->mRootNode != nullptr, "Character source has no scene hierarchy" );
            Hierarchy hierarchy;
            collectNodes( *scene->mRootNode, aiMatrix4x4(), 1, hierarchy );
            require( hierarchy.meshNode != nullptr, "Character source has no mesh node" );
            const auto meshModel = hierarchy.nodes.at( hierarchy.meshNode->mName.C_Str() ).model;
            require( rigid( meshModel ), "Character mesh bind transform must be rigid" );
            validateSkin( *scene, hierarchy, meshModel );

            auto loader = make_ptr<CharacterLoader>();
            const auto rig = dynamic_pointer_cast<MeshSkeleton>( loader->createSkeleton( scene ) );
            require( rig && rig->getAnimations().size() == scene->mNumAnimations,
                     "Character contains a duplicate or unsupported skeletal clip" );
            for( unsigned i = 0; i < scene->mNumAnimations; ++i )
            {
                const auto &source = *scene->mAnimations[i];
                const auto name = source.mName.length ? String( source.mName.C_Str() )
                                                      : String( "Animation_" ) + StringUtil::toString( i );
                const auto clip = rig->getAnimation( name );
                require( clip && clip->_getNodeTrackList().size() == source.mNumChannels,
                         "Character clip lost one or more skeletal tracks during import" );
            }
            Sources candidate;
            XMLSkeletonSerializer serializer;
            candidate.skeleton = serializer.exportSkeletonData( *rig );
            if( !resource::SkeletonResourceCompiler::validateSource( candidate.skeleton, error ) )
            {
                return false;
            }
            writeGeometry( *scene, *hierarchy.meshNode, meshModel, *rig, candidate, meshId, skeletonId );
            output = std::move( candidate );
            return true;
        }
        catch( const std::exception &exception )
        {
            error = exception.what();
            return false;
        }
#else
        (void)inputPath;
        (void)meshId;
        (void)skeletonId;
        (void)output;
        error = "Character cooking requires Assimp and native Workphone graphics support";
        return false;
#endif
    }
}
