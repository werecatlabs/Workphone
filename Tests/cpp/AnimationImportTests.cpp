#include <WPAssimp/AssimpLoader.hpp>
#include <WPAssimp/AnimationImport.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/FactoryManager.hpp>
#include <Workphone/System/TimerMT.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Animation/Animation.hpp>
#include <Workphone/Animation/ActorAnimationTrack.hpp>
#include <Workphone/Animation/AnimationTimeIndex.hpp>
#include <Workphone/Animation/KeyFrameTransform3.hpp>
#include <Workphone/Mesh/MeshSkeleton.hpp>
#include <Workphone/Scene/GameActor.hpp>
#include <Workphone/Scene/Transform.hpp>
#include <cmath>
#include <cstdio>
#include <limits>
#include <stdexcept>

#if WP_ANIMATION_IMPORTED_RESOURCES
#include <WPAssimp/CharacterImport.hpp>
#include <WPGraphics/ClawMesh.hpp>
#include <WPGraphics/Resources/MeshResourceCompiler.hpp>
#include <WPGraphics/Resources/SkinnedMeshResource.hpp>
#include <WPSQLite/ResourceCompilationDatabase.hpp>
#include <Workphone/Scene/Components/Animator.hpp>
#include <Workphone/System/ResourceCompilerRegistry.hpp>
#include <Workphone/System/ResourceSystem.hpp>
#include <Workphone/System/SkeletonResourceCompiler.hpp>
#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#endif

using namespace workphone;

namespace
{
    void require( bool condition, const char *message )
    {
        if( !condition ) throw std::runtime_error( message );
    }

    bool closeValue( double a, double b ) { return std::abs( a - b ) < 1e-4; }

    class ImportHarness : public AssimpLoader
    {
    public:
        using AssimpLoader::importAnimations;
        using AssimpLoader::importSceneAnimations;
        void bindActor( const String &name, SmartPtr<scene::IGameActor> actor )
        {
            m_sceneActorsByName[name] = actor;
        }
    };

    SmartPtr<scene::GameActor> actor()
    {
        auto value = make_ptr<scene::GameActor>();
        value->setTransform( make_ptr<scene::Transform>() );
        return value;
    }

    SmartPtr<MeshSkeleton> skeleton()
    {
        auto value = make_ptr<MeshSkeleton>();
        value->createBone( "Root", 0 );
        value->createBone( "Child", 1 );
        return value;
    }

