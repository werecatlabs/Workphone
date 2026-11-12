#include <EditorPCH.hpp>
#include <editor/OpenAISettingsControls.hpp>
#include <Workphone/AI/OpenAICredentials.hpp>
#include <Workphone/AI/MeshyCredentials.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    namespace
    {
        struct CredentialSettings
        {
            const char *name;
            const char *environment;
            const char *entryName;
            const char *saveName;
            const char *saveLabel;
            const char *removeLabel;
            const char *savedMessage;
            bool ( *has )( String & );
            bool ( *save )( const String &, String & );
            bool ( *clear )( String & );
        };

        const CredentialSettings openAISettings{
            "OpenAI", "OPENAI_API_KEY", "EditorOpenAIKey", "EditorOpenAIKeySave",
            "Save API Key", "Remove Saved Key",
            "API key saved. New AI chat and image requests will use it.",
            openai_credentials::hasEditorKey, openai_credentials::saveEditorKey,
            openai_credentials::clearEditorKey };
        const CredentialSettings meshySettings{
            "Meshy", "MESHY_API_KEY", "EditorMeshyKey", "EditorMeshyKeySave",
            "Save Meshy Key", "Remove Saved Meshy Key",
            "Meshy API key saved. New Image to Mesh requests will use it.",
            meshy_credentials::hasEditorKey, meshy_credentials::saveEditorKey,
            meshy_credentials::clearEditorKey };
    }

    class ApiKeySettingsListener : public IEventListener
    {
    public:
        enum Action { Save = 1, Remove };
        const CredentialSettings *settings = nullptr;
        AtomicWeakPtr<ui::IUITextEntry> entry;
        AtomicWeakPtr<ui::IUIText> status;
        AtomicWeakPtr<ui::IUIButton> remove;

        void refresh( const String &message = {} )
        {
            auto output = status.load().lock();
            if( !output || !settings ) return;
            String error;
            const bool saved = settings->has( error );
            if( auto button = remove.load().lock() ) button->setEnabled( saved || !error.empty() );
            if( !error.empty() ) output->setText( error );
            else if( !message.empty() ) output->setText( message );
            else output->setText( saved ? String( "A " ) + settings->name + " API key is saved for this Windows user."
                                       : String( "No saved key. " ) + settings->environment + " is used if it is set." );
        }

        Parameter handleEvent( EventType, hash_type eventValue, const Array<Parameter> &,
                               SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject>,
                               SmartPtr<IEvent> ) override
        {
            if( eventValue != IEvent::handleSelection || !settings ) return {};
            auto input = entry.load().lock();
            auto output = status.load().lock();
            auto button = dynamic_pointer_cast<ui::IUIButton>( sender );
            if( !input || !output || !button || !button->isEnabled() ) return {};

            String error;
            bool success = false;
            if( button->getElementId() == Save )
                success = settings->save( input->getText(), error );
            else if( button->getElementId() == Remove )
                success = settings->clear( error );
            else return {};

            if( success )
            {
                input->setText( "" );
                refresh( button->getElementId() == Save
                             ? String( settings->savedMessage )
                             : String( "Saved key removed. " ) + settings->environment + " is used if it is set." );
            }
            else output->setText( error.empty() ? String( "Could not update the " ) + settings->name + " API key." : error );
            return {};
        }

        WP_CLASS_REGISTER_DECL;
    };

    WP_CLASS_REGISTER_DERIVED( workphone::editor, ApiKeySettingsListener, IEventListener );

    namespace
    {
        void createCredentialControls( SmartPtr<ui::IUIElement> parent, const CredentialSettings &settings )
        {
            auto app = core::IApplicationManager::instancePtr();
            auto ui = app ? app->getUI() : nullptr;
            if( !ui || !parent ) return;

            auto hint = ui->addElementByType<ui::IUIText>();
            hint->setText( String( settings.name ) + " credentials apply to this Editor user across all projects.\n"
                           "The key is stored in Windows Credential Manager, outside project files." );
            parent->addChild( hint );

            auto listener = make_ptr<ApiKeySettingsListener>();
            listener->settings = &settings;
            auto input = ui->addElementByType<ui::IUITextEntry>();
            input->setLabel( String( settings.name ) + " API key" );
            input->setName( settings.entryName );
            input->setSecureEntry( true );
            input->setPlaceholder( "Enter a key to save or replace the saved key" );
            parent->addChild( input );
            listener->entry = input;

            auto save = ui->addElementByType<ui::IUIButton>();
            save->setLabel( settings.saveLabel );
            save->setName( settings.saveName );
            save->setElementId( ApiKeySettingsListener::Save );
            save->addObjectListener( listener );
            parent->addChild( save );

            auto remove = ui->addElementByType<ui::IUIButton>();
            remove->setLabel( settings.removeLabel );
            remove->setElementId( ApiKeySettingsListener::Remove );
            remove->setSameLine( true );
            remove->addObjectListener( listener );
            parent->addChild( remove );
            listener->remove = remove;

            auto status = ui->addElementByType<ui::IUIText>();
            parent->addChild( status );
            listener->status = status;
#if !defined( _WIN32 )
            hint->setText( String( "Saving Editor credentials is available on Windows.\nOn this platform, set " ) +
                           settings.environment + " before starting the Editor." );
            input->setReadOnly( true );
            save->setEnabled( false );
#endif
            listener->refresh();
        }

        void refreshCredentialControls( SmartPtr<ui::IUIElement> parent, const CredentialSettings &settings )
        {
            if( !parent ) return;
            for( auto child : parent->getChildren() )
            {
                if( child->getName() != settings.saveName ) continue;
                for( auto listener : child->getObjectListeners() )
                {
                    if( auto controls = dynamic_pointer_cast<ApiKeySettingsListener>( listener ) )
                        controls->refresh();
                }
                break;
            }
        }
    }

    void createOpenAISettingsControls( SmartPtr<ui::IUIElement> parent )
    {
        createCredentialControls( parent, openAISettings );
    }

    void refreshOpenAISettingsControls( SmartPtr<ui::IUIElement> parent )
    {
        refreshCredentialControls( parent, openAISettings );
    }

    void createMeshySettingsControls( SmartPtr<ui::IUIElement> parent )
    {
        createCredentialControls( parent, meshySettings );
    }

    void refreshMeshySettingsControls( SmartPtr<ui::IUIElement> parent )
    {
        refreshCredentialControls( parent, meshySettings );
    }
}
