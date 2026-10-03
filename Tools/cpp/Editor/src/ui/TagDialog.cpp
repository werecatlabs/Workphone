#include <EditorPCH.hpp>
#include <ui/TagDialog.hpp>
#include <ui/ActorWindow.hpp>
#include <ui/TagManager.hpp>
#include <ui/UIManager.hpp>
#include <editor/EditorManager.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, TagDialog, EditorWindow );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, TagDialog::UIElementListener, IEventListener );

    namespace
    {
        SmartPtr<scene::IGameActor> getActorFromSelection( SmartPtr<ISharedObject> selected )
        {
            if( !selected )
            {
                return nullptr;
            }

            if( selected->isDerived<scene::IGameActor>() )
            {
                return workphone::static_pointer_cast<scene::IGameActor>( selected );
            }

            if( selected->isDerived<scene::IComponent>() )
            {
                auto component = workphone::static_pointer_cast<scene::IComponent>( selected );
                return component ? component->getActor() : nullptr;
            }

            return nullptr;
        }
    }  // namespace

    TagDialog::TagDialog() = default;

    TagDialog::~TagDialog()
    {
        unload( nullptr );
    }

    void TagDialog::load( SmartPtr<ISharedObject> data )
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
            parentWindow->setLabel( "Tags" );
            parentWindow->setSize( Vector2F( 400.0f, 340.0f ) );

            auto uiListener = workphone::make_ptr<UIElementListener>();
            uiListener->setOwner( this );
            m_uiListener = uiListener;

            m_tagName = ui->addElementByType<ui::IUILabelTextInputPair>();
            m_tagName->setLabel( "Tag" );
            m_tagName->setValue( "" );
            m_tagName->setElementId( TagName );
            parentWindow->addChild( m_tagName );

            m_addTagOptionButton = ui->addElementByType<ui::IUIButton>();
            m_addTagOptionButton->setLabel( "Add Option" );
            m_addTagOptionButton->setElementId( AddTagOption );
            m_addTagOptionButton->addObjectListener( uiListener );
            parentWindow->addChild( m_addTagOptionButton );

            m_removeTagOptionButton = ui->addElementByType<ui::IUIButton>();
            m_removeTagOptionButton->setLabel( "Remove Option" );
            m_removeTagOptionButton->setSameLine( true );
            m_removeTagOptionButton->setElementId( RemoveTagOption );
            m_removeTagOptionButton->addObjectListener( uiListener );
            parentWindow->addChild( m_removeTagOptionButton );

            m_addToActorButton = ui->addElementByType<ui::IUIButton>();
            m_addToActorButton->setLabel( "Add To Actor" );
            m_addToActorButton->setElementId( AddToActor );
            m_addToActorButton->addObjectListener( uiListener );
            parentWindow->addChild( m_addToActorButton );

            m_removeFromActorButton = ui->addElementByType<ui::IUIButton>();
            m_removeFromActorButton->setLabel( "Remove From Actor" );
            m_removeFromActorButton->setSameLine( true );
            m_removeFromActorButton->setElementId( RemoveFromActor );
            m_removeFromActorButton->addObjectListener( uiListener );
            parentWindow->addChild( m_removeFromActorButton );

            m_tree = ui->addElementByType<ui::IUITreeCtrl>();
            m_tree->setElementId( TagTree );
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

    void TagDialog::unload( SmartPtr<ISharedObject> data )
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

            if( m_addTagOptionButton )
            {
                ui->removeElement( m_addTagOptionButton );
                m_addTagOptionButton = nullptr;
            }

            if( m_removeTagOptionButton )
            {
                ui->removeElement( m_removeTagOptionButton );
                m_removeTagOptionButton = nullptr;
            }

            if( m_addToActorButton )
            {
                ui->removeElement( m_addToActorButton );
                m_addToActorButton = nullptr;
            }

            if( m_removeFromActorButton )
            {
                ui->removeElement( m_removeFromActorButton );
                m_removeFromActorButton = nullptr;
            }

            if( m_tagName )
            {
                ui->removeElement( m_tagName );
                m_tagName = nullptr;
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

    void TagDialog::populate()
    {
        if( !m_tree )
        {
            return;
        }

        m_tree->clear();

        auto editorManager = EditorManager::getSingletonPtr();
        auto uiManager = editorManager ? editorManager->getUI() : nullptr;
        auto tagManager = uiManager ? uiManager->getTagManager() : nullptr;
        if( !tagManager )
        {
            return;
        }

        tagManager->refreshFromScene();

        auto root = m_tree->addRoot();
        WP_ASSERT( root );
        Util::setText( root, "Tag Options" );
        root->setExpanded( true );

        auto tags = tagManager->getTags();
        for( const auto &tag : tags )
        {
            auto node = m_tree->addNode();
            WP_ASSERT( node );
            Util::setText( node, tag );
            root->addChild( node );
        }
    }

    void TagDialog::setWindowVisible( bool visible )
    {
        if( visible )
        {
            populate();
        }

        EditorWindow::setWindowVisible( visible );
    }

    String TagDialog::getEditedTag() const
    {
        if( m_tree )
        {
            if( auto selectedNode = m_tree->getSelectedTreeNode() )
            {
                auto selectedTag = StringUtil::trim( selectedNode->getLabel() );
                if( !StringUtil::isNullOrEmpty( selectedTag ) && selectedTag != "Tag Options" )
                {
                    return selectedTag;
                }
            }
        }

        return m_tagName ? StringUtil::trim( m_tagName->getValue() ) : String();
    }

    void TagDialog::updateActorTagDisplay()
    {
        auto editorManager = EditorManager::getSingletonPtr();
        auto uiManager = editorManager ? editorManager->getUI() : nullptr;
        if( uiManager )
        {
            if( auto actorWindow = uiManager->getActorWindow() )
            {
                actorWindow->refreshTagDisplay();
            }
        }
    }

    TagDialog::UIElementListener::UIElementListener() = default;

    TagDialog::UIElementListener::~UIElementListener() = default;

    Parameter TagDialog::UIElementListener::handleEvent(
        EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        auto owner = getOwner();
        if( !owner )
        {
            return {};
        }

        auto editorManager = EditorManager::getSingletonPtr();
        auto uiManager = editorManager ? editorManager->getUI() : nullptr;
        auto tagManager = uiManager ? uiManager->getTagManager() : nullptr;
        if( !tagManager )
        {
            return {};
        }

        auto element = workphone::dynamic_pointer_cast<ui::IUIElement>( sender );
        auto elementId = element ? static_cast<WidgetId>( element->getElementId() ) : WidgetId::Count;

        if( eventValue == IEvent::handleSelection )
        {
            switch( elementId )
            {
            case WidgetId::TagTree:
            {
                if( auto selectedNode = owner->m_tree->getSelectedTreeNode() )
                {
                    auto selectedTag = StringUtil::trim( selectedNode->getLabel() );
                    if( selectedTag != "Tag Options" )
                    {
                        owner->m_tagName->setValue( selectedTag );
                    }
                }
            }
            break;
            case WidgetId::AddTagOption:
            {
                tagManager->addTag( owner->m_tagName->getValue() );
                owner->populate();
            }
            break;
            case WidgetId::RemoveTagOption:
            {
                tagManager->removeTag( owner->getEditedTag() );
                owner->m_tagName->setValue( "" );
                owner->populate();
                owner->updateActorTagDisplay();
            }
            break;
            case WidgetId::AddToActor:
            {
                auto tag = owner->getEditedTag();
                if( !StringUtil::isNullOrEmpty( tag ) )
                {
                    tagManager->addTag( tag );

                    auto applicationManager = core::IApplicationManager::instancePtr();
                    auto selectionManager =
                        applicationManager ? applicationManager->getSelectionManagerPtr() : nullptr;
                    auto selection = selectionManager ? selectionManager->getSelection() :
                                                        Array<SmartPtr<ISharedObject>>();

                    for( auto selected : selection )
                    {
                        auto actor = getActorFromSelection( selected );
                        if( actor )
                        {
                            actor->addTag( tag );
                        }
                    }

                    owner->populate();
                    owner->updateActorTagDisplay();
                }
            }
            break;
            case WidgetId::RemoveFromActor:
            {
                auto tag = owner->getEditedTag();
                if( !StringUtil::isNullOrEmpty( tag ) )
                {
                    auto applicationManager = core::IApplicationManager::instancePtr();
                    auto selectionManager =
                        applicationManager ? applicationManager->getSelectionManagerPtr() : nullptr;
                    auto selection = selectionManager ? selectionManager->getSelection() :
                                                        Array<SmartPtr<ISharedObject>>();

                    for( auto selected : selection )
                    {
                        auto actor = getActorFromSelection( selected );
                        if( actor )
                        {
                            actor->removeTag( tag );
                        }
                    }

                    owner->updateActorTagDisplay();
                }
            }
            break;
            default:
            {
            }
            break;
            }
        }

        return {};
    }

    SmartPtr<TagDialog> TagDialog::UIElementListener::getOwner() const
    {
        return m_owner.lock();
    }

    void TagDialog::UIElementListener::setOwner( SmartPtr<TagDialog> owner )
    {
        m_owner = owner;
    }
}  // namespace workphone::editor
