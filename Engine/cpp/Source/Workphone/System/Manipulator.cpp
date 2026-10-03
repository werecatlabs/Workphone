#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/Manipulator.hpp>
#include <Workphone/Scene/Components/Camera/CameraController.hpp>
#include <Workphone/Scene/Components/Camera.hpp>
#include <Workphone/Interface/Graphics/IDebug.hpp>
#include <Workphone/Interface/Graphics/IDebugLine.hpp>
#include <Workphone/Interface/Graphics/IGraphicsMesh.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindow.hpp>
#include <Workphone/Interface/Input/IInputEvent.hpp>
#include <Workphone/Interface/Input/IMouseState.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/ICameraManager.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Interface/System/ISelectionManager.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Interface/UI/IUIManager.hpp>
#include <Workphone/Interface/UI/IUIWindow.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Math/MathUtil.hpp>
#include <Workphone/ApplicationUtil.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, Manipulator, ISharedObject );
    WP_CLASS_REGISTER_DERIVED( workphone, Manipulator::SelectionManagerListener, IEventListener );

    Manipulator::Manipulator() = default;

    Manipulator::~Manipulator() = default;

    void Manipulator::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto &loadingState = getLoadingState();
            if( loadingState != LoadingState::Loading && loadingState != LoadingState::Loaded )
            {
                setLoadingState( LoadingState::Loading );

                auto applicationManager = core::IApplicationManager::instance();
                auto selectionManager = applicationManager->getSelectionManager();

                if( selectionManager )
                {
                    m_selectionManagerListener = workphone::make_ptr<SelectionManagerListener>();
                    m_selectionManagerListener->setOwner( this );
                    selectionManager->addObjectListener( m_selectionManagerListener );
                }

                setLoadingState( LoadingState::Loaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Manipulator::unload( SmartPtr<ISharedObject> data )
    {
    }

    void Manipulator::update()
    {
    }

    auto Manipulator::handleEvent( const SmartPtr<IInputEvent> &event ) -> bool
    {
        return false;
    }

    void Manipulator::setEnabled( bool bEnabled )
    {
        if( m_enabled != bEnabled )
        {
            m_enabled = bEnabled;
            updateManipulatorPosition();
        }
    }

    auto Manipulator::isEnabled() -> bool
    {
        return m_enabled;
    }

    auto Manipulator::isVisible() const -> bool
    {
        return m_visible;
    }

    void Manipulator::setVisible( bool visible )
    {
        m_visible = visible;
    }

    auto Manipulator::getPosition() const -> Vector3<real_Num>
    {
        return m_relativeTranslation;
    }

    void Manipulator::setPosition( const Vector3<real_Num> &position )
    {
        m_relativeTranslation = position;
    }

    auto Manipulator::getRotation() const -> Vector3<real_Num>
    {
        return m_relativeRotation;
    }

    void Manipulator::setRotation( const Vector3<real_Num> &rotation )
    {
        m_relativeRotation = rotation;
    }

    auto Manipulator::getScale() const -> Vector3<real_Num>
    {
        return m_relativeScale;
    }

    void Manipulator::setScale( const Vector3<real_Num> &scale )
    {
        m_relativeScale = scale;
    }

    void Manipulator::updateManipulatorPosition()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto selectionManager = applicationManager->getSelectionManager();

        auto selection = selectionManager->getSelection();

        if( !selection.empty() )
        {
            auto vAveragePosition = Vector3<real_Num>::zero();

            for( auto selected : selection )
            {
                if( selected )
                {
                    if( selected->isDerived<scene::IGameActor>() )
                    {
                        auto actor = workphone::dynamic_pointer_cast<scene::IGameActor>( selected );
                        if( actor )
                        {
                            auto worldTransform = actor->getTransform();
                            if( worldTransform )
                            {
                                vAveragePosition += worldTransform->getPosition();
                            }
                        }
                    }
                }
            }

            vAveragePosition /= static_cast<f32>( selection.size() );
            m_vPosition = vAveragePosition;

            setPosition( vAveragePosition );
            setVisible( true );
        }
        else
        {
            setVisible( false );
        }
    }

    Manipulator::SelectionManagerListener::SelectionManagerListener() = default;

    Manipulator::SelectionManagerListener::~SelectionManagerListener()
    {
        m_owner = nullptr;
    }

    auto Manipulator::SelectionManagerListener::handleEvent( EventType eventType, hash_type eventValue,
                                                             const Array<Parameter> &arguments,
                                                             SmartPtr<ISharedObject> sender,
                                                             SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
        -> Parameter
    {
        if( eventValue == IEvent::addSelectedObject )
        {
            addSelectedObject();
        }
        if( eventValue == IEvent::addSelectedObjects )
        {
            addSelectedObjects();
        }
        if( eventValue == IEvent::deselectObjects )
        {
            deselectObjects();
        }

        return {};
    }

    void Manipulator::SelectionManagerListener::addSelectedObject()
    {
        if( auto owner = getOwner() )
        {
            owner->updateManipulatorPosition();
        }
    }

    void Manipulator::SelectionManagerListener::addSelectedObjects()
    {
        if( auto owner = getOwner() )
        {
            owner->updateManipulatorPosition();
        }
    }

    void Manipulator::SelectionManagerListener::setSelectedObjects()
    {
        if( auto owner = getOwner() )
        {
            owner->updateManipulatorPosition();
        }
    }

    void Manipulator::SelectionManagerListener::deselectObjects()
    {
        if( auto owner = getOwner() )
        {
            owner->updateManipulatorPosition();
        }
    }

    void Manipulator::SelectionManagerListener::deselectAll()
    {
        if( auto owner = getOwner() )
        {
            owner->updateManipulatorPosition();
        }
    }

    auto Manipulator::SelectionManagerListener::getOwner() const -> SmartPtr<Manipulator>
    {
        return m_owner;
    }

    void Manipulator::SelectionManagerListener::setOwner( SmartPtr<Manipulator> owner )
    {
        m_owner = owner;
    }

}  // namespace workphone
