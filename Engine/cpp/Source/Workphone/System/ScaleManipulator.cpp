#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/ScaleManipulator.hpp>
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
#include <limits>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, ScaleManipulator, Manipulator );

    ScaleManipulator::ScaleManipulator( bool drawDebugLines ) : m_drawDebugLines( drawDebugLines )
    {
    }

    ScaleManipulator::~ScaleManipulator() = default;

    void ScaleManipulator::load( SmartPtr<ISharedObject> data )
    {
        m_lines.resize( 3 );
        m_selected.resize( 3 );

        Manipulator::load( data );

        if( auto applicationManager = core::IApplicationManager::instancePtr() )
        {
            if( applicationManager->getSelectionManager() )
            {
                updateManipulatorPosition();
            }
        }
    }

    void ScaleManipulator::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto &loadingState = getLoadingState();
            if( loadingState == LoadingState::Loaded )
            {
                setLoadingState( LoadingState::Unloading );
                setDebugLinesVisible( false );

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
                m_lines.clear();
                m_selected.clear();
                m_bMouseButtonDown = false;

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ScaleManipulator::preUpdate()
    {
        if( !isLoaded() || !m_drawDebugLines || !m_enabled )
        {
            setDebugLinesVisible( false );
            return;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager )
        {
            setDebugLinesVisible( false );
            return;
        }

        auto selectionManager = applicationManager->getSelectionManager();
        if( !selectionManager || selectionManager->getSelection().empty() )
        {
            setVisible( false );
            setDebugLinesVisible( false );
            return;
        }

        updateManipulatorPosition();
        if( !isVisible() )
        {
            setDebugLinesVisible( false );
            return;
        }

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        auto cameraManager = applicationManager->getCameraManager();
        if( !graphicsSystem || !cameraManager )
        {
            setDebugLinesVisible( false );
            return;
        }

        auto debug = graphicsSystem->getDebug();
        auto cameraActor = cameraManager->getEditorCamera();
        if( !debug || !cameraActor )
        {
            setDebugLinesVisible( false );
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

        const auto lines = getLines();
        static const auto hash = StringUtil::getHash( "ScaleManipulator" );

        for( size_t i = 0; i < lines.size(); ++i )
        {
            const auto &line = lines[i];
            m_lines[i] = debug->drawLine( hash + static_cast<hash_type>( i ), line.getStart(),
                                          line.getEnd(), colours[i] );
        }

        setDebugLinesVisible( true );
    }

    void ScaleManipulator::update()
    {
    }

    void ScaleManipulator::postUpdate()
    {
    }

    auto ScaleManipulator::handleEvent( const SmartPtr<IInputEvent> &event ) -> bool
    {
        if( !isLoaded() || !event )
        {
            return false;
        }

        if( !m_enabled || !m_drawDebugLines || !isVisible() )
        {
            return false;
        }

        //if( !isVisible() )
        //{
        //    return false;
        //}

        //if( event->getEventType() != IInputEvent::EventType::Mouse )
        //{
        //    return false;
        //}

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

        auto bResult = false;

        m_previousCursorPosition = m_currentCursorPosition;

        auto mouseState = event->getMouseState();
        if( mouseState )
        {
            auto mousePosition = mouseState->getAbsolutePosition();
            auto relativeMousePosition = mouseState->getRelativePosition();

            auto mouseStateRelative = ApplicationUtil::getRelativeMousePos( relativeMousePosition );

            m_currentCursorPosition.X() = mousePosition.X();
            m_currentCursorPosition.Y() = mousePosition.Y();

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

            auto ray = camera->getCameraToViewportRay( mouseStateRelative );
            auto handles = getHandles();

            if( !m_bMouseButtonDown )
            {
                for( size_t i = 0; i < m_selected.size(); ++i )
                {
                    m_selected[i] = false;
                }

                auto closestDistance = std::numeric_limits<real_Num>::max();
                auto selectedAxis = AT_LAST;
                for( size_t i = 0; i < handles.size(); ++i )
                {
                    const auto result = MathUtil<real_Num>::intersects( ray, handles[i] );
                    if( result.first && result.second >= real_Num( 0.0 ) &&
                        result.second < closestDistance )
                    {
                        closestDistance = result.second;
                        selectedAxis = static_cast<AxisType>( i );
                    }
                }

                if( selectedAxis != AT_LAST )
                {
                    m_selected[selectedAxis] = true;

                    const Vector3<real_Num> axes[] = { Vector3<real_Num>::unitX(),
                                                       Vector3<real_Num>::unitY(),
                                                       Vector3<real_Num>::unitZ() };
                    m_vAxis = axes[selectedAxis];
                }
            }

            // Mouse event
            auto mouseStateEvent = mouseState->getEventType();
            if( mouseStateEvent == IMouseState::Event::LeftReleased )
            {
                const auto wasDragging = m_bMouseButtonDown;
                m_bMouseButtonDown = false;
                bResult = wasDragging;
            }
            else if( mouseStateEvent == IMouseState::Event::LeftPressed )
            {
                if( m_selected[0] || m_selected[1] || m_selected[2] )
                {
                    m_bMouseButtonDown = true;
                    bResult = true;
                }
            }
            else if( ( mouseStateEvent == IMouseState::Event::Moved ) && m_bMouseButtonDown )
            {
                if( m_selected[0] || m_selected[1] || m_selected[2] )
                {
                    const auto mouseMove = m_currentCursorPosition - m_previousCursorPosition;
                    auto scaleFactor =
                        real_Num( 1.0 ) + ( mouseMove.X() - mouseMove.Y() ) * m_scaleSensitivity;
                    if( scaleFactor < real_Num( 0.01 ) )
                    {
                        scaleFactor = real_Num( 0.01 );
                    }

                    auto scaleDelta = Vector3<real_Num>::unit();
                    if( m_selected[AT_X] )
                    {
                        scaleDelta.X() = scaleFactor;
                    }
                    else if( m_selected[AT_Y] )
                    {
                        scaleDelta.Y() = scaleFactor;
                    }
                    else if( m_selected[AT_Z] )
                    {
                        scaleDelta.Z() = scaleFactor;
                    }

                    const auto selection = selectionManager->getSelection();
                    for( const auto &selected : selection )
                    {
                        auto actor = workphone::dynamic_pointer_cast<scene::IGameActor>( selected );
                        if( actor )
                        {
                            if( auto transform = actor->getTransform() )
                            {
                                transform->setLocalScale( transform->getLocalScale() * scaleDelta );
                                transform->setDirty( true );
                                actor->updateTransform();
                            }
                        }
                    }

                    updateManipulatorPosition();
                    bResult = true;
                }
            }
        }

        return bResult;
    }

    auto ScaleManipulator::getLines() const -> Array<Line3<real_Num>>
    {
        Array<Line3<real_Num>> lines;
        lines.resize( 3 );

        auto pos = getPosition();
        const auto length = m_handleHeight * m_fScale;
        lines[0] = Line3<real_Num>( pos, pos + Vector3<real_Num>::unitX() * length );
        lines[1] = Line3<real_Num>( pos, pos + Vector3<real_Num>::unitY() * length );
        lines[2] = Line3<real_Num>( pos, pos + Vector3<real_Num>::unitZ() * length );

        return lines;
    }

    auto ScaleManipulator::getHandles() const -> Array<Cylinder3<real_Num>>
    {
        Array<Cylinder3<real_Num>> handles;
        handles.resize( 3 );

        auto pos = getPosition();

        auto radius = m_handleRadius * m_fScale;
        auto height = m_handleHeight * m_fScale;

        const Vector3<real_Num> axes[] = { Vector3<real_Num>::unitX(), Vector3<real_Num>::unitY(),
                                           Vector3<real_Num>::unitZ() };
        for( size_t i = 0; i < handles.size(); ++i )
        {
            const auto centre = pos + axes[i] * ( height * real_Num( 0.5 ) );
            handles[i] =
                Cylinder3<real_Num>( Line3<real_Num>( centre, centre + axes[i] ), radius, height );
        }

        return handles;
    }

    auto ScaleManipulator::getDrawDebugLines() const -> bool
    {
        return m_drawDebugLines;
    }

    void ScaleManipulator::setDrawDebugLines( bool drawDebugLines )
    {
        if( m_drawDebugLines != drawDebugLines )
        {
            m_drawDebugLines = drawDebugLines;
            if( !m_drawDebugLines )
            {
                m_bMouseButtonDown = false;
                setDebugLinesVisible( false );
            }
        }
    }

    void ScaleManipulator::setDebugLinesVisible( bool visible )
    {
        for( auto &line : m_lines )
        {
            if( line )
            {
                line->setVisible( visible );
                if( visible )
                {
                    line->setMaxLifeTime( 1e10 );
                }
            }
        }
    }

}  // namespace workphone
