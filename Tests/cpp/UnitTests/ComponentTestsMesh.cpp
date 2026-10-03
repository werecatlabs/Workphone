#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Mesh/MeshSkeleton.hpp>
#include <Workphone/System/Resource.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::scene;

namespace
{
    class TestMeshResource : public Resource<IMeshResource>
    {
    public:
        f32 getScale() const override
        {
            return scale;
        }
        void setScale( f32 scale ) override
        {
            this->scale = scale;
        }
        MaterialNaming getMaterialNaming() const override
        {
            return materialNaming;
        }
        void setMaterialNaming( MaterialNaming materialNaming ) override
        {
            this->materialNaming = materialNaming;
        }
        bool getConstraints() const override
        {
            return constraints;
        }
        void setConstraints( bool constraints ) override
        {
            this->constraints = constraints;
        }
        bool getAnimation() const override
        {
            return animation;
        }
        void setAnimation( bool animation ) override
        {
            this->animation = animation;
        }
        bool getVisibility() const override
        {
            return visibility;
        }
        void setVisibility( bool visibility ) override
        {
            this->visibility = visibility;
        }
        bool getCameras() const override
        {
            return cameras;
        }
        void setCameras( bool cameras ) override
        {
            this->cameras = cameras;
        }
        bool getLights() const override
        {
            return lights;
        }
        void setLights( bool lights ) override
        {
            this->lights = lights;
        }
        bool getLightmapUVs() const override
        {
            return lightmapUVs;
        }
        void setLightmapUVs( bool lightmapUVs ) override
        {
            this->lightmapUVs = lightmapUVs;
        }

        bool getUseMeshInstancing() const override
        {
            return useMeshInstancing;
        }

        void setUseMeshInstancing( bool useMeshInstancing ) override
        {
            this->useMeshInstancing = useMeshInstancing;
        }

        SmartPtr<IMesh> getMesh() const override
        {
            return mesh;
        }
        void setMesh( SmartPtr<IMesh> mesh ) override
        {
            this->mesh = mesh;
        }

        f32 scale = 1.0f;
        MaterialNaming materialNaming = MaterialNaming::MaterialName;
        bool constraints = true;
        bool animation = true;
        bool visibility = true;
        bool cameras = true;
        bool lights = true;
        bool lightmapUVs = false;
        bool useMeshInstancing = false;
        SmartPtr<IMesh> mesh;
    };

    SmartPtr<TestMeshResource> makeMeshResource( const String &filePath )
    {
        auto resource = workphone::make_ptr<TestMeshResource>();
        if( resource )
        {
            resource->setFilePath( filePath );
        }

        return resource;
    }

    SmartPtr<MeshSkeleton> makeSkeleton()
    {
        return workphone::make_ptr<MeshSkeleton>();
    }
}  // namespace

