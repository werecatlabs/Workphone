#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/UI/UIDropdown.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/State/States/UIDropdownStateData.hpp>

namespace workphone
{
    namespace ui
    {

        WP_CLASS_REGISTER_DERIVED( workphone::ui, UIDropdown, UIElement<IUIDropdown> );

        UIDropdown::UIDropdown() = default;

        UIDropdown::~UIDropdown() = default;

        void UIDropdown::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                UIElement::load( data );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void UIDropdown::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                UIElement::unload( data );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        Array<String> UIDropdown::getOptions() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<UIDropdownStateData>() )
                {
                    return { state->options.begin(), state->options.end() };
                }
            }

            return {};
        }

        void UIDropdown::setOptions( const Array<String> &options )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UIDropdownStateData>() )
                {
                    state->options = { options.begin(), options.end() };
                }
            }
        }

        void UIDropdown::addOption( const String &option )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UIDropdownStateData>() )
                {
                    state->options.push_back( option );
                }
            }
        }

        u32 UIDropdown::getSelectedOption() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<UIDropdownStateData>() )
                {
                    return state->selectedOption;
                }
            }

            return 0u;
        }

        void UIDropdown::setSelectedOption( u32 selectedOption )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<UIDropdownStateData>() )
                {
                    state->selectedOption = selectedOption;
                }
            }
        }

        void UIDropdown::invalidate()
        {
            if( auto stateContext = getStateContext() )
            {
                stateContext->setDirty( true );
            }
        }

    }  // namespace ui
}  // namespace workphone
