#include <WPAssimp/AssimpLoader.hpp>
#include <WPAssimp/AnimationImport.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/FactoryManager.hpp>
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
        const auto source = importer.ReadFile( WP_TEST_SOURCE_ROOT "/Tests/Fixtures/Animation/analytic_channels.gltf", 0 );
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
    int result = 0;
    try { samplingFailuresAndDefaults(); importedRoutes(); }
    catch( const std::exception &error ) { std::fprintf( stderr, "Animation import: FAIL: %s\n", error.what() ); result = 1; }
    application->setFactoryManager( nullptr );
    core::IApplicationManager::setInstance( nullptr );
    application = nullptr;
    TypeManager::setInstance( nullptr );
    types.unload();
    return result;
}