    void importedRoutes()
    {
        Assimp::Importer importer;
        const auto source = importer.ReadFile( WP_TEST_SOURCE_ROOT "/Tests/cpp/Fixtures/Animation/analytic_channels.gltf", 0 );
        if( !source ) throw std::runtime_error( importer.GetErrorString() );
        require( source->mNumAnimations == 1, "committed glTF has one real imported clip" );
        auto loader = make_ptr<ImportHarness>();
        auto root = actor();
        auto child = actor();
        loader->bindActor( "Root", root );
        loader->bindActor( "Child", child );
        auto sceneClips = loader->importSceneAnimations( source );
        require( sceneClips.size() == 1, "real scene import creates the clip" );
        sceneClips.front()->apply( 1.0f, 1.0f, 1.0f );
        require( closeValue( root->getTransform()->getPosition().X(), 4 ),
                 "merged rotation key interpolates independent translation channel" );
        require( closeValue( child->getTransform()->getPosition().Y(), 2 ),
                 "missing child translation retains source bind pose" );
        const auto childDirection = child->getTransform()->getOrientation() * Vector3<real_Num>::unitX();
        require( closeValue( childDirection.X(), std::sqrt( 0.5 ) ) &&
                 closeValue( childDirection.Y(), std::sqrt( 0.5 ) ),
                 "sparse child track samples its own key grid, not a clip-wide key index" );
        sceneClips.front()->apply( 0.5f, 1.0f, 1.0f );
        const auto direction = root->getTransform()->getOrientation() * Vector3<real_Num>::unitX();
        require( closeValue( root->getTransform()->getPosition().X(), 3 ) &&
                 closeValue( direction.X(), std::sqrt( 0.5 ) ) &&
                 closeValue( direction.Z(), -std::sqrt( 0.5 ) ),
                 "actual runtime interpolates position and quaternion between imported keys" );

        auto first = skeleton();
        auto second = skeleton();
        loader->importAnimations( source, first );
        auto clip = dynamic_pointer_cast<Animation>( first->getAnimation( "Analytic" ) );
        require( clip != nullptr, "real skeletal import creates the clip" );
        clip->apply( first.get(), 0.5f, 1.0f, 1.0f );
        clip->apply( second.get(), 1.0f, 1.0f, 1.0f );
        require( closeValue( first->getBone( "Root" )->getPosition().X(), 3 ) &&
                 closeValue( second->getBone( "Root" )->getPosition().X(), 4 ),
                 "a shared imported clip targets each skeleton independently" );
        require( closeValue( second->getBone( "Child" )->getPosition().Y(), 2 ),
                 "skeletal route uses bind translation for missing channels" );
        auto unmatched = make_ptr<MeshSkeleton>();
        clip->apply( unmatched.get(), 1.5f, 1.0f, 1.0f );
        require( closeValue( first->getBone( "Root" )->getPosition().X(), 3 ),
                 "missing destination joint cannot mutate the original imported rig" );

        // Reimport malformed replacement under the same name must retain old key data.
        aiNodeAnim *channel = nullptr;
        for( unsigned i = 0; i < source->mAnimations[0]->mNumChannels; ++i )
            if( String( source->mAnimations[0]->mChannels[i]->mNodeName.C_Str() ) == "Root" )
                channel = source->mAnimations[0]->mChannels[i];
        require( channel && channel->mNumPositionKeys >= 2, "fixture root channel layout" );
        const auto saved = channel->mPositionKeys[1].mTime;
        channel->mPositionKeys[1].mTime = channel->mPositionKeys[0].mTime;
        loader->importAnimations( source, first );
        require( loader->importSceneAnimations( source ).empty(), "invalid scene clip fails as a whole" );
        clip->apply( first.get(), 1.0f, 1.0f, 1.0f );
        require( closeValue( first->getBone( "Root" )->getPosition().X(), 4 ),
                 "invalid replacement preserves prior skeletal animation samples" );
        channel->mPositionKeys[1].mTime = saved;

        auto legacy = make_ptr<Animation>();
        legacy->setLength( 2 );
        auto legacyTrack = legacy->addTrack( StringUtil::getHash( "ActorAnimationTrack" ),
                                            0, first->getBone( "Root" ) );
        auto legacyKey = dynamic_pointer_cast<KeyFrameTransform3>( legacyTrack->createKeyFrame( 0 ) );
        require( legacyKey && legacyTrack->getNumKeyFrames() == 1 && legacyTrack->getKeyFrame( 0 ).get() == legacyKey.get(),
                 "creating the initial transform key returns the actual retained track key" );
        legacyKey->setPosition( { 8, 0, 0 } );
        legacy->apply( second.get(), 0, 1, 1 );
        require( closeValue( second->getBone( "Root" )->getPosition().X(), 8 ) &&
                 closeValue( first->getBone( "Root" )->getPosition().X(), 4 ),
                 "legacy bone-bound tracks resolve their bone name on the requested skeleton" );
        const auto channelCount = source->mAnimations[0]->mNumChannels;
        source->mAnimations[0]->mNumChannels = 1;
        loader->importAnimations( source, first );
        require( clip->getNodeTracks().size() == 1 && clip->getNumNodeTracks() == 1,
                 "valid reimport retires removed channels and prior owned tracks" );
        const auto duration = source->mAnimations[0]->mDuration;
        source->mAnimations[0]->mNumChannels = 0;
        source->mAnimations[0]->mDuration = std::numeric_limits<double>::quiet_NaN();
        loader->importAnimations( source, first );
        source->mAnimations[0]->mDuration = duration;
        source->mAnimations[0]->mNumChannels = channelCount;
        require( clip->getNodeTracks().size() == 1 && closeValue( clip->getLength(), 2 ),
                 "invalid empty replacement metadata cannot clear a prior clip" );
        std::puts( "glTF scene/skeletal import, runtime interpolation and independent targets: PASS" );
    }

#if WP_ANIMATION_IMPORTED_RESOURCES
    namespace fs = std::filesystem;
    using namespace resource;

    struct CharacterWorkspace
    {
        fs::path folder = fs::temp_directory_path() /
                          fs::u8path( ( "wp-imported-skin-" + StringUtil::getUUID() ).c_str() );

