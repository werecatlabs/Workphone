#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiTextEntry.hpp>
#include <Workphone/Workphone.hpp>
#include <imgui.h>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiTextEntry, ImGuiElement<IUITextEntry> );

    ImGuiTextEntry::ImGuiTextEntry() = default;

    ImGuiTextEntry::~ImGuiTextEntry() = default;

    void ImGuiTextEntry::update()
    {
        auto name = getName();
        auto label = getLabel();
        auto guiLabel = label.empty() ? name : label;

        auto value = getText();
        if( StringUtil::isNullOrEmpty( value ) )
        {
            value = "";
        }

        constexpr int BUFFER_SIZE = 4096;
        char buffer[BUFFER_SIZE];
        StringUtil::toBuffer( value, buffer, BUFFER_SIZE );

        ImGuiInputTextFlags inputFlags = ImGuiInputTextFlags_None;
        if( isReadOnly() )
        {
            inputFlags |= ImGuiInputTextFlags_ReadOnly;
        }
        if( isSecureEntry() )
        {
            inputFlags |= ImGuiInputTextFlags_Password;
        }

        auto changed = false;
        if( isMultiline() )
        {
            changed = ImGui::InputTextMultiline( guiLabel.c_str(), buffer, BUFFER_SIZE, ImVec2( 0, 0 ),
                                                 inputFlags );
        }
        else if( auto placeholder = getPlaceholder(); !placeholder.empty() )
        {
            changed = ImGui::InputTextWithHint( guiLabel.c_str(), placeholder.c_str(), buffer,
                                                BUFFER_SIZE, inputFlags );
        }
        else
        {
            changed = ImGui::InputText( guiLabel.c_str(), buffer, BUFFER_SIZE, inputFlags );
        }

        if( changed )
        {
            String newText = buffer;
            setText( newText );

            auto listeners = getObjectListeners();
            for( auto &listener : listeners )
            {
                Array<Parameter> args;
                args.emplace_back( newText );
                listener->handleEvent( EventType::UI, IEvent::handleValueChanged, args, this, this,
                                       nullptr );
            }
        }

        auto dropTarget = getDropTarget();
        if( dropTarget )
        {
            if( ImGui::BeginDragDropTarget() )
            {
                auto payload = ImGui::AcceptDragDropPayload( "_TREENODE" );
                if( payload )
                {
                    auto pData = static_cast<const char *>( payload->Data );
                    auto dataSize = payload->DataSize;
                    auto data = String( pData, dataSize );

                    auto menuItemId = getElementId();

                    auto args = Array<Parameter>();
                    args.resize( 3 );

                    args[0].setStr( data );
                    args[1].setStr( name );
                    args[2].setStr( value );

                    dropTarget->handleEvent( EventType::UI, IEvent::handleDrop, args, this, this,
                                             nullptr );
                }

                ImGui::EndDragDropTarget();
            }
        }

        if( ImGui::IsKeyReleased( ImGuiKey_Enter ) )
        {
            auto listeners = getObjectListeners();
            for( auto &listener : listeners )
            {
                auto args = Array<Parameter>();
                args.resize( 2 );

                args[0].str = getText();

                listener->handleEvent( EventType::Object, IEvent::handlePropertyChanged, args, this,
                                       this, nullptr );
            }
        }
    }

    void ImGuiTextEntry::setText( const String &text )
    {
        m_text = text;
    }

    String ImGuiTextEntry::getText() const
    {
        return m_text;
    }

    void ImGuiTextEntry::setTextSize( f32 textSize )
    {
    }

    f32 ImGuiTextEntry::getTextSize() const
    {
        return 0.0f;
    }

    void ImGuiTextEntry::setVerticalAlignment( u8 alignment )
    {
    }

    u8 ImGuiTextEntry::getVerticalAlignment() const
    {
        return 0;
    }

    void ImGuiTextEntry::setHorizontalAlignment( u8 alignment )
    {
    }

    u8 ImGuiTextEntry::getHorizontalAlignment() const
    {
        return 0;
    }

    void ImGuiTextEntry::setPlaceholder( const String &placeholder )
    {
        m_placeholder = placeholder;
    }

    String ImGuiTextEntry::getPlaceholder() const
    {
        return m_placeholder;
    }

    void ImGuiTextEntry::setReadOnly( bool readOnly )
    {
        m_readOnly = readOnly;
    }

    bool ImGuiTextEntry::isReadOnly() const
    {
        return m_readOnly;
    }

    void ImGuiTextEntry::setSecureEntry( bool secureEntry )
    {
        m_secureEntry = secureEntry;
    }

    bool ImGuiTextEntry::isSecureEntry() const
    {
        return m_secureEntry;
    }

    void ImGuiTextEntry::setMultiline( bool multiline )
    {
        m_multiline = multiline;
    }

    bool ImGuiTextEntry::isMultiline() const
    {
        return m_multiline;
    }

    void ImGuiTextEntry::setInputType( InputType inputType, const String &textHint )
    {
        m_inputType = inputType;
        m_textHint = textHint;
    }

    ImGuiTextEntry::InputType ImGuiTextEntry::getInputType() const
    {
        return m_inputType;
    }

    String ImGuiTextEntry::getTextHint() const
    {
        return m_textHint;
    }
}  // namespace workphone::ui
