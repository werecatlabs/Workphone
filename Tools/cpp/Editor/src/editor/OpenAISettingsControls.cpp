#include <EditorPCH.hpp>
#include <editor/OpenAISettingsControls.hpp>
#include <Workphone/AI/OpenAICredentials.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    class OpenAISettingsListener : public IEventListener
    {
    public:
        enum Action { Save = 1, Remove };
        AtomicWeakPtr<ui::IUITextEntry> entry;
        AtomicWeakPtr<ui::IUIText> status;
        AtomicWeakPtr<ui::IUIButton> remove;

        void refresh( const String &message = {} )
        {
            auto output = status.load().lock();
            if( !output ) return;
            String error;
            const bool saved = openai_credentials::hasEditorKey( error );
            if( auto button = remove.load().lock() ) button->setEnabled( saved || !error.empty() );
            if( !error.empty() ) output->setText( error );
            else if( !message.empty() ) output->setText( message );
            else output->setText( saved ? "An OpenAI API key is saved for this Windows user."
                                       : "No saved key. OPENAI_API_KEY is used if it is set." );
        }

        Parameter handleEvent( EventType, hash_type eventValue, const Array<Parameter> &,
                               SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject>,
                               SmartPtr<IEvent> ) override
        {
            if( eventValue != IEvent::handleSelection ) return {};
            auto input = entry.load().lock();
            auto output = status.load().lock();
            auto button = dynamic_pointer_cast<ui::IUIButton>( sender );
            if( !input || !output || !button || !button->isEnabled() ) return {};

            String error;
            bool success = false;
            if( button->getElementId() == Save )
                success = openai_credentials::saveEditorKey( input->getText(), error );
            else if( button->getElementId() == Remove )
                success = openai_credentials::clearEditorKey( error );
            else return {};

            if( success )
            {
                input->setText( "" );
                refresh( button->getElementId() == Save
                             ? "API key saved. New AI chat and image requests will use it."
                             : "Saved key removed. OPENAI_API_KEY is used if it is set." );
            }
            else output->setText( error.empty() ? String( "Could not update the OpenAI API key." ) : error );
            return {};
        }

        WP_CLASS_REGISTER_DECL;
    };

    WP_CLASS_REGISTER_DERIVED( workphone::editor, OpenAISettingsListener, IEventListener );

    void createOpenAISettingsControls( SmartPtr<ui::IUIElement> parent )
    {
        auto app = core::IApplicationManager::instancePtr();
        auto ui = app ? app->getUI() : nullptr;
        if( !ui || !parent ) return;

        auto hint = ui->addElementByType<ui::IUIText>();
        hint->setText( "OpenAI credentials apply to this Editor user across all projects.\n"
                       "The key is stored in Windows Credential Manager, outside project files." );
        parent->addChild( hint );

        auto listener = make_ptr<OpenAISettingsListener>();
        auto input = ui->addElementByType<ui::IUITextEntry>();
        input->setLabel( "OpenAI API key" );
        input->setName( "EditorOpenAIKey" );
        input->setSecureEntry( true );
        input->setPlaceholder( "Enter a key to save or replace the saved key" );
        parent->addChild( input );
        listener->entry = input;

        auto save = ui->addElementByType<ui::IUIButton>();
        save->setLabel( "Save API Key" );
        save->setName( "EditorOpenAIKeySave" );
        save->setElementId( OpenAISettingsListener::Save );
        save->addObjectListener( listener );
        parent->addChild( save );

        auto remove = ui->addElementByType<ui::IUIButton>();
        remove->setLabel( "Remove Saved Key" );
        remove->setElementId( OpenAISettingsListener::Remove );
        remove->setSameLine( true );
        remove->addObjectListener( listener );
        parent->addChild( remove );
        listener->remove = remove;

        auto status = ui->addElementByType<ui::IUIText>();
        parent->addChild( status );
        listener->status = status;
#if !defined( _WIN32 )
        hint->setText( "Saving Editor credentials is available on Windows.\n"
                       "On this platform, set OPENAI_API_KEY before starting the Editor." );
        input->setReadOnly( true );
        save->setEnabled( false );
#endif
        listener->refresh();
    }

    void refreshOpenAISettingsControls( SmartPtr<ui::IUIElement> parent )
    {
        if( !parent ) return;
        for( auto child : parent->getChildren() )
        {
            if( child->getName() != "EditorOpenAIKeySave" ) continue;
            for( auto listener : child->getObjectListeners() )
            {
                if( auto controls = dynamic_pointer_cast<OpenAISettingsListener>( listener ) )
                    controls->refresh();
            }
            break;
        }
    }
}