        CharacterWorkspace()
        {
            require( fs::create_directory( folder ), "Create private character import workspace" );
        }

        ~CharacterWorkspace()
        {
            std::error_code ignored;
            fs::remove_all( folder, ignored );
        }
    };

    String pathText( const fs::path &path ) { return path.generic_u8string().c_str(); }

    void writeSource( const fs::path &path, const void *bytes, size_t size )
    {
        std::ofstream file( path, std::ios::binary );
        file.write( static_cast<const char *>( bytes ), static_cast<std::streamsize>( size ) );
        file.close();
        require( bool( file ), "Write imported character authoring source" );
    }

    void requireResource( bool success, const String &error, const char *step )
    {
        if( !success ) throw std::runtime_error( std::string( step ) + ": " + error.c_str() );
    }

    wp_vec3f vertexPosition( const wp_graphics_mesh *mesh, size_t index )
    {
        const auto *vertices = static_cast<const u8 *>( wp_graphics_mesh_get_vertices( mesh ) );
        const auto stride = wp_graphics_mesh_get_vertex_stride( mesh );
        wp_vec3f position{};
        require( vertices && stride >= sizeof( position ), "Imported mesh has readable vertices" );
        std::memcpy( &position, vertices + index * stride, sizeof( position ) );
        return position;
    }

    bool closePosition( const wp_vec3f &a, const wp_vec3f &b )
    {
        return closeValue( a.x, b.x ) && closeValue( a.y, b.y ) && closeValue( a.z, b.z );
    }

    void verifyCharacterPose( const std::shared_ptr<const RuntimeResource> &resource,
                              const std::shared_ptr<const RuntimeResource> &rig, const String &target )
    {
        String error;
        render::CookedSkinnedMeshData skin;
        requireResource( render::SkinnedMeshResourceCompiler::decode( *resource, skin, error ), error,
                         "Decode cooked imported skin" );
        require( skin.vertices.size() == 4 && skin.target == target && skin.packagedBuild,
                 "Imported skin retains the exact vertex count and package target/mode" );

        scene::Animator firstAnimator, secondAnimator;
        requireResource( firstAnimator.loadResource( rig, error ), error, "Load first imported rig" );
        requireResource( secondAnimator.loadResource( rig, error ), error, "Load second imported rig" );
        auto firstSkeleton = firstAnimator.getSkeleton();
        auto secondSkeleton = secondAnimator.getSkeleton();
        require( firstSkeleton && secondSkeleton && firstSkeleton != secondSkeleton,
                 "Cooked imported rig creates independent animation owners" );
        const auto root = firstSkeleton->getBone( "Root" );
        const auto child = firstSkeleton->getBone( "Child" );
        const auto clip = firstSkeleton->getAnimation( "Lift" );
        require( root && child && child->getParent() == root &&
                 closeValue( child->getPosition().Y(), 1 ) && clip && closeValue( clip->getLength(), 2 ),
                 "Cooked import retains joint names, parent, child bind translation and clip length" );

        auto firstMesh = make_ptr<render::ClawMesh>();
        auto secondMesh = make_ptr<render::ClawMesh>();
        requireResource( firstMesh->loadResource( resource, error, target, true ), error,
                         "Install first imported mesh and skin" );
        requireResource( secondMesh->loadResource( resource, error, target, true ), error,
                         "Install second imported mesh and skin" );
        const auto *bindMesh = secondMesh->getNativeMesh();
        require( bindMesh && firstMesh->getNativeMesh() != bindMesh &&
                 firstMesh->hasSkinningData() && secondMesh->hasSkinningData() &&
                 wp_graphics_mesh_get_vertex_count( bindMesh ) == 4 &&
                 wp_graphics_mesh_get_index_count( bindMesh ) == 6 &&
                 wp_graphics_mesh_get_submesh_count( bindMesh ) == 1 &&
                 wp_graphics_mesh_get_primitive_type( bindMesh ) == WORKPHONE_PRIMITIVE_TRIANGLE_LIST,
                 "Imported package installs two independent indexed quad meshes" );

        const std::array<wp_vec3f, 4> bind = {{ { 0, 0, 0 }, { 2, 0, 0 }, { 0, 2, 0 }, { 2, 2, 0 } }};
        const std::array<float, 4> rootWeight = { 1, 0, .25f, .6f };
        std::array<size_t, 4> fixtureVertex{};
        std::array<bool, 4> found{};
        for( size_t vertex = 0; vertex < bind.size(); ++vertex )
        {
            const auto position = vertexPosition( bindMesh, vertex );
            size_t index = 0;
            while( index < bind.size() && !closePosition( position, bind[index] ) ) ++index;
            require( index < bind.size() && !found[index], "Bind-space import preserves the source quad" );
            found[index] = true;
            fixtureVertex[vertex] = index;

            float rootSum = 0, childSum = 0;
            const auto &influence = skin.vertices[vertex];
            for( size_t slot = 0; slot < influence.weights.size(); ++slot )
            {
                const auto weight = influence.weights[slot];
                if( weight == 0 ) continue;
                const auto joint = influence.joints[slot];
                require( joint == root->getBoneHandle() || joint == child->getBoneHandle(),
                         "Every positive imported influence refers to a source skin joint" );
                if( joint == root->getBoneHandle() ) rootSum += weight;
                else childSum += weight;
            }
            require( closeValue( rootSum, rootWeight[index] ) && closeValue( childSum, 1 - rootWeight[index] ),
                     "Imported skin keeps each mixed weight attached to the corresponding source vertex" );
        }

        // At one second the root moved +1X and the child rotates +45 degrees about its bind joint.
        const auto s = std::sqrt( .5f );
        const std::array<wp_vec3f, 4> midpoint = {{
            { 1, 0, 0 }, { 1 + 3 * s, 1 + s, 0 },
            { 1 - .75f * s, 1.25f + .75f * s, 0 }, { 2.2f + .4f * s, 1.6f + 1.2f * s, 0 }
        }};
        const std::array<wp_vec3f, 4> endpoint = {{ { 2, 0, 0 }, { 3, 3, 0 }, { 1.25f, 1.25f, 0 }, { 2.8f, 2.4f, 0 } }};
        firstAnimator.setAnimationTime( 1 );
        secondAnimator.setAnimationTime( 2 );
        requireResource( firstMesh->applySkeletonPose( firstSkeleton, error ), error,
                         "Apply interpolated imported pose" );
        requireResource( secondMesh->applySkeletonPose( secondSkeleton, error ), error,
                         "Apply independent imported endpoint pose" );
        for( size_t vertex = 0; vertex < bind.size(); ++vertex )
        {
            const auto index = fixtureVertex[vertex];
            require( closePosition( vertexPosition( firstMesh->getNativeMesh(), vertex ), midpoint[index] ) &&
                     closePosition( vertexPosition( secondMesh->getNativeMesh(), vertex ), endpoint[index] ),
                     "Imported clip and inverse bind produce independent weighted native vertex positions" );
        }
        firstMesh->unload( nullptr );
        secondMesh->unload( nullptr );
    }

