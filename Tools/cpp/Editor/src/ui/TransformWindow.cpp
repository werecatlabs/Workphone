#include <EditorPCH.hpp>
#include "ui/TransformWindow.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, TransformWindow, EditorWindow );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, TransformWindow::VectorListener, IEventListener );

    TransformWindow::TransformWindow() = default;

    TransformWindow::~TransformWindow() = default;

    void TransformWindow::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() || isLoading() )
            {
                WP_LOG_WARNING( "TransformWindow::load: already loading or loaded, skipping" );
                return;
            }

            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                WP_LOG_ERROR( "TransformWindow::load: application manager is null" );
                setLoadingState( LoadingState::None );
                return;
            }

            auto ui = applicationManager->getUI();
            if( !ui )
            {
                WP_LOG_ERROR( "TransformWindow::load: UI manager is null" );
                setLoadingState( LoadingState::None );
                return;
            }

            auto parent = getParent();

            auto parentWindow = ui->addElementByType<ui::IUIWindow>();
            if( !parentWindow )
            {
                WP_LOG_ERROR( "TransformWindow::load: failed to create parent window" );
                setLoadingState( LoadingState::None );
                return;
            }

            setParentWindow( parentWindow );
            parentWindow->setLabel( "TransformWindow" );

            if( getShowWorldTransform() )
            {
                parentWindow->setSize( Vector2F( 0.0f, 150.0f ) );
            }
            else
            {
                parentWindow->setSize( Vector2F( 0.0f, 85.0f ) );
            }

            parentWindow->setHasBorder( true );

            if( parent )
            {
                parent->addChild( parentWindow );
            }

            m_localPosition = createVectorField( ui, parentWindow, "Local Position" );
            m_localRotation = createVectorField( ui, parentWindow, "Local Rotation" );
            m_localScale = createVectorField( ui, parentWindow, "Local Scale" );

            if( !m_localPosition || !m_localRotation || !m_localScale )
            {
                WP_LOG_ERROR( "TransformWindow::load: failed to create local transform UI elements" );
                setLoadingState( LoadingState::None );
                return;
            }

            m_localPositionListener = setupListener( m_localPosition, scene::ITransform::Type::LocalPosition );
            m_localRotationListener = setupListener( m_localRotation, scene::ITransform::Type::LocalRotation );
            m_localScaleListener = setupListener( m_localScale, scene::ITransform::Type::LocalScale );

            if( getShowWorldTransform() )
            {
                m_position = createVectorField( ui, parentWindow, "Position" );
                m_rotation = createVectorField( ui, parentWindow, "Rotation" );
                m_scale = createVectorField( ui, parentWindow, "Scale" );

                if( !m_position || !m_rotation || !m_scale )
                {
                    WP_LOG_ERROR( "TransformWindow::load: failed to create world transform UI elements" );
                    setLoadingState( LoadingState::None );
                    return;
                }

                m_positionListener = setupListener( m_position, scene::ITransform::Type::Position );
                m_rotationListener = setupListener( m_rotation, scene::ITransform::Type::Rotation );
                m_scaleListener = setupListener( m_scale, scene::ITransform::Type::Scale );
            }

            if( parentWindow->isValid() )
            {
                setLoadingState( LoadingState::Loaded );
            }
            else
            {
                WP_LOG_ERROR( "TransformWindow::load: parent window is not valid after setup" );
                setLoadingState( LoadingState::None );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            setLoadingState( LoadingState::None );
        }
    }

    SmartPtr<ui::IUIVector3> TransformWindow::createVectorField( SmartPtr<ui::IUIManager> ui,
                                                                  SmartPtr<ui::IUIWindow> parentWindow,
                                                                  const String &label )
    {
        if( !ui )
        {
            WP_LOG_ERROR( "TransformWindow::createVectorField: UI manager is null" );
            return nullptr;
        }

        auto vectorField = ui->addElementByType<ui::IUIVector3>();
        if( !vectorField )
        {
            WP_LOG_ERROR( "TransformWindow::createVectorField: failed to create vector field" );
            return nullptr;
        }

        vectorField->setLabel( label );

        if( parentWindow )
        {
            parentWindow->addChild( vectorField );
        }

        return vectorField;
    }

    SmartPtr<TransformWindow::VectorListener> TransformWindow::setupListener( SmartPtr<ui::IUIVector3> vectorUI,
                                                                               scene::ITransform::Type type )
    {
        if( !vectorUI )
        {
            WP_LOG_ERROR( "TransformWindow::setupListener: vectorUI is null" );
            return nullptr;
        }

        auto listener = workphone::make_ptr<VectorListener>();
        if( !listener )
        {
            WP_LOG_ERROR( "TransformWindow::setupListener: failed to create listener" );
            return nullptr;
        }

        listener->setOwner( this );
        listener->setVectorUI( vectorUI );
        listener->setType( type );
        vectorUI->addObjectListener( listener );

        return listener;
    }

    void TransformWindow::cleanupListener( SmartPtr<VectorListener> &listener,
                                           SmartPtr<ui::IUIVector3> vectorUI )
    {
        if( !listener )
        {
            return;
        }

        try
        {
            if( vectorUI )
            {
                vectorUI->removeObjectListener( listener );
            }

            listener->unload( nullptr );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        listener = nullptr;
    }

    void TransformWindow::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( !isLoaded() )
            {
                return;
            }

            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                WP_LOG_ERROR( "TransformWindow::unload: application manager is null" );
            }

            auto ui = applicationManager ? applicationManager->getUI() : nullptr;
            if( !ui )
            {
                WP_LOG_WARNING( "TransformWindow::unload: UI manager is null, attempting listener cleanup only" );
            }

            cleanupListener( m_localPositionListener, m_localPosition );
            cleanupListener( m_localRotationListener, m_localRotation );
            cleanupListener( m_localScaleListener, m_localScale );
            cleanupListener( m_positionListener, m_position );
            cleanupListener( m_rotationListener, m_rotation );
            cleanupListener( m_scaleListener, m_scale );

            if( ui )
            {
                auto removeUIElement = [ui]( SmartPtr<ui::IUIVector3> &element ) mutable
                {
                    if( element )
                    {
                        try
                        {
                            ui->removeElement( element );
                        }
                        catch( std::exception &e )
                        {
                            WP_LOG_EXCEPTION( e );
                        }
                        element = nullptr;
                    }
                };

                removeUIElement( m_localPosition );
                removeUIElement( m_localRotation );
                removeUIElement( m_localScale );
                removeUIElement( m_position );
                removeUIElement( m_rotation );
                removeUIElement( m_scale );

                if( auto parentWindow = getParentWindow() )
                {
                    try
                    {
                        ui->removeElement( parentWindow );
                    }
                    catch( std::exception &e )
                    {
                        WP_LOG_EXCEPTION( e );
                    }
                    setParentWindow( nullptr );
                }
            }
            else
            {
                m_localPosition = nullptr;
                m_localRotation = nullptr;
                m_localScale = nullptr;
                m_position = nullptr;
                m_rotation = nullptr;
                m_scale = nullptr;
                setParentWindow( nullptr );
            }

            try
            {
                EditorWindow::unload( nullptr );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            setLoadingState( LoadingState::Unloaded );
        }
    }

    void TransformWindow::updateSelection()
    {
        if( !isLoaded() )
        {
            return;
        }

        refreshTransformUI();
    }

    void TransformWindow::refreshTransformUI()
    {
        auto transform = getTransform();
        if( !transform )
        {
            return;
        }

        try
        {
            if( m_localPosition )
            {
                m_localPosition->setValue( transform->getLocalPosition() );
            }

            if( m_localRotation )
            {
                m_localRotation->setValue( transform->getLocalRotation() );
            }

            if( m_localScale )
            {
                m_localScale->setValue( transform->getLocalScale() );
            }

            if( m_position )
            {
                m_position->setValue( transform->getPosition() );
            }

            if( m_rotation )
            {
                m_rotation->setValue( transform->getRotation() );
            }

            if( m_scale )
            {
                m_scale->setValue( transform->getScale() );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<scene::ITransform> TransformWindow::getTransform() const
    {
        return m_transform;
    }

    void TransformWindow::setTransform( SmartPtr<scene::ITransform> transform )
    {
        m_transform = transform;

        if( isLoaded() )
        {
            refreshTransformUI();
        }
    }

    bool TransformWindow::getShowWorldTransform() const
    {
        return m_showWorldTransform;
    }

    void TransformWindow::setShowWorldTransform( bool showWorldTransform )
    {
        m_showWorldTransform = showWorldTransform;
    }

    void TransformWindow::VectorListener::unload( SmartPtr<ISharedObject> data )
    {
        m_vectorUI = nullptr;
        m_owner = nullptr;
    }

    Parameter TransformWindow::VectorListener::handleEvent( EventType eventType, hash_type eventValue,
                                                             const Array<Parameter> &arguments,
                                                             SmartPtr<ISharedObject> sender,
                                                             SmartPtr<ISharedObject> object,
                                                             SmartPtr<IEvent> event )
    {
        try
        {
            if( eventValue == IEvent::handleValueChanged )
            {
                handleValueChanged();
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return {};
    }

    void TransformWindow::VectorListener::handleValueChanged()
    {
        auto owner = getOwner();
        if( !owner )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager )
        {
            WP_LOG_ERROR( "TransformWindow::VectorListener::handleValueChanged: application manager is null" );
            return;
        }

        auto sceneManager = applicationManager->getGameManagerPtr();
        if( !sceneManager )
        {
            WP_LOG_ERROR( "TransformWindow::VectorListener::handleValueChanged: scene manager is null" );
            return;
        }

        auto transform = owner->getTransform();
        if( !transform )
        {
            return;
        }

        auto actor = transform->getActor();
        if( !actor )
        {
            WP_LOG_WARNING( "TransformWindow::VectorListener::handleValueChanged: transform has no actor" );
            return;
        }

        auto vectorUI = getVectorUI();
        if( !vectorUI )
        {
            WP_LOG_WARNING( "TransformWindow::VectorListener::handleValueChanged: vector UI is null" );
            return;
        }

        auto value = vectorUI->getValue();

        try
        {
            switch( m_type )
            {
            case scene::ITransform::Type::LocalPosition:
            {
                actor->setLocalPosition( value );
            }
            break;
            case scene::ITransform::Type::LocalRotation:
            {
                actor->setLocalRotation( value );
            }
            break;
            case scene::ITransform::Type::LocalScale:
            {
                actor->setLocalScale( value );
            }
            break;
            case scene::ITransform::Type::Position:
            {
                transform->setPosition( value );
                transform->setLocalDirty( true );
            }
            break;
            case scene::ITransform::Type::Rotation:
            {
                transform->setRotation( value );
                transform->setLocalDirty( true );
            }
            break;
            case scene::ITransform::Type::Scale:
            {
                transform->setScale( value );
                transform->setLocalDirty( true );
            }
            break;
            default:
            {
                WP_LOG_WARNING( "TransformWindow::VectorListener::handleValueChanged: unknown transform type" );
            }
            break;
            }

            sceneManager->addDirtyActor( actor );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<TransformWindow> TransformWindow::VectorListener::getOwner() const
    {
        auto p = m_owner.lock();
        return p;
    }

    void TransformWindow::VectorListener::setOwner( SmartPtr<TransformWindow> owner )
    {
        m_owner = owner;
    }

    SmartPtr<ui::IUIVector3> TransformWindow::VectorListener::getVectorUI() const
    {
        return m_vectorUI.lock();
    }

    void TransformWindow::VectorListener::setVectorUI( SmartPtr<ui::IUIVector3> vectorUI )
    {
        m_vectorUI = vectorUI;
    }

    scene::ITransform::Type TransformWindow::VectorListener::getType() const
    {
        return m_type;
    }

    void TransformWindow::VectorListener::setType( scene::ITransform::Type type )
    {
        m_type = type;
    }

    TransformWindow::VectorListener::VectorListener() = default;

    TransformWindow::VectorListener::~VectorListener() = default;
}  // namespace workphone::editor
