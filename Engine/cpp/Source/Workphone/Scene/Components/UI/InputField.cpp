#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/UI/InputField.hpp>
#include <Workphone/Scene/Components/UI/LayoutTransform.hpp>
#include <Workphone/Interface/UI/IUIManager.hpp>
#include <Workphone/Interface/UI/IUITextEntry.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Input/IInputEvent.hpp>
#include <Workphone/Interface/Input/IKeyboardState.hpp>
#include <Workphone/Interface/Input/IMouseState.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Properties.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, InputField, UIComponent );

    const String InputField::textStr = "text";
    const String InputField::placeholderStr = "placeholder";
    const String InputField::readOnlyStr = "readOnly";
    const String InputField::secureEntryStr = "secureEntry";
    const String InputField::multilineStr = "multiline";
    const String InputField::inputTypeStr = "inputType";
    const String InputField::textHintStr = "textHint";
    const String InputField::textSizeStr = "textSize";

    InputField::InputField()
    {
        m_readOnly = false;
        m_secureEntry = false;
        m_multiline = false;
        m_inputType = ui::IUITextEntry::InputType::Text;
        m_textSize = 12;
    }

    InputField::~InputField()
    {
    }

    void InputField::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );
            createUI();
            UIComponent::load( data );
            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void InputField::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                setLoadingState( LoadingState::Unloading );

                m_textEntry = nullptr;

                UIComponent::unload( data );
                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void InputField::createUI()
    {
        try
        {
            auto element = getElement();
            if( !element )
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );

                auto renderUI = applicationManager->getRenderUIPtr();
                if( renderUI )
                {
                    auto textEntry = renderUI->addElementByType<ui::IUITextEntry>();
                    setTextEntry( textEntry );
                    setElement( textEntry );

                    updateElementState();
                    updateVisibility();
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<Properties> InputField::getProperties() const
    {
        if( auto properties = UIComponent::getProperties() )
        {
            properties->setProperty( textStr, m_text );
            properties->setProperty( placeholderStr, m_placeholder );
            properties->setProperty( readOnlyStr, m_readOnly );
            properties->setProperty( secureEntryStr, m_secureEntry );
            properties->setProperty( multilineStr, m_multiline );
            properties->setProperty( inputTypeStr, static_cast<s32>( m_inputType ) );
            properties->setProperty( textHintStr, m_textHint );
            properties->setProperty( textSizeStr, m_textSize );

            return properties;
        }

        return nullptr;
    }

    void InputField::setProperties( SmartPtr<Properties> properties )
    {
        if( properties )
        {
            properties->getPropertyValue( textStr, m_text );
            properties->getPropertyValue( placeholderStr, m_placeholder );
            properties->getPropertyValue( readOnlyStr, m_readOnly );
            properties->getPropertyValue( secureEntryStr, m_secureEntry );
            properties->getPropertyValue( multilineStr, m_multiline );

            s32 inputTypeValue = static_cast<s32>( m_inputType );
            properties->getPropertyValue( inputTypeStr, inputTypeValue );
            m_inputType = static_cast<ui::IUITextEntry::InputType>( inputTypeValue );

            properties->getPropertyValue( textHintStr, m_textHint );
            properties->getPropertyValue( textSizeStr, m_textSize );

            UIComponent::setProperties( properties );
            updateElementState();
        }
    }

    void InputField::updateElementState()
    {
        if( auto textEntry = getTextEntry() )
        {
            textEntry->setText( m_text );
            textEntry->setPlaceholder( m_placeholder );
            textEntry->setReadOnly( m_readOnly );
            textEntry->setSecureEntry( m_secureEntry );
            textEntry->setMultiline( m_multiline );
            textEntry->setInputType( m_inputType, m_textHint );
            textEntry->setTextSize( static_cast<f32>( m_textSize ) );
        }
    }

    SmartPtr<ui::IUITextEntry> InputField::getTextEntry() const
    {
        return m_textEntry;
    }

    void InputField::setTextEntry( SmartPtr<ui::IUITextEntry> textEntry )
    {
        m_textEntry = textEntry;
    }

    String InputField::getText() const
    {
        if( auto textEntry = getTextEntry() )
        {
            return textEntry->getText();
        }

        return m_text;
    }

    void InputField::setText( const String &text )
    {
        m_text = text;

        if( auto textEntry = getTextEntry() )
        {
            textEntry->setText( text );
        }
    }

    String InputField::getPlaceholder() const
    {
        return m_placeholder;
    }

    void InputField::setPlaceholder( const String &placeholder )
    {
        m_placeholder = placeholder;
        if( auto textEntry = getTextEntry() )
        {
            textEntry->setPlaceholder( placeholder );
        }
    }

    bool InputField::isReadOnly() const
    {
        return m_readOnly;
    }

    void InputField::setReadOnly( bool readOnly )
    {
        m_readOnly = readOnly;
        if( auto textEntry = getTextEntry() )
        {
            textEntry->setReadOnly( readOnly );
        }
    }

    bool InputField::isSecureEntry() const
    {
        return m_secureEntry;
    }

    void InputField::setSecureEntry( bool secureEntry )
    {
        m_secureEntry = secureEntry;
        if( auto textEntry = getTextEntry() )
        {
            textEntry->setSecureEntry( secureEntry );
        }
    }

    bool InputField::isMultiline() const
    {
        return m_multiline;
    }

    void InputField::setMultiline( bool multiline )
    {
        m_multiline = multiline;
        if( auto textEntry = getTextEntry() )
        {
            textEntry->setMultiline( multiline );
        }
    }

    ui::IUITextEntry::InputType InputField::getInputType() const
    {
        return m_inputType;
    }

    void InputField::setInputType( ui::IUITextEntry::InputType inputType, const String &textHint )
    {
        m_inputType = inputType;
        m_textHint = textHint;
        if( auto textEntry = getTextEntry() )
        {
            textEntry->setInputType( inputType, textHint );
        }
    }

    String InputField::getTextHint() const
    {
        return m_textHint;
    }

    u32 InputField::getTextSize() const
    {
        return m_textSize;
    }

    void InputField::setTextSize( u32 textSize )
    {
        m_textSize = textSize;
        if( auto textEntry = getTextEntry() )
        {
            textEntry->setTextSize( static_cast<f32>( textSize ) );
        }
    }

}  // namespace workphone::scene