    void importedCharacterPackage()
    {
        CharacterWorkspace workspace;
        const auto source = workspace.folder / "source";
        const auto compiled = workspace.folder / "compiled";
        const auto package = workspace.folder / "package";
        fs::create_directory( source );
        const auto input = source / "character.gltf";
        fs::copy_file( fs::u8path( WP_TEST_SOURCE_ROOT "/Tests/cpp/Fixtures/Animation/analytic_skin.gltf" ), input );

        const ResourceID meshId( "data://character.mesh" ), rigId( "data://character.skelres" ),
                         skinId( "data://character.skinres" );
        const String meshUuid = "6f7f5fa3-88a8-479d-84c5-65a017baf011";
        const String rigUuid = "6f7f5fa3-88a8-479d-84c5-65a017baf012";
        const String skinUuid = "6f7f5fa3-88a8-479d-84c5-65a017baf013";
        const String target = "imported-skin-contract";
        String error;
        character_import::Sources imported;
        requireResource( character_import::importSources( pathText( input ), meshId, rigId, imported, error ),
                         error, "Import glTF geometry, skin and clip through WPAssimp" );
        require( !imported.mesh.empty() && !imported.skeleton.empty() && !imported.skin.empty(),
                 "Successful character import supplies all three cooker inputs" );
        writeSource( source / "character.mesh", imported.mesh.data(), imported.mesh.size() );
        writeSource( source / "character.skelres", imported.skeleton.data(), imported.skeleton.size() );
        writeSource( source / "character.skinres", imported.skin.data(), imported.skin.size() );

        // A changed bind transform without its matching inverse bind is unsupported by the rigid runtime.
        std::ifstream original( input, std::ios::binary );
        std::string mismatch( ( std::istreambuf_iterator<char>( original ) ), {} );
        original.close();
        const std::string oldBind = "\"translation\":[0,1,0]";
        const auto bindAt = mismatch.find( oldBind );
        require( bindAt != std::string::npos, "Analytic source contains the expected child bind translation" );
        mismatch.replace( bindAt, oldBind.size(), "\"translation\":[0,2,0]" );
        const auto badInput = source / "inconsistent-bind.gltf";
        writeSource( badInput, mismatch.data(), mismatch.size() );
        const auto previous = imported;
        const auto accepted = character_import::importSources( pathText( badInput ), meshId, rigId, imported, error );
        require( !accepted && !error.empty() && imported.mesh == previous.mesh &&
                 imported.skeleton == previous.skeleton && imported.skin == previous.skin,
                 "Inconsistent inverse bind fails without replacing the prior complete source set" );

        {
            auto registry = std::make_shared<ResourceCompilerRegistry>();
            requireResource( registry->registerCompiler( std::make_shared<render::MeshResourceCompiler>(), &error ),
                             error, "Register mesh compiler" );
            requireResource( registry->registerCompiler( std::make_shared<SkeletonResourceCompiler>(), &error ),
                             error, "Register skeleton and clip compiler" );
            requireResource( registry->registerCompiler( std::make_shared<render::SkinnedMeshResourceCompiler>(), &error ),
                             error, "Register skin compiler" );
            ResourceSystem authoring( registry, std::make_shared<ResourceCompilationDatabase>() );
            ResourceSystemConfig config;
            config.sourceRoot = pathText( source );
            config.compiledRoot = pathText( compiled );
            config.target = target;
            requireResource( authoring.initialize( config, error ), error, "Open imported character cooker" );
            CompilationOptions options;
            options.packagedBuild = true;
            const auto result = authoring.compile( skinId, options );
            if( !result.succeeded() )
                for( const auto &message : result.messages ) std::fprintf( stderr, "%s\n", message.c_str() );
            require( result.succeeded(), "Cook imported skin and its mesh/rig closure" );
            const Array<RuntimeResourceBinding> bindings = {{ skinUuid, skinId }, { meshUuid, meshId }, { rigUuid, rigId }};
            requireResource( authoring.writeRuntimePackage( pathText( package ), { skinId }, bindings, error ), error,
                             "Export imported character package" );
            authoring.shutdown();
        }
        fs::remove_all( source );
        fs::remove_all( compiled );
        size_t artifacts = 0;
        for( const auto &entry : fs::recursive_directory_iterator( package ) )
            if( entry.is_regular_file() )
            {
                require( entry.path().extension() == ".wprs" || entry.path() == package / "runtime.wprm",
                         "Character package contains only cooked resources and the runtime manifest" );
                ++artifacts;
            }
        require( artifacts == 4 && !fs::exists( source ) && !fs::exists( compiled ),
                 "Source-free character package has one manifest and three cooked containers" );

        ResourceSystem runtime( nullptr, nullptr );
        RuntimeResourceConfig config;
        config.compiledRoot = pathText( package );
        config.manifestPath = pathText( package / "runtime.wprm" );
        config.target = target;
        requireResource( runtime.initializeRuntime( config, error ), error, "Mount imported character package" );
        const auto skin = runtime.loadUUID( skinUuid, error );
        requireResource( bool( skin ), error, "Resolve cooked skin by UUID" );
        const auto mesh = runtime.loadUUID( meshUuid, error );
        requireResource( bool( mesh ), error, "Resolve cooked mesh by UUID" );
        const auto rig = runtime.loadUUID( rigUuid, error );
        requireResource( bool( rig ), error, "Resolve cooked skeleton and clip by UUID" );
        require( skin->header.resourceId == skinId && mesh->header.resourceId == meshId &&
                 rig->header.resourceId == rigId && skin->dependencies.size() == 2 &&
                 std::find( skin->dependencies.begin(), skin->dependencies.end(), mesh ) != skin->dependencies.end() &&
                 std::find( skin->dependencies.begin(), skin->dependencies.end(), rig ) != skin->dependencies.end(),
                 "Cooked skin links to the same pinned mesh and rig generations resolved by UUID" );
        verifyCharacterPose( skin, rig, target );
        runtime.shutdown();
        std::puts( "Original glTF import, typed package/UUID mount and independent native skin deformation: PASS" );
    }
#endif

