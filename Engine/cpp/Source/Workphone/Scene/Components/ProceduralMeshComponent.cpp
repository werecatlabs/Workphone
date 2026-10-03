#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/ProceduralMeshComponent.hpp>
#include <Workphone/Scene/Components/Mesh.hpp>
#include <Workphone/Scene/Components/MeshRenderer.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/IResourceManager.hpp>
#include <Workphone/Interface/Mesh/IMesh.hpp>
#include <Workphone/Mesh/MeshManager.hpp>
#include <Workphone/Mesh/MeshResource.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <stdexcept>
namespace workphone::scene
{
    ProceduralMeshComponent::ProceduralMeshComponent() = default;
    ProceduralMeshComponent::~ProceduralMeshComponent() = default;

    WP_CLASS_REGISTER_DERIVED( workphone::scene, ProceduralMeshComponent, Component );
    void ProceduralMeshComponent::load( SmartPtr<ISharedObject> data )
    {
        Component::load( data );
        m_dirty = true;
        // Defer until all serialized sibling components have loaded.
        setLoadingState( LoadingState::Loaded );
    }
    void ProceduralMeshComponent::setActor( SmartPtr<IGameActor> actor )
    {
        if( actor != getActor() && m_meshComponent )
            unload( nullptr );
        Component::setActor( actor );
        m_dirty = true;
    }
    void ProceduralMeshComponent::update()
    {
        if( Thread::getCurrentTask() == TaskId::Application && m_dirty && getActor() && isEnabled() )
            regenerate();
    }
    bool ProceduralMeshComponent::regenerate()
    {
        m_dirty = false;
        if( !getActor() )
        {
            m_dirty = true;
            return false;
        }
        try
        {
            auto generated = buildMesh();
            if( !generated )
                throw std::runtime_error( "Generator returned no mesh" );
            auto app = core::IApplicationManager::instance();
            auto manager = workphone::dynamic_pointer_cast<MeshManager>( app->getMeshManager() );
            if( !manager )
                throw std::runtime_error( "Mesh manager is unavailable" );
            generated->updateAABB( true );
            auto resource = workphone::make_ptr<MeshResource>();
            // A fresh name forces both render backends to reload after regeneration.
            resource->setFilePath( "__procedural/" + StringUtil::getUUID() + ".meshbin" );
            resource->setName( resource->getFilePath() );
            resource->setMesh( generated );
            resource->setLoadingState( LoadingState::Loaded );
            manager->addMeshResource( resource );
            auto actor = getActor();
            if( !m_meshComponent )
            {
                m_meshComponent = actor->getComponent<Mesh>();
                m_ownsMesh = !m_meshComponent;
                if( m_ownsMesh )
                    m_meshComponent = actor->addComponent<Mesh>();
                else
                {
                    m_previousResource = m_meshComponent->getMeshResource();
                    m_previousPath = m_meshComponent->getMeshPath();
                }
            }
            if( !m_renderer )
            {
                m_renderer = actor->getComponent<MeshRenderer>();
                m_ownsRenderer = !m_renderer;
                if( m_ownsRenderer )
                    m_renderer = actor->addComponent<MeshRenderer>();
            }
            m_meshComponent->setMeshResource( resource );
            m_renderer->setState( getState() );
            if( m_resource )
                manager->removeMeshResource( m_resource );
            m_resource = resource;
            applyGeneratedMaterials();
            m_renderer->updateMaterials();
            m_error.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            m_error = e.what();
            return false;
        }
    }
    void ProceduralMeshComponent::unload( SmartPtr<ISharedObject> data )
    {
        if( auto actor = getActor() )
        {
            if( m_meshComponent && m_meshComponent->getMeshResource() == m_resource )
            {
                if( m_ownsRenderer && m_renderer )
                {
                    m_renderer->unload( nullptr );
                    actor->removeComponentInstance( m_renderer );
                }
                if( m_ownsMesh )
                {
                    m_meshComponent->unload( nullptr );
                    actor->removeComponentInstance( m_meshComponent );
                }
                else if( m_previousResource )
                    m_meshComponent->setMeshResource( m_previousResource );
                else
                    m_meshComponent->setMeshPath( m_previousPath );
            }
        }
        if( auto app = core::IApplicationManager::instancePtr() )
            if( auto manager = workphone::dynamic_pointer_cast<MeshManager>( app->getMeshManager() ) )
                if( m_resource )
                    manager->removeMeshResource( m_resource );
        m_resource = nullptr;
        m_previousResource = nullptr;
        m_previousPath.clear();
        m_meshComponent = nullptr;
        m_renderer = nullptr;
        m_ownsMesh = false;
        m_ownsRenderer = false;
        m_dirty = true;
        Component::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }
    SmartPtr<Properties> ProceduralMeshComponent::getProperties() const
    {
        auto p = Component::getProperties();
        p->setButtonPressed( "Regenerate" );
        p->setProperty( "Generation Error", m_error );
        p->getPropertyObject( "Generation Error" ).setReadOnly( true );
        return p;
    }
    void ProceduralMeshComponent::setProperties( SmartPtr<Properties> p )
    {
        if( !p )
            return;
        Component::setProperties( p );
        requestRegeneration();
    }
}  // namespace workphone::scene
