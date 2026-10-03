#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Mesh/Bone.hpp>
#include <Workphone/Core/Exception.hpp>
#include <Workphone/Mesh/SubMesh.hpp>
#include <Workphone/Mesh/Mesh.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Animation/IAnimation.hpp>
#include <Workphone/Interface/Mesh/IGraphicsBone.hpp>
#include <Workphone/Interface/System/ILogManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Mesh/LinkedSkeletonAnimationSource.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, Bone, IBone );

    Bone::Bone() :
        m_iHandle( 0 ),
        m_manuallyControlled( false ),
        m_initialPosition( Vector3<real_Num>::zero() ),
        m_initialOrientation( Quaternion<real_Num>::identity() ),
        m_position( Vector3<real_Num>::zero() ),
        m_orientation( Quaternion<real_Num>::identity() ),
        m_bindingPosition( Vector3<real_Num>::zero() ),
        m_bindingOrientation( Quaternion<real_Num>::identity() )
    {
        try
        {
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    logManager->logMessage( "Bone: Constructor called", ILogManager::Type::Info );
                }
            }
        }
        catch( const std::exception &e )
        {
            // Fallback logging in case the log manager is not available
            std::cerr << "Bone constructor error: " << e.what() << std::endl;
        }
    }

    Bone::~Bone()
    {
        try
        {
            // Clean up children
            m_children.clear();
            //m_parent.reset();

            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    logManager->logMessage( "Bone: Destructor called", ILogManager::Type::Info );
                }
            }
        }
        catch( const std::exception &e )
        {
            // Fallback logging in case the log manager is not available
            std::cerr << "Bone destructor error: " << e.what() << std::endl;
        }
    }

    SmartPtr<IBone> Bone::createChild(
        u16 handle, const Vector3<real_Num> &translate /*= Vector3<real_Num>::zero()*/,
        const Quaternion<real_Num> &rotate /*= Quaternion<real_Num>::identity() */ )
    {
        try
        {
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "Bone: Creating child bone with handle " ) +
                               StringUtil::toString( handle );
                    logManager->logMessage( msg, ILogManager::Type::Info );
                }
            }

            auto factoryManager = core::IApplicationManager::instance()->getFactoryManager();
            if( !factoryManager )
            {
                if( auto applicationManager = core::IApplicationManager::instance() )
                {
                    if( auto logManager = applicationManager->getLogManager() )
                    {
                        logManager->logMessage( "Bone: Factory manager not available",
                                                ILogManager::Type::Error );
                    }
                }
                throw std::runtime_error( "Factory manager not available" );
            }

            auto child = factoryManager->make_ptr<Bone>();
            if( !child )
            {
                if( auto applicationManager = core::IApplicationManager::instance() )
                {
                    if( auto logManager = applicationManager->getLogManager() )
                    {
                        logManager->logMessage( "Bone: Failed to create child bone",
                                                ILogManager::Type::Error );
                    }
                }
                throw std::runtime_error( "Failed to create child bone" );
            }

            child->m_iHandle = handle;
            child->m_position = translate;
            child->m_orientation = rotate;
            child->m_initialPosition = translate;
            child->m_initialOrientation = rotate;
            child->m_parent = SmartPtr<Bone>( this );

            m_children.push_back( child );

            return child;
        }
        catch( const std::exception &e )
        {
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "Bone: Exception in createChild: " ) + e.what();
                    logManager->logMessage( msg, ILogManager::Type::Exception );
                }
            }
            throw;
        }
    }

    void Bone::setBindingPose()
    {
        try
        {
            m_bindingPosition = m_position;
            m_bindingOrientation = m_orientation;

            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    logManager->logMessage( "Bone: Binding pose set", ILogManager::Type::Info );
                }
            }

            // Set binding pose for all children recursively
            for( auto &child : m_children )
            {
                if( child )
                {
                    child->setBindingPose();
                }
            }
        }
        catch( const std::exception &e )
        {
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "Bone: Exception in setBindingPose: " ) + e.what();
                    logManager->logMessage( msg, ILogManager::Type::Exception );
                }
            }
            throw;
        }
    }

    void Bone::reset()
    {
        try
        {
            m_position = m_bindingPosition;
            m_orientation = m_bindingOrientation;

            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    logManager->logMessage( "Bone: Reset to binding pose", ILogManager::Type::Info );
                }
            }

            // Reset all children recursively
            for( auto &child : m_children )
            {
                if( child )
                {
                    child->reset();
                }
            }
        }
        catch( const std::exception &e )
        {
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "Bone: Exception in reset: " ) + e.what();
                    logManager->logMessage( msg, ILogManager::Type::Exception );
                }
            }
            throw;
        }
    }

    void Bone::setManuallyControlled( bool manuallyControlled )
    {
        try
        {
            m_manuallyControlled = manuallyControlled;

            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "Bone: Manual control set to " ) +
                               ( manuallyControlled ? "true" : "false" );
                    logManager->logMessage( msg, ILogManager::Type::Info );
                }
            }
        }
        catch( const std::exception &e )
        {
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "Bone: Exception in setManuallyControlled: " ) + e.what();
                    logManager->logMessage( msg, ILogManager::Type::Exception );
                }
            }
            throw;
        }
    }

    bool Bone::isManuallyControlled() const
    {
        try
        {
            return m_manuallyControlled;
        }
        catch( const std::exception &e )
        {
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "Bone: Exception in isManuallyControlled: " ) + e.what();
                    logManager->logMessage( msg, ILogManager::Type::Exception );
                }
            }
            throw;
        }
    }

    Vector3<real_Num> Bone::getPosition() const
    {
        return m_position;
    }

    void Bone::setPosition( const Vector3<real_Num> &position )
    {
        try
        {
            m_position = position;

            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "Bone: Position updated" );
                    logManager->logMessage( msg, ILogManager::Type::Info );
                }
            }
        }
        catch( const std::exception &e )
        {
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "Bone: Exception in setPosition: " ) + e.what();
                    logManager->logMessage( msg, ILogManager::Type::Exception );
                }
            }
            throw;
        }
    }

    Quaternion<real_Num> Bone::getOrientation() const
    {
        return m_orientation;
    }

    void Bone::setOrientation( const Quaternion<real_Num> &orientation )
    {
        try
        {
            m_orientation = orientation;

            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "Bone: Orientation updated" );
                    logManager->logMessage( msg, ILogManager::Type::Info );
                }
            }
        }
        catch( const std::exception &e )
        {
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "Bone: Exception in setOrientation: " ) + e.what();
                    logManager->logMessage( msg, ILogManager::Type::Exception );
                }
            }
            throw;
        }
    }

    u16 Bone::getBoneHandle() const
    {
        return m_iHandle;
    }

    void Bone::setBoneHandle( u16 handle )
    {
        m_iHandle = handle;
    }

    SmartPtr<IBone> Bone::getParent() const
    {
        return m_parent.lock();
    }

    Array<SmartPtr<IBone>> Bone::getChildren() const
    {
        try
        {
            Array<SmartPtr<IBone>> children;
            children.reserve( m_children.size() );

            for( const auto &child : m_children )
            {
                children.push_back( child );
            }

            return children;
        }
        catch( const std::exception &e )
        {
            if( auto applicationManager = core::IApplicationManager::instance() )
            {
                if( auto logManager = applicationManager->getLogManager() )
                {
                    auto msg = String( "Bone: Exception in getChildren: " ) + e.what();
                    logManager->logMessage( msg, ILogManager::Type::Exception );
                }
            }
            throw;
        }
    }

}  // namespace workphone
