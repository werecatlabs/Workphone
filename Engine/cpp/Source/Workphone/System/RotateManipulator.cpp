#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/RotateManipulator.hpp>
#include <Workphone/Scene/Components/Camera/CameraController.hpp>
#include <Workphone/Scene/Components/Camera.hpp>
#include <Workphone/Interface/Graphics/IDebug.hpp>
#include <Workphone/Interface/Graphics/IDebugLine.hpp>
#include <Workphone/Interface/Graphics/IDebugCircle.hpp>
#include <Workphone/Interface/Graphics/IGraphicsMesh.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindow.hpp>
#include <Workphone/Interface/Input/IInputDeviceManager.hpp>
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
#include <limits>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, RotateManipulator, Manipulator );

    RotateManipulator::RotateManipulator( bool drawDebugLines ) : m_drawDebugLines( drawDebugLines )
    {
    }

    RotateManipulator::~RotateManipulator() = default;

    void RotateManipulator::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto &loadingState = getLoadingState();
            if( loadingState != LoadingState::Loading && loadingState != LoadingState::Loaded )
            {
                m_circles.resize( 3 );
                m_orientations.resize( 3 );
                m_selected.resize( 3 );

                // Debug circles are created in the XZ plane, with their normal
                // along Y. Rotate them to produce X, Y and Z rotation rings.
                m_orientations[AT_X] = Quaternion<real_Num>::eulerDegrees( 0.0, 0.0, 90.0 );
                m_orientations[AT_Y] = Quaternion<real_Num>::identity();
                m_orientations[AT_Z] = Quaternion<real_Num>::eulerDegrees( 90.0, 0.0, 0.0 );

                for( size_t i = 0; i < m_selected.size(); ++i )
                {
                    m_selected[i] = false;
                }

                Manipulator::load( data );

                if( auto applicationManager = core::IApplicationManager::instancePtr() )
                {
                    if( applicationManager->getSelectionManager() )
                    {
                        updateManipulatorPosition();
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void RotateManipulator::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto &loadingState = getLoadingState();
            if( loadingState == LoadingState::Loaded )
            {
                setLoadingState( LoadingState::Unloading );
                setDebugCirclesVisible( false );

                if( auto applicationManager = core::IApplicationManager::instancePtr() )
                {
                    if( auto selectionManager = applicationManager->getSelectionManager() )
                    {
                        if( m_selectionManagerListener )
                        {
                            selectionManager->removeObjectListener( m_selectionManagerListener );
                        }
                    }
                }

                m_selectionManagerListener = nullptr;
                m_circles.clear();
                m_orientations.clear();
                m_selected.clear();
                m_mouseButtonDown = false;

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void RotateManipulator::update()
    {
        if( !isLoaded() || !m_drawDebugLines || !isEnabled() )
        {
            setDebugCirclesVisible( false );
            return;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager )
        {
            setDebugCirclesVisible( false );
            return;
        }

        auto selectionManager = applicationManager->getSelectionManager();
        if( !selectionManager || selectionManager->getSelection().empty() )
        {
            setVisible( false );
            setDebugCirclesVisible( false );
            return;
        }

        updateManipulatorPosition();
        if( !isVisible() )
        {
            setDebugCirclesVisible( false );
            return;
        }

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        auto cameraManager = applicationManager->getCameraManager();
        if( !graphicsSystem || !cameraManager )
        {
            setDebugCirclesVisible( false );
            return;
        }

        auto debug = graphicsSystem->getDebug();
        auto cameraActor = cameraManager->getEditorCamera();
        if( !debug || !cameraActor )
        {
            setDebugCirclesVisible( false );
            return;
        }

        if( auto cameraTransform = cameraActor->getTransform() )
        {
            auto cameraDistance = ( getPosition() - cameraTransform->getPosition() ).length();
            if( cameraDistance < real_Num( 1.0 ) )
            {
                cameraDistance = real_Num( 1.0 );
            }

            m_fScale = cameraDistance * m_cameraDistanceScale;
        }

        const u32 selectedColour = 0xFFFF00FFu;
        const u32 colours[] = { m_selected[AT_X] ? selectedColour : 0xFF0000FFu,
                                m_selected[AT_Y] ? selectedColour : 0x00FF00FFu,
                                m_selected[AT_Z] ? selectedColour : 0x0000FFFFu };

        const auto position = getPosition();
        static const auto hash = StringUtil::getHash( "RotateManipulator" );

        for( size_t i = 0; i < m_circles.size(); ++i )
        {
            m_circles[i] = debug->drawCircle( hash + static_cast<hash_type>( i ), position,
                                              m_orientations[i], m_fScale, colours[i] );
        }

        setDebugCirclesVisible( true );
    }

    auto RotateManipulator::handleEvent( const SmartPtr<IInputEvent> &event ) -> bool
    {
        if( !isLoaded() || !isEnabled() || !m_drawDebugLines || !isVisible() || !event )
        {
            return false;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager )
        {
            return false;
        }

        auto cameraManager = applicationManager->getCameraManager();
        auto selectionManager = applicationManager->getSelectionManager();
        if( !cameraManager || !selectionManager )
        {
            return false;
        }

        auto mouseState = event->getMouseState();
        if( !mouseState )
        {
            return false;
        }

        auto cameraActor = cameraManager->getEditorCamera();
        if( !cameraActor )
        {
            return false;
        }

        auto camera = cameraActor->getComponent<scene::Camera>();
        if( !camera )
        {
            return false;
        }

        const auto mousePosition = mouseState->getAbsolutePosition();
        const auto relativeMousePosition = mouseState->getRelativePosition();
        const auto mouseStateRelative = ApplicationUtil::getRelativeMousePos( relativeMousePosition );
        const auto mouseStateEvent = mouseState->getEventType();

        if( !m_mouseButtonDown )
        {
            for( size_t i = 0; i < m_selected.size(); ++i )
            {
                m_selected[i] = false;
            }

            const auto position = getPosition();
            const auto controlWidth = m_controlWidth * m_fScale;
            const auto ray = camera->getCameraToViewportRay( mouseStateRelative );

            auto closestDistance = std::numeric_limits<real_Num>::max();
            auto selectedAxis = AT_LAST;

            for( size_t i = 0; i < m_orientations.size(); ++i )
            {
                const auto normal = m_orientations[i] * Vector3<real_Num>::unitY();
                const auto cylinderAxis = Line3<real_Num>( position, position + normal );
                const auto cylinder = Cylinder3<real_Num>( cylinderAxis, m_fScale, controlWidth );
                const auto result = MathUtil<real_Num>::intersects( ray, cylinder );
                if( result.first && result.second >= real_Num( 0.0 ) )
                {
                    const auto intersectionPoint = ray.getOrigin() + ray.getDirection() * result.second;
                    const auto radius = ( intersectionPoint - position ).length();
                    if( radius > m_fScale * ( real_Num( 1.0 ) - m_tollerance ) &&
                        radius < m_fScale * ( real_Num( 1.0 ) + m_tollerance ) &&
                        result.second < closestDistance )
                    {
                        closestDistance = result.second;
                        selectedAxis = static_cast<AxisType>( i );
                    }
                }
            }

            if( selectedAxis != AT_LAST )
            {
                m_selected[selectedAxis] = true;
                vAxis = selectedAxis == AT_X   ? Vector3<real_Num>::unitX()
                        : selectedAxis == AT_Y ? Vector3<real_Num>::unitY()
                                               : Vector3<real_Num>::unitZ();
            }
        }

        if( mouseStateEvent == IMouseState::Event::LeftPressed )
        {
            if( m_selected[AT_X] || m_selected[AT_Y] || m_selected[AT_Z] )
            {
                m_prevCursor = mousePosition;
                m_cursorPos = mousePosition;
                m_mouseButtonDown = true;
                return true;
            }
        }
        else if( mouseStateEvent == IMouseState::Event::LeftReleased )
        {
            const auto wasDragging = m_mouseButtonDown;
            m_mouseButtonDown = false;
            return wasDragging;
        }
        else if( mouseStateEvent == IMouseState::Event::Moved )
        {
            m_prevCursor = m_cursorPos;
            m_cursorPos = mousePosition;

            if( m_mouseButtonDown )
            {
                const auto mouseMove = m_cursorPos - m_prevCursor;
                const auto angle = ( mouseMove.X() - mouseMove.Y() ) * m_rotationSpeed;

                auto rotation = Vector3<real_Num>::zero();
                if( m_selected[AT_X] )
                {
                    rotation.X() = angle;
                }
                else if( m_selected[AT_Y] )
                {
                    rotation.Y() = angle;
                }
                else if( m_selected[AT_Z] )
                {
                    rotation.Z() = angle;
                }

                const auto delta =
                    Quaternion<real_Num>::eulerDegrees( rotation.X(), rotation.Y(), rotation.Z() );
                const auto selection = selectionManager->getSelection();
                for( const auto &selected : selection )
                {
                    auto actor = workphone::dynamic_pointer_cast<scene::IGameActor>( selected );
                    if( actor )
                    {
                        if( auto transform = actor->getTransform() )
                        {
                            auto orientation = transform->getLocalOrientation() * delta;
                            orientation.normalise();
                            transform->setLocalOrientation( orientation );
                            transform->setDirty( true );
                            actor->updateTransform();
                        }
                    }
                }

                updateManipulatorPosition();
                return true;
            }
        }

        return false;
    }

    auto RotateManipulator::getDrawDebugLines() const -> bool
    {
        return m_drawDebugLines;
    }

    void RotateManipulator::setDrawDebugLines( bool drawDebugLines )
    {
        if( m_drawDebugLines != drawDebugLines )
        {
            m_drawDebugLines = drawDebugLines;
            if( !m_drawDebugLines )
            {
                m_mouseButtonDown = false;
                setDebugCirclesVisible( false );
            }
        }
    }

    void RotateManipulator::setDebugCirclesVisible( bool visible )
    {
        for( auto &circle : m_circles )
        {
            if( circle )
            {
                circle->setVisible( visible );
                if( visible )
                {
                    circle->setMaxLifeTime( 1e10 );
                }
            }
        }
    }

}  // namespace workphone
