#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Jobs/CreateRigidBodies.hpp>
#include <Workphone/Scene/Components/CollisionMesh.hpp>
#include <Workphone/Scene/Components/Mesh.hpp>
#include <Workphone/Scene/Components/Rigidbody.hpp>
#include <Workphone/Interface/Mesh/IMeshResource.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, CreateRigidBodies, Job );

    CreateRigidBodies::CreateRigidBodies() = default;

    CreateRigidBodies::~CreateRigidBodies() = default;

    void CreateRigidBodies::execute()
    {
        auto actor = getActor();
        auto cascade = getCascade();

        createRigidBodies( actor, cascade );
    }

    auto CreateRigidBodies::getActor() const -> SmartPtr<scene::IGameActor>
    {
        return m_actor;
    }

    void CreateRigidBodies::setActor( SmartPtr<scene::IGameActor> actor )
    {
        m_actor = actor;
    }

    auto CreateRigidBodies::getCascade() const -> bool
    {
        return m_cascade;
    }

    void CreateRigidBodies::setCascade( bool cascade )
    {
        m_cascade = cascade;
    }

    auto CreateRigidBodies::getMakeStatic() const -> bool
    {
        return m_makeStatic;
    }

    void CreateRigidBodies::setMakeStatic( bool makeStatic )
    {
        m_makeStatic = makeStatic;
    }

    auto CreateRigidBodies::isConvex() const -> bool
    {
        return m_isConvex;
    }

    void CreateRigidBodies::setConvex( bool convex )
    {
        m_isConvex = convex;
    }

    void CreateRigidBodies::createRigidBodies( SmartPtr<scene::IGameActor> actor, bool cascade )
    {
        auto makeStatic = getMakeStatic();
        actor->setStatic( makeStatic );

        auto rigidBody = actor->getComponent<scene::Rigidbody>();
        if( !rigidBody )
        {
            rigidBody = actor->addComponent<scene::Rigidbody>();
        }

        auto meshCollision = actor->getComponent<scene::CollisionMesh>();
        if( !meshCollision )
        {
            meshCollision = actor->addComponent<scene::CollisionMesh>();
        }

        auto convex = isConvex();
        if( !makeStatic )
        {
            convex = true;
        }

        meshCollision->setConvex( convex );

        if( meshCollision )
        {
            auto mesh = actor->getComponent<scene::Mesh>();
            if( mesh )
            {
                meshCollision->setMeshResource( mesh->getMeshResource() );
            }
        }

        if( cascade )
        {
            auto children = actor->getChildren();
            for( auto child : children )
            {
                createRigidBodies( child, cascade );
            }
        }
    }

}  // namespace workphone