BOOST_AUTO_TEST_CASE( components_mesh_default_state_and_static_property_names )
{
    try
    {
        TestGuard guard;

        auto mesh = workphone::make_ptr<scene::Mesh>();
        BOOST_REQUIRE( mesh );

        BOOST_CHECK( !mesh->isLoaded() );
        BOOST_CHECK( mesh->getMeshPath().empty() );
        BOOST_CHECK( !mesh->getMeshResource() );
        BOOST_CHECK( !mesh->getSkeleton() );
        BOOST_CHECK_EQUAL( mesh->getFsmPriority(), 15000 );

        BOOST_CHECK_EQUAL( scene::Mesh::m_meshPathStr, "Mesh Path" );
        BOOST_CHECK_EQUAL( scene::Mesh::m_meshStr, "Mesh Resource" );
        BOOST_CHECK_EQUAL( scene::Mesh::m_skeletonStr, "Skeleton" );
        BOOST_CHECK_EQUAL( scene::Mesh::m_fsmPriorityStr, "FSM Priority" );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

BOOST_AUTO_TEST_CASE( components_mesh_get_properties_contains_serialized_state )
{
    try
    {
        TestGuard guard;

        auto mesh = workphone::make_ptr<scene::Mesh>();
        auto meshResource = makeMeshResource( "UnitTests/MeshComponent/Serialized.mesh" );
        auto skeleton = makeSkeleton();
        BOOST_REQUIRE( mesh );
        BOOST_REQUIRE( meshResource );
        BOOST_REQUIRE( skeleton );

        mesh->setMeshResource( meshResource );
        mesh->setSkeleton( skeleton );
        mesh->setFsmPriority( 4321 );

        auto properties = mesh->getProperties();
        BOOST_REQUIRE( properties );

        String meshPath;
        s32 fsmPriority = 0;
        BOOST_CHECK( properties->hasProperty( scene::Mesh::m_meshPathStr ) );
        BOOST_CHECK( properties->hasProperty( scene::Mesh::m_meshStr ) );
        BOOST_CHECK( properties->hasProperty( scene::Mesh::m_skeletonStr ) );
        BOOST_CHECK( properties->hasProperty( scene::Mesh::m_fsmPriorityStr ) );
        BOOST_CHECK( properties->getPropertyValue( scene::Mesh::m_meshPathStr, meshPath ) );
        BOOST_CHECK( properties->getPropertyValue( scene::Mesh::m_fsmPriorityStr, fsmPriority ) );
        BOOST_CHECK_EQUAL( meshPath, "UnitTests/MeshComponent/Serialized.mesh" );
        BOOST_CHECK_EQUAL( fsmPriority, 4321 );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

BOOST_AUTO_TEST_CASE( components_mesh_setters_round_trip_resource_path_skeleton_and_priority )
{
    try
    {
        TestGuard guard;

        auto mesh = workphone::make_ptr<scene::Mesh>();
        auto meshResource = makeMeshResource( "UnitTests/MeshComponent/DirectResource.mesh" );
        auto skeleton = makeSkeleton();
        BOOST_REQUIRE( mesh );
        BOOST_REQUIRE( meshResource );
        BOOST_REQUIRE( skeleton );

        mesh->setMeshResource( meshResource );
        mesh->setSkeleton( skeleton );
        mesh->setFsmPriority( 100 );

        BOOST_CHECK( mesh->getMeshResource().get() == meshResource.get() );
        BOOST_CHECK_EQUAL( mesh->getMeshPath(), "UnitTests/MeshComponent/DirectResource.mesh" );
        BOOST_CHECK( mesh->getSkeleton().get() == skeleton.get() );
        BOOST_CHECK_EQUAL( mesh->getFsmPriority(), 100 );

        mesh->setMeshResource( nullptr );
        BOOST_CHECK( !mesh->getMeshResource() );
        BOOST_CHECK_EQUAL( mesh->getMeshPath(), "UnitTests/MeshComponent/DirectResource.mesh" );

        mesh->setSkeleton( nullptr );
        BOOST_CHECK( !mesh->getSkeleton() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

BOOST_AUTO_TEST_CASE( components_mesh_empty_path_clears_resource )
{
    try
    {
        TestGuard guard;

        auto mesh = workphone::make_ptr<scene::Mesh>();
        auto meshResource = makeMeshResource( "UnitTests/MeshComponent/ClearResource.mesh" );
        BOOST_REQUIRE( mesh );
        BOOST_REQUIRE( meshResource );

        mesh->setMeshResource( meshResource );
        BOOST_REQUIRE( mesh->getMeshResource() );

        mesh->setMeshPath( StringUtil::EmptyString );

        BOOST_CHECK( mesh->getMeshPath().empty() );
        BOOST_CHECK( !mesh->getMeshResource() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

BOOST_AUTO_TEST_CASE( components_mesh_set_properties_updates_path_and_priority )
{
    try
    {
        TestGuard guard;

        auto mesh = workphone::make_ptr<scene::Mesh>();
        auto properties = workphone::make_ptr<Properties>();
        BOOST_REQUIRE( mesh );
        BOOST_REQUIRE( properties );

        const String sourcePath = "UnitTests\\MeshComponent\\..\\MeshComponent\\FromProperties.mesh";
        const auto expectedPath = StringUtil::cleanupPath( sourcePath );
        properties->setProperty( scene::Mesh::m_meshPathStr, sourcePath );
        properties->setProperty( scene::Mesh::m_fsmPriorityStr, 25000 );

        mesh->setProperties( properties );

        BOOST_CHECK_EQUAL( mesh->getMeshPath(), expectedPath );
        BOOST_CHECK_EQUAL( mesh->getFsmPriority(), 25000 );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

BOOST_AUTO_TEST_CASE( components_mesh_load_properties_and_unload_clears_mesh_data )
{
    try
    {
        TestGuard guard;

        auto mesh = workphone::make_ptr<scene::Mesh>();
        auto properties = workphone::make_ptr<Properties>();
        auto skeleton = makeSkeleton();
        BOOST_REQUIRE( mesh );
        BOOST_REQUIRE( properties );
        BOOST_REQUIRE( skeleton );

        properties->setProperty( scene::Mesh::m_meshPathStr,
                                 "UnitTests/MeshComponent/LoadedFromProperties.mesh" );
        properties->setProperty( scene::Mesh::m_fsmPriorityStr, 77 );

        mesh->setSkeleton( skeleton );
        mesh->load( properties );

        BOOST_CHECK( mesh->isLoaded() );
        BOOST_CHECK_EQUAL( mesh->getMeshPath(), "UnitTests/MeshComponent/LoadedFromProperties.mesh" );
        BOOST_CHECK_EQUAL( mesh->getFsmPriority(), 77 );
        BOOST_CHECK( mesh->getSkeleton().get() == skeleton.get() );

        mesh->unload( nullptr );

        BOOST_CHECK( !mesh->isLoaded() );
        BOOST_CHECK( mesh->getMeshPath().empty() );
        BOOST_CHECK( !mesh->getMeshResource() );
        BOOST_CHECK( mesh->getSkeleton().get() == skeleton.get() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

BOOST_AUTO_TEST_CASE( components_mesh_actor_attachment_lifecycle )
{
    try
    {
        TestGuard guard;
        BOOST_REQUIRE( guard.sceneManager );

        auto actor = guard.sceneManager->createActor();
        BOOST_REQUIRE( actor );

        auto mesh = actor->addComponent<scene::Mesh>();
        BOOST_REQUIRE( mesh );
        BOOST_CHECK( actor->getComponent<scene::Mesh>().get() == mesh.get() );
        BOOST_CHECK( mesh->getActor().get() == actor.get() );

        mesh->setFsmPriority( 9000 );
        BOOST_CHECK_EQUAL( mesh->getFsmPriority(), 9000 );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}
