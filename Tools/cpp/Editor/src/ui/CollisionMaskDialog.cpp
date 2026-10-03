#include <EditorPCH.hpp>
#include <ui/CollisionMaskDialog.hpp>
#include <ui/ActorWindow.hpp>
#include <ui/CollisionMaskManager.hpp>
#include <ui/UIManager.hpp>
#include <editor/EditorManager.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, CollisionMaskDialog, EditorWindow );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, CollisionMaskDialog::UIElementListener,
                               IEventListener );

    namespace
    {
        u32 parseMaskValue( const String &value, u32 defaultValue )
        {
            try
            {
                size_t processedCharacters = 0;
                auto parsedValue = std::stoul( value, &processedCharacters, 0 );
                return processedCharacters > 0 ? static_cast<u32>( parsedValue ) : defaultValue;
            }
            catch( const std::exception & )
            {
                return defaultValue;
            }
        }
    }  // namespace

    CollisionMaskDialog::CollisionMaskDialog() = default;

    CollisionMaskDialog::~CollisionMaskDialog()
    {
        unload( nullptr );
    }

    void CollisionMaskDialog::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto ui = applicationManager->getUIPtr();
            WP_ASSERT( ui );

            auto parentWindow = ui->addElementByType<ui::IUIWindow>();
            WP_ASSERT( parentWindow );

            setParentWindow( parentWindow );
            parentWindow->setLabel( "Collision Masks" );
            parentWindow->setSize( Vector2F( 400.0f, 340.0f ) );

            auto uiListener = workphone::make_ptr<UIElementListener>();
            uiListener->setOwner( this );
            m_uiListener = uiListener;

            m_maskName = ui->addElementByType<ui::IUILabelTextInputPair>();
            m_maskName->setLabel( "Name" );
            m_maskName->setValue( "" );
            m_maskName->setElementId( MaskName );
            parentWindow->addChild( m_maskName );

            m_maskValue = ui->addElementByType<ui::IUILabelTextInputPair>();
            m_maskValue->setLabel( "Mask" );
            m_maskValue->setValue( "0" );
            m_maskValue->setElementId( MaskValue );
            parentWindow->addChild( m_maskValue );

            m_addMaskButton = ui->addElementByType<ui::IUIButton>();
            m_addMaskButton->setLabel( "Add Mask" );
            m_addMaskButton->setElementId( AddMask );
            m_addMaskButton->addObjectListener( uiListener );
            parentWindow->addChild( m_addMaskButton );

            m_removeMaskButton = ui->addElementByType<ui::IUIButton>();
            m_removeMaskButton->setLabel( "Remove Mask" );
            m_removeMaskButton->setSameLine( true );
            m_removeMaskButton->setElementId( RemoveMask );
            m_removeMaskButton->addObjectListener( uiListener );
            parentWindow->addChild( m_removeMaskButton );

            m_tree = ui->addElementByType<ui::IUITreeCtrl>();
            m_tree->setElementId( MaskTree );
            m_tree->setMultiSelect( false );
            m_tree->addObjectListener( uiListener );
            parentWindow->addChild( m_tree );

            populate();

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CollisionMaskDialog::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( !isLoaded() )
            {
                return;
            }

            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto ui = applicationManager->getUIPtr();
            WP_ASSERT( ui );

            if( m_tree )
            {
                m_tree->removeObjectListener( m_uiListener );
                ui->removeElement( m_tree );
                m_tree = nullptr;
            }

            if( m_addMaskButton )
            {
                ui->removeElement( m_addMaskButton );
                m_addMaskButton = nullptr;
            }

            if( m_removeMaskButton )
            {
                ui->removeElement( m_removeMaskButton );
                m_removeMaskButton = nullptr;
            }

            if( m_maskValue )
            {
                ui->removeElement( m_maskValue );
                m_maskValue = nullptr;
            }

            if( m_maskName )
            {
                ui->removeElement( m_maskName );
                m_maskName = nullptr;
            }

            if( auto parentWindow = getParentWindow() )
            {
                ui->removeElement( parentWindow );
                setParentWindow( nullptr );
            }

            if( m_uiListener )
            {
                m_uiListener->unload( nullptr );
                m_uiListener = nullptr;
            }

            EditorWindow::unload( data );
            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CollisionMaskDialog::populate()
    {
        if( !m_tree )
        {
            return;
        }

        m_tree->clear();

        auto editorManager = EditorManager::getSingletonPtr();
        auto uiManager = editorManager ? editorManager->getUI() : nullptr;
        auto maskManager = uiManager ? uiManager->getCollisionMaskManager() : nullptr;
        if( !maskManager )
        {
            return;
        }

        auto root = m_tree->addRoot();
        WP_ASSERT( root );
        Util::setText( root, "Collision Masks" );
        root->setExpanded( true );

        auto options = maskManager->getOptions();
        for( const auto &option : options )
        {
            auto node = m_tree->addNode();
            WP_ASSERT( node );
            Util::setText( node, option.name );
            root->addChild( node );
        }
    }

    void CollisionMaskDialog::setWindowVisible( bool visible )
    {
        if( visible )
        {
            populate();
        }

        EditorWindow::setWindowVisible( visible );
    }

    CollisionMaskDialog::UIElementListener::UIElementListener() = default;

    CollisionMaskDialog::UIElementListener::~UIElementListener() = default;

    Parameter CollisionMaskDialog::UIElementListener::handleEvent(
        EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object,
        SmartPtr<IEvent> event )
    {
        auto owner = getOwner();
        if( !owner )
        {
            return {};
        }

        auto editorManager = EditorManager::getSingletonPtr();
        auto uiManager = editorManager ? editorManager->getUI() : nullptr;
        auto maskManager = uiManager ? uiManager->getCollisionMaskManager() : nullptr;
        if( !maskManager )
        {
            return {};
        }

        auto element = workphone::dynamic_pointer_cast<ui::IUIElement>( sender );
        auto elementId = element ? static_cast<WidgetId>( element->getElementId() ) : WidgetId::Count;

        if( eventValue == IEvent::handleSelection )
        {
            switch( elementId )
            {
            case WidgetId::MaskTree:
            {
                if( auto selectedNode = owner->m_tree->getSelectedTreeNode() )
                {
                    const auto optionIndex = maskManager->getOptionIndexByName( selectedNode->getLabel() );
                    owner->m_maskName->setValue( maskManager->getOptionNameByIndex( optionIndex ) );
                    owner->m_maskValue->setValue(
                        StringUtil::toString( maskManager->getMaskByIndex( optionIndex ) ) );
                }
            }
            break;
            case WidgetId::AddMask:
            {
                const auto mask = parseMaskValue( owner->m_maskValue->getValue(),
                                                  maskManager->getDefaultMask() );
                maskManager->addOption( owner->m_maskName->getValue(), mask );
                owner->m_maskValue->setValue( StringUtil::toString( mask ) );
                owner->populate();
            }
            break;
            case WidgetId::RemoveMask:
            {
                auto maskName = owner->m_maskName->getValue();
                if( auto selectedNode = owner->m_tree->getSelectedTreeNode() )
                {
                    maskName = selectedNode->getLabel();
                }

                maskManager->removeOption( maskName );
                owner->m_maskName->setValue( "" );
                owner->m_maskValue->setValue( StringUtil::toString( maskManager->getDefaultMask() ) );
                owner->populate();
            }
            break;
            default:
            {
            }
            break;
            }

            if( uiManager )
            {
                if( auto actorWindow = uiManager->getActorWindow() )
                {
                    actorWindow->refreshCollisionMaskDropdown();
                }
            }
        }

        return {};
    }

    SmartPtr<CollisionMaskDialog> CollisionMaskDialog::UIElementListener::getOwner() const
    {
        return m_owner.lock();
    }

    void CollisionMaskDialog::UIElementListener::setOwner( SmartPtr<CollisionMaskDialog> owner )
    {
        m_owner = owner;
    }
}  // namespace workphone::editor
