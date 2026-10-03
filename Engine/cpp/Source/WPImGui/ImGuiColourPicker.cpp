#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiColourPicker.hpp>
#include "ImCurveEdit.hpp"
#include <Workphone/Workphone.hpp>
#include <imgui.h>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiColourPicker, ImGuiElement<IUIColourPicker> );

    ImGuiColourPicker::ImGuiColourPicker() = default;

    ImGuiColourPicker::~ImGuiColourPicker() = default;

    void ImGuiColourPicker::update()
    {
        auto colour = getColour();
        auto name = getName();
        auto label = getLabel();

        auto guiLabel = label.empty() ? name : label;

        if( ImGui::ColorEdit3( guiLabel.c_str(), &colour.r ) )
        {
            setColour( colour );

            if( auto parent = getParent() )
            {
                if( parent->isDerived<IUIToolbar>() )
                {
                    auto toolbar = workphone::static_pointer_cast<IUIToolbar>( parent );
                    WP_ASSERT( toolbar );

                    auto listeners = toolbar->getObjectListeners();
                    for( auto listener : listeners )
                    {
                        auto args = Array<Parameter>();
                        args.resize( 3 );

                        args[0] = Parameter( colour.r );
                        args[1] = Parameter( colour.g );
                        args[2] = Parameter( colour.b );

                        listener->handleEvent( EventType::UI, IEvent::handleValueChanged, args,
                                               toolbar, this, nullptr );
                    }
                }
                else
                {
                    auto listeners = getObjectListeners();
                    for( auto listener : listeners )
                    {
                        if( listener )
                        {
                            auto args = Array<Parameter>();
                            args.resize( 3 );

                            args[0] = Parameter( colour.r );
                            args[1] = Parameter( colour.g );
                            args[2] = Parameter( colour.b );

                            listener->handleEvent( EventType::UI, IEvent::handleValueChanged, args,
                                                   this, this, nullptr );
                        }
                    }
                }
            }
        }
    }

    void ImGuiColourPicker::setGradient( const ColourF &startColour, const ColourF &endColour )
    {
        ScopedLock lock( this );
        m_gradient.first = startColour;
        m_gradient.second = endColour;
    }

    Pair<ColourF, ColourF> ImGuiColourPicker::getGradient() const
    {
        ScopedLock lock( this );
        return m_gradient;
    }

    void ImGuiColourPicker::setColourFormat( ColourFormat format )
    {
        ScopedLock lock( this );
        m_colourFormat = format;
    }

    ImGuiColourPicker::ColourFormat ImGuiColourPicker::getColourFormat() const
    {
        ScopedLock lock( this );
        return m_colourFormat;
    }

    void ImGuiColourPicker::setLabel( const String &label )
    {
        ScopedLock lock( this );
        m_label = label;
    }

    String ImGuiColourPicker::getLabel() const
    {
        ScopedLock lock( this );
        return m_label;
    }
}  // namespace workphone::ui