    void samplingFailuresAndDefaults()
    {
        aiNodeAnim channel;
        channel.mNumRotationKeys = 2;
        channel.mRotationKeys = new aiQuatKey[2];
        channel.mRotationKeys[0] = aiQuatKey( 0, aiQuaternion( 1, 0, 0, 0 ) );
        channel.mRotationKeys[1] = aiQuatKey( 50, aiQuaternion( -1, 0, 0, 0 ) );
        aiMatrix4x4 bind;
        bind.a1 = bind.b2 = bind.c3 = 2;
        bind.a4 = 7; bind.b4 = 8; bind.c4 = 9;
        std::vector<animation_import::TransformSample> samples;
        std::string error;
        require( animation_import::sampleChannel( channel, bind, 50, 0, samples, error ),
                 "missing channel/reference pose and zero-rate compatibility fallback" );
        require( samples.size() == 2 && closeValue( samples[1].time, 2 ) &&
                 closeValue( samples[0].position.x, 7 ) && closeValue( samples[1].scale.z, 2 ),
                 "reference translation/scale and seconds conversion preserved" );
        const auto midpoint = animation_import::interpolate( channel.mRotationKeys[0].mValue,
                                                            channel.mRotationKeys[1].mValue, 0.5 );
        require( closeValue( std::abs( midpoint.w ), 1 ), "antipodal keys follow shortest rotation path" );
        const auto oldSize = samples.size();
        channel.mRotationKeys[1].mInterpolation = aiAnimInterpolation_Step;
        require( !animation_import::sampleChannel( channel, bind, 50, 25, samples, error ) &&
                 samples.size() == oldSize && !error.empty(), "unsupported STEP fails without partial output" );
        channel.mRotationKeys[1].mInterpolation = aiAnimInterpolation_Linear;
        channel.mRotationKeys[1].mValue.w = std::numeric_limits<float>::quiet_NaN();
        require( !animation_import::sampleChannel( channel, bind, 50, 25, samples, error ), "nonfinite quaternion fails" );
        channel.mRotationKeys[1].mValue = aiQuaternion( 1, 0, 0, 0 );
        channel.mRotationKeys[1].mTime = 0;
        require( !animation_import::sampleChannel( channel, bind, 50, 25, samples, error ), "duplicate time fails" );
        channel.mRotationKeys[1].mTime = 50;
        require( !animation_import::sampleChannel( channel, bind, 50, -1, samples, error ), "negative tick rate fails" );
        channel.mRotationKeys[1].mTime = 1e-8;
        require( !animation_import::sampleChannel( channel, bind, 1e-8, 1, samples, error ),
                 "distinct times inside legacy epsilon merge tolerance are rejected" );
        aiNodeAnim empty;
        require( animation_import::sampleChannel( empty, bind, 0, 25, samples, error ) && samples.size() == 1,
                 "zero-duration empty channel samples its bind pose once" );
        std::puts( "Animation sampling boundaries, validation and last-good output: PASS" );
    }
}

int main()
{
    TypeManager types;
    types.load();
    TypeManager::setInstance( &types );
    auto application = make_ptr<core::ApplicationManager>();
    core::IApplicationManager::setInstance( application );
    application->setFactoryManager( make_ptr<FactoryManager>() );
    // Actor transform dirtiness records frame time through the application
    // timer, including the first GameActor::setTransform in importedRoutes.
    auto timer = make_ptr<TimerMT>();
    timer->load( nullptr );
    application->setTimer( timer );
    int result = 0;
    try
    {
        samplingFailuresAndDefaults();
        importedRoutes();
#if WP_ANIMATION_IMPORTED_RESOURCES
        importedCharacterPackage();
#endif
    }
    catch( const std::exception &error ) { std::fprintf( stderr, "Animation import: FAIL: %s\n", error.what() ); result = 1; }
    application->setFactoryManager( nullptr );
    application->setTimer( nullptr );
    timer->unload( nullptr );
    timer = nullptr;
    core::IApplicationManager::setInstance( nullptr );
    application = nullptr;
    TypeManager::setInstance( nullptr );
    types.unload();
    return result;
}
