#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/SubComponent.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, SubComponent, Resource<ISubComponent> );

    u32 SubComponent::m_idExt = 0;

    SubComponent::SubComponent() = default;

    SubComponent::~SubComponent() = default;

    auto SubComponent::getParentComponent() const -> SmartPtr<IComponent>
    {
        return m_parentComponent;
    }

    void SubComponent::setParentComponent( SmartPtr<IComponent> parentComponent )
    {
        if( m_parentComponent == parentComponent )
        {
            return;
        }
        m_parentComponent = parentComponent;
    }

    auto SubComponent::getParent() const -> SmartPtr<ISubComponent>
    {
        return m_parent;
    }

    auto SubComponent::toData() const -> SmartPtr<ISharedObject>
    {
        auto data = Resource<ISubComponent>::toData();
        return data;
    }

    void SubComponent::fromData( SmartPtr<ISharedObject> data )
    {
        Resource<ISubComponent>::fromData( data );
    }

    auto SubComponent::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = workphone::make_ptr<Properties>();

        // Export Parent Component
        if( m_parentComponent )
        {
            properties->setProperty( "ParentComponent", m_parentComponent->toData()->getName(),
                                     Properties::componentStr );
        }
        else
        {
            properties->setProperty( "ParentComponent", Properties::noneStr, Properties::componentStr );
        }

        // Export Parent SubComponent
        if( m_parent )
        {
            properties->setProperty( "ParentSubComponent", m_parent->toData()->getName(),
                                     Properties::componentStr );
        }
        else
        {
            properties->setProperty( "ParentSubComponent", Properties::noneStr,
                                     Properties::componentStr );
        }

        // Export Children count as a simple property
        properties->setProperty( "ChildrenCount", std::to_string( m_children.size() ),
                                 Properties::intTypeStr );

        return properties;
    }

    void SubComponent::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            return;
        }

        // Import Parent SubComponent
        if( properties->hasProperty( "ParentSubComponent" ) )
        {
            String parentName = properties->getProperty( "ParentSubComponent" );
            if( parentName == Properties::noneStr )
            {
                setParent( nullptr );
            }
            else
            {
                // In a production environment, we would look up the object by name or UUID via the ResourceDatabase
                // Since we don't have the database access here, we log a warning and a conceptual implementation
                auto applicationManager = core::IApplicationManager::instance();
                if( applicationManager && applicationManager->getLogManager() )
                {
                    applicationManager->getLogManager()->logMessage(
                        "Setting parent for " + this->toData()->getName() + " to " + parentName +
                            " (Resource lookup required)",
                        workphone::ILogManager::Type::Info );
                }
            }
        }
    }

    void SubComponent::setParent( SmartPtr<ISubComponent> parent )
    {
        if( parent.get() == this )
        {
            return;
        }

        if( m_parent == parent )
        {
            return;
        }

        // Remove from old parent's children if exists
        if( m_parent )
        {
            m_parent->removeChild( this );
        }

        m_parent = parent;

        // Add to new parent's children if exists
        if( m_parent )
        {
            m_parent->addChild( this );
        }
    }

    void SubComponent::addChildByType( u32 componentType )
    {
        auto applicationManager = core::IApplicationManager::instance();
        if( !applicationManager )
            return;

        auto factoryManager = applicationManager->getFactoryManager();
        if( !factoryManager )
        {
            applicationManager->getLogManager()->logMessage(
                "FactoryManager is null during addChildByType", workphone::ILogManager::Type::Error );
            return;
        }

        auto factory = factoryManager->getFactoryById( componentType );
        if( factory )
        {
            auto child = factory->make_object<ISubComponent>();
            if( child )
            {
                addChild( child );
            }
            else
            {
                applicationManager->getLogManager()->logMessage(
                    "Factory failed to create ISubComponent for type " + std::to_string( componentType ),
                    workphone::ILogManager::Type::Error );
            }
        }
        else
        {
            applicationManager->getLogManager()->logMessage(
                "No factory found for component type " + std::to_string( componentType ),
                workphone::ILogManager::Type::Warning );
        }
    }

    void SubComponent::addChild( SmartPtr<ISubComponent> child )
    {
        if( !child )
            return;
        if( child.get() == this )
            return;
        if( std::find( m_children.begin(), m_children.end(), child ) != m_children.end() )
            return;

        child->setParent( this );
        m_children.push_back( child );
    }

    void SubComponent::removeChild( SmartPtr<ISubComponent> child )
    {
        if( !child )
            return;
        auto it = std::find( m_children.begin(), m_children.end(), child );
        if( it != m_children.end() )
        {
            child->setParent( nullptr );
            m_children.erase( it );
        }
    }

    auto SubComponent::getChildren() const -> Array<SmartPtr<ISubComponent>>
    {
        return m_children;
    }
}  // namespace workphone::scene
