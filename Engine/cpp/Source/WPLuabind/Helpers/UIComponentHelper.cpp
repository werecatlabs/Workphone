#include <WPLuabind/WPLuabindPCH.hpp>
#include <WPLuabind/Helpers/UIComponentHelper.hpp>

namespace workphone
{
    void UIComponentHelper::buttonSetClickHandler( scene::Button *button, scene::IComponent *component,
                                                   const String &functionName )
    {
        if( !button || !component || functionName.empty() )
        {
            throw std::invalid_argument( "Button click handler requires a component and function" );
        }

        // Keep native event containers behind the binding. Lua owns the button;
        // the event listener holds only weak references to its target script.
        auto event = workphone::make_ptr<scene::ComponentEvent>();
        event->setEventHash( IEvent::CLICK_HASH );
        auto listener = workphone::make_ptr<scene::ComponentEventListener>();
        listener->setEvent( event );
        listener->setComponent( component );
        listener->setFunction( functionName );
        event->addListener( listener );
        // Replace the previous click handler while preserving hover events.
        for( const auto &existing : button->getEvents() )
        {
            if( existing && existing->getEventHash() == IEvent::CLICK_HASH )
            {
                button->removeEvent( existing );
            }
        }
        button->addEvent( event );
    }

    lua_Integer UIComponentHelper::getInputFieldType( const scene::InputField *inputField )
    {
        return static_cast<lua_Integer>( inputField->getInputType() );
    }

    void UIComponentHelper::setInputFieldType( scene::InputField *inputField, lua_Integer inputType )
    {
        inputField->setInputType( static_cast<ui::IUITextEntry::InputType>( inputType ) );
    }

    void UIComponentHelper::setInputFieldTypeWithHint( scene::InputField *inputField, lua_Integer inputType,
                                     const String &textHint )
    {
        inputField->setInputType( static_cast<ui::IUITextEntry::InputType>( inputType ), textHint );
    }

    lua_Integer UIComponentHelper::getPanelFlags( const scene::Layout *layout )
    {
        return static_cast<lua_Integer>( layout->getPanelFlags() );
    }

    void UIComponentHelper::setPanelFlags( scene::Layout *layout, lua_Integer flags )
    {
        layout->setPanelFlags( static_cast<PanelFlags>( flags ) );
    }

    bool UIComponentHelper::hasPanelFlag( const scene::Layout *layout, lua_Integer flag )
    {
        return layout->hasPanelFlag( static_cast<PanelFlags>( flag ) );
    }

    void UIComponentHelper::addPanelFlag( scene::Layout *layout, lua_Integer flag )
    {
        layout->addPanelFlag( static_cast<PanelFlags>( flag ) );
    }

    void UIComponentHelper::removePanelFlag( scene::Layout *layout, lua_Integer flag )
    {
        layout->removePanelFlag( static_cast<PanelFlags>( flag ) );
    }

    lua_Integer UIComponentHelper::getChildHorizontalAlignment( const scene::LayoutContainer *layout )
    {
        return static_cast<lua_Integer>( layout->getChildHorizontalAlignment() );
    }

    void UIComponentHelper::setChildHorizontalAlignment( scene::LayoutContainer *layout, lua_Integer alignment )
    {
        layout->setChildHorizontalAlignment( static_cast<HorizontalAlignment>( alignment ) );
    }

    lua_Integer UIComponentHelper::getChildVerticalAlignment( const scene::LayoutContainer *layout )
    {
        return static_cast<lua_Integer>( layout->getChildVerticalAlignment() );
    }

    void UIComponentHelper::setChildVerticalAlignment( scene::LayoutContainer *layout, lua_Integer alignment )
    {
        layout->setChildVerticalAlignment( static_cast<VerticalAlignment>( alignment ) );
    }

    lua_Integer UIComponentHelper::getLayoutHorizontalAlignment( const scene::LayoutTransform *transform )
    {
        return static_cast<lua_Integer>( transform->getHorizontalAlignment() );
    }

    void UIComponentHelper::setLayoutHorizontalAlignment( scene::LayoutTransform *transform, lua_Integer alignment )
    {
        transform->setHorizontalAlignment( static_cast<HorizontalAlignment>( alignment ) );
    }

    lua_Integer UIComponentHelper::getLayoutVerticalAlignment( const scene::LayoutTransform *transform )
    {
        return static_cast<lua_Integer>( transform->getVerticalAlignment() );
    }

    void UIComponentHelper::setLayoutVerticalAlignment( scene::LayoutTransform *transform, lua_Integer alignment )
    {
        transform->setVerticalAlignment( static_cast<VerticalAlignment>( alignment ) );
    }

    lua_Integer UIComponentHelper::getScrollBarDirection( const scene::ScrollBar *scrollBar )
    {
        return static_cast<lua_Integer>( scrollBar->getDirection() );
    }

    void UIComponentHelper::setScrollBarDirection( scene::ScrollBar *scrollBar, lua_Integer direction )
    {
        scrollBar->setDirection( static_cast<Direction>( direction ) );
    }

    lua_Integer UIComponentHelper::getComponentSliderDirection( const scene::Slider *slider )
    {
        return static_cast<lua_Integer>( slider->getDirection() );
    }

    void UIComponentHelper::setComponentSliderDirection( scene::Slider *slider, lua_Integer direction )
    {
        slider->setDirection( static_cast<Direction>( direction ) );
    }

    lua_Integer UIComponentHelper::getComponentToggleType( const scene::Toggle *toggle )
    {
        return static_cast<lua_Integer>( toggle->getToggleType() );
    }

    void UIComponentHelper::setComponentToggleType( scene::Toggle *toggle, lua_Integer type )
    {
        toggle->setToggleType( static_cast<scene::Toggle::ToggleType>( type ) );
    }

    lua_Integer UIComponentHelper::getComponentToggleState( const scene::Toggle *toggle )
    {
        return static_cast<lua_Integer>( toggle->getToggleState() );
    }

    void UIComponentHelper::setComponentToggleState( scene::Toggle *toggle, lua_Integer state )
    {
        toggle->setToggleState( static_cast<scene::Toggle::ToggleState>( state ) );
    }

    lua_Integer UIComponentHelper::getTabOrientation( const scene::TabView *tabView )
    {
        return static_cast<lua_Integer>( tabView->getOrientation() );
    }

    void UIComponentHelper::setTabOrientation( scene::TabView *tabView, lua_Integer orientation )
    {
        tabView->setOrientation( static_cast<scene::TabView::Orientation>( orientation ) );
    }

    lua_Integer UIComponentHelper::getTabPosition( const scene::TabView *tabView )
    {
        return static_cast<lua_Integer>( tabView->getTabPosition() );
    }

    void UIComponentHelper::setTabPosition( scene::TabView *tabView, lua_Integer position )
    {
        tabView->setTabPosition( static_cast<scene::TabView::TabPosition>( position ) );
    }

}  // namespace workphone
