#include <WPLuabind/WPLuabindPCH.hpp>
#include <WPLuabind/Bindings/ComponentBind.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Scene/Components/UI/Button.hpp>
#include <Workphone/Scene/Components/ComponentEvent.hpp>
#include <Workphone/Scene/Components/ComponentEventListener.hpp>
#include <Workphone/Scene/Components/UI/Dropdown.hpp>
#include <Workphone/Scene/Components/UI/GridLayout.hpp>
#include <Workphone/Scene/Components/UI/HorizontalLayout.hpp>
#include <Workphone/Scene/Components/UI/Image.hpp>
#include <Workphone/Scene/Components/UI/InputField.hpp>
#include <Workphone/Scene/Components/UI/Layout.hpp>
#include <Workphone/Scene/Components/UI/LayoutContainer.hpp>
#include <Workphone/Scene/Components/UI/LayoutTransform.hpp>
#include <Workphone/Scene/Components/UI/ScrollBar.hpp>
#include <Workphone/Scene/Components/UI/ScrollView.hpp>
#include <Workphone/Scene/Components/UI/Slider.hpp>
#include <Workphone/Scene/Components/UI/TabItem.hpp>
#include <Workphone/Scene/Components/UI/TabPage.hpp>
#include <Workphone/Scene/Components/UI/TabView.hpp>
#include <Workphone/Scene/Components/UI/TableCell.hpp>
#include <Workphone/Scene/Components/UI/TableLayout.hpp>
#include <Workphone/Scene/Components/UI/Text.hpp>
#include <Workphone/Scene/Components/UI/Thumbnail.hpp>
#include <Workphone/Scene/Components/UI/Toggle.hpp>
#include <Workphone/Scene/Components/UI/ToggleGroup.hpp>
#include <Workphone/Scene/Components/UI/ToolTip.hpp>
#include <Workphone/Scene/Components/UI/UIComponent.hpp>
#include <Workphone/Scene/Components/UI/VerticalLayout.hpp>
#include <luabind/luabind.hpp>

namespace workphone
{
    void buttonSetClickHandler( scene::Button *button, scene::IComponent *component,
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

    lua_Integer _getInputFieldType( const scene::InputField *inputField )
    {
        return static_cast<lua_Integer>( inputField->getInputType() );
    }

    void _setInputFieldType( scene::InputField *inputField, lua_Integer inputType )
    {
        inputField->setInputType( static_cast<ui::IUITextEntry::InputType>( inputType ) );
    }

    void _setInputFieldTypeWithHint( scene::InputField *inputField, lua_Integer inputType,
                                     const String &textHint )
    {
        inputField->setInputType( static_cast<ui::IUITextEntry::InputType>( inputType ), textHint );
    }

    lua_Integer _getPanelFlags( const scene::Layout *layout )
    {
        return static_cast<lua_Integer>( layout->getPanelFlags() );
    }

    void _setPanelFlags( scene::Layout *layout, lua_Integer flags )
    {
        layout->setPanelFlags( static_cast<PanelFlags>( flags ) );
    }

    bool _hasPanelFlag( const scene::Layout *layout, lua_Integer flag )
    {
        return layout->hasPanelFlag( static_cast<PanelFlags>( flag ) );
    }

    void _addPanelFlag( scene::Layout *layout, lua_Integer flag )
    {
        layout->addPanelFlag( static_cast<PanelFlags>( flag ) );
    }

    void _removePanelFlag( scene::Layout *layout, lua_Integer flag )
    {
        layout->removePanelFlag( static_cast<PanelFlags>( flag ) );
    }

    lua_Integer _getChildHorizontalAlignment( const scene::LayoutContainer *layout )
    {
        return static_cast<lua_Integer>( layout->getChildHorizontalAlignment() );
    }

    void _setChildHorizontalAlignment( scene::LayoutContainer *layout, lua_Integer alignment )
    {
        layout->setChildHorizontalAlignment( static_cast<HorizontalAlignment>( alignment ) );
    }

    lua_Integer _getChildVerticalAlignment( const scene::LayoutContainer *layout )
    {
        return static_cast<lua_Integer>( layout->getChildVerticalAlignment() );
    }

    void _setChildVerticalAlignment( scene::LayoutContainer *layout, lua_Integer alignment )
    {
        layout->setChildVerticalAlignment( static_cast<VerticalAlignment>( alignment ) );
    }

    lua_Integer _getLayoutHorizontalAlignment( const scene::LayoutTransform *transform )
    {
        return static_cast<lua_Integer>( transform->getHorizontalAlignment() );
    }

    void _setLayoutHorizontalAlignment( scene::LayoutTransform *transform, lua_Integer alignment )
    {
        transform->setHorizontalAlignment( static_cast<HorizontalAlignment>( alignment ) );
    }

    lua_Integer _getLayoutVerticalAlignment( const scene::LayoutTransform *transform )
    {
        return static_cast<lua_Integer>( transform->getVerticalAlignment() );
    }

    void _setLayoutVerticalAlignment( scene::LayoutTransform *transform, lua_Integer alignment )
    {
        transform->setVerticalAlignment( static_cast<VerticalAlignment>( alignment ) );
    }

    lua_Integer _getScrollBarDirection( const scene::ScrollBar *scrollBar )
    {
        return static_cast<lua_Integer>( scrollBar->getDirection() );
    }

    void _setScrollBarDirection( scene::ScrollBar *scrollBar, lua_Integer direction )
    {
        scrollBar->setDirection( static_cast<Direction>( direction ) );
    }

    lua_Integer _getComponentSliderDirection( const scene::Slider *slider )
    {
        return static_cast<lua_Integer>( slider->getDirection() );
    }

    void _setComponentSliderDirection( scene::Slider *slider, lua_Integer direction )
    {
        slider->setDirection( static_cast<Direction>( direction ) );
    }

    lua_Integer _getComponentToggleType( const scene::Toggle *toggle )
    {
        return static_cast<lua_Integer>( toggle->getToggleType() );
    }

    void _setComponentToggleType( scene::Toggle *toggle, lua_Integer type )
    {
        toggle->setToggleType( static_cast<scene::Toggle::ToggleType>( type ) );
    }

    lua_Integer _getComponentToggleState( const scene::Toggle *toggle )
    {
        return static_cast<lua_Integer>( toggle->getToggleState() );
    }

    void _setComponentToggleState( scene::Toggle *toggle, lua_Integer state )
    {
        toggle->setToggleState( static_cast<scene::Toggle::ToggleState>( state ) );
    }

    lua_Integer _getTabOrientation( const scene::TabView *tabView )
    {
        return static_cast<lua_Integer>( tabView->getOrientation() );
    }

    void _setTabOrientation( scene::TabView *tabView, lua_Integer orientation )
    {
        tabView->setOrientation( static_cast<scene::TabView::Orientation>( orientation ) );
    }

    lua_Integer _getTabPosition( const scene::TabView *tabView )
    {
        return static_cast<lua_Integer>( tabView->getTabPosition() );
    }

    void _setTabPosition( scene::TabView *tabView, lua_Integer position )
    {
        tabView->setTabPosition( static_cast<scene::TabView::TabPosition>( position ) );
    }

    void bindComponentUI( lua_State *L )
    {
        using namespace scene;
        using namespace luabind;

        // UIComponent binding
        module( L )[class_<UIComponent, Component, SmartPtr<UIComponent>>( "UIComponent" )
                        .def( "load", &UIComponent::load )
                        .def( "unload", &UIComponent::unload )
                        .def( "getElementListener", &UIComponent::getElementListener )
                        .def( "setElementListener", &UIComponent::setElementListener )
                        .def( "getElementPtr", &UIComponent::getElementPtr )
                        .def( "getElement", &UIComponent::getElement )
                        .def( "setElement", &UIComponent::setElement )
                        .def( "getCanvasPtr", &UIComponent::getCanvasPtr )
                        .def( "getCanvas", &UIComponent::getCanvas )
                        .def( "setCanvas", &UIComponent::setCanvas )
                        .def( "updateDimensions", &UIComponent::updateDimensions )
                        .def( "updateMaterials", &UIComponent::updateMaterials )
                        .def( "getChildObjects", &UIComponent::getChildObjects )
                        .def( "getProperties", &UIComponent::getProperties )
                        .def( "setProperties", &UIComponent::setProperties )
                        .def( "updateFlags", &UIComponent::updateFlags )
                        .def( "updateTransform", &UIComponent::updateTransform )
                        .def( "getCascadeInput", &UIComponent::getCascadeInput )
                        .def( "setCascadeInput", &UIComponent::setCascadeInput )
                        .def( "getAutoCalculateOrder", &UIComponent::getAutoCalculateOrder )
                        .def( "setAutoCalculateOrder", &UIComponent::setAutoCalculateOrder )
                        .def( "updateOrder", &UIComponent::updateOrder )
                        .def( "getActorListeners", &UIComponent::getActorListeners )
                        .def( "getComponentListeners", &UIComponent::getComponentListeners )
                        .def( "addActorListener", &UIComponent::addActorListener )
                        .def( "removeActorListener", &UIComponent::removeActorListener )
                        .def( "addListener", &UIComponent::addListener )
                        .def( "removeListener", &UIComponent::removeListener )
                        .def( "getColour", &UIComponent::getColour )
                        .def( "setColour", &UIComponent::setColour )
                        .def( "getHandleInputEvents", &UIComponent::getHandleInputEvents )
                        .def( "setHandleInputEvents", &UIComponent::setHandleInputEvents )
                        .def( "getShowLabel", &UIComponent::getShowLabel )
                        .def( "setShowLabel", &UIComponent::setShowLabel )
                        .def( "getLabel", &UIComponent::getLabel )
                        .def( "setLabel", &UIComponent::setLabel )
                        .def( "getLabelActor", &UIComponent::getLabelActor )
                        .def( "setLabelActor", &UIComponent::setLabelActor )
                        .def( "getLayoutTransform", &UIComponent::getLayoutTransform )
                        .def( "setLayoutTransform", &UIComponent::setLayoutTransform )
                        .def( "getEditorWindowBorderSize", &UIComponent::getEditorWindowBorderSize )
                        .def( "setEditorWindowBorderSize", &UIComponent::setEditorWindowBorderSize )
                        .def( "updateVisibility", &UIComponent::updateVisibility )
                        .def( "updateElementState", &UIComponent::updateElementState )
                        .def( "updateColour", &UIComponent::updateColour )
                        .scope[def( "typeInfo", UIComponent::typeInfo )]];

        module( L )[class_<UIComponent::UIElementListener, IEventListener,
                           SmartPtr<UIComponent::UIElementListener>>( "UIComponentElementListener" )
                        .def( "getOwner", &UIComponent::UIElementListener::getOwner )
                        .def( "setOwner", &UIComponent::UIElementListener::setOwner )
                        .scope[def( "typeInfo", UIComponent::UIElementListener::typeInfo )]];

        module( L )[class_<Button, UIComponent, SmartPtr<Button>>( "UIButtonComponent" )
                        .def( "setClickHandler", buttonSetClickHandler )
                        .def( "getImage", &Button::getImage )
                        .def( "setImage", &Button::setImage )
                        .def( "getText", &Button::getText )
                        .def( "setText", &Button::setText )
                        .def( "getTextStr", &Button::getTextStr )
                        .def( "setTextStr", &Button::setTextStr )
                        .def( "getTextSize", &Button::getTextSize )
                        .def( "setTextSize", &Button::setTextSize )
                        .def( "getNormalColour", &Button::getNormalColour )
                        .def( "setNormalColour", &Button::setNormalColour )
                        .def( "getHighlightedColour", &Button::getHighlightedColour )
                        .def( "setHighlightedColour", &Button::setHighlightedColour )
                        .def( "getPressedColour", &Button::getPressedColour )
                        .def( "setPressedColour", &Button::setPressedColour )
                        .def( "getDisabledColour", &Button::getDisabledColour )
                        .def( "setDisabledColour", &Button::setDisabledColour )
                        .scope[def( "typeInfo", Button::typeInfo )]];

        module(
            L )[class_<Dropdown::Option, ISharedObject, SmartPtr<Dropdown::Option>>( "UIDropdownOption" )
                    .def( constructor<>() )
                    .def( constructor<const String &>() )
                    .def_readwrite( "imageTexture", &Dropdown::Option::imageTexture )
                    .def_readwrite( "text", &Dropdown::Option::text )
                    .scope[def( "typeInfo", Dropdown::Option::typeInfo )]];

        module( L )[class_<Dropdown, UIComponent, SmartPtr<Dropdown>>( "UIDropdownComponent" )
                        .def( "getButton", &Dropdown::getButton )
                        .def( "setButton", &Dropdown::setButton )
                        .def( "getPanel", &Dropdown::getPanel )
                        .def( "setPanel", &Dropdown::setPanel )
                        .def( "getContent", &Dropdown::getContent )
                        .def( "setContent", &Dropdown::setContent )
                        .def( "getOptionPrefab", &Dropdown::getOptionPrefab )
                        .def( "setOptionPrefab", &Dropdown::setOptionPrefab )
                        .def( "isOpen", &Dropdown::isOpen )
                        .def( "setOpen", &Dropdown::setOpen )
                        .def( "getOptions", &Dropdown::getOptions )
                        .def( "setOptions", &Dropdown::setOptions )
                        .def( "addOption", &Dropdown::addOption )
                        .def( "removeOption", &Dropdown::removeOption )
                        .def( "removeOptions", &Dropdown::removeOptions )
                        .def( "getPanelColour", &Dropdown::getPanelColour )
                        .def( "setPanelColour", &Dropdown::setPanelColour )
                        .def( "getOptionButtonColour", &Dropdown::getOptionButtonColour )
                        .def( "setOptionButtonColour", &Dropdown::setOptionButtonColour )
                        .def( "getContentOffset", &Dropdown::getContentOffset )
                        .def( "setContentOffset", &Dropdown::setContentOffset )
                        .def( "getContentHeight", &Dropdown::getContentHeight )
                        .def( "setContentHeight", &Dropdown::setContentHeight )
                        .def( "getPanelSize", &Dropdown::getPanelSize )
                        .def( "setPanelSize", &Dropdown::setPanelSize )
                        .def( "getContentZOrder", &Dropdown::getContentZOrder )
                        .def( "setContentZOrder", &Dropdown::setContentZOrder )
                        .def( "getContentName", &Dropdown::getContentName )
                        .def( "setContentName", &Dropdown::setContentName )
                        .def( "getPanelName", &Dropdown::getPanelName )
                        .def( "setPanelName", &Dropdown::setPanelName )
                        .def( "getScrollbarName", &Dropdown::getScrollbarName )
                        .def( "setScrollbarName", &Dropdown::setScrollbarName )
                        .def( "getScrollbarBackgroundName", &Dropdown::getScrollbarBackgroundName )
                        .def( "setScrollbarBackgroundName", &Dropdown::setScrollbarBackgroundName )
                        .def( "getScrollbarZOrderOffset", &Dropdown::getScrollbarZOrderOffset )
                        .def( "setScrollbarZOrderOffset", &Dropdown::setScrollbarZOrderOffset )
                        .def( "getInitialOptionCapacity", &Dropdown::getInitialOptionCapacity )
                        .def( "setInitialOptionCapacity", &Dropdown::setInitialOptionCapacity )
                        .scope[def( "typeInfo", Dropdown::typeInfo )]];

        module( L )[class_<LayoutContainer, Component, SmartPtr<LayoutContainer>>(
                        "UILayoutContainerComponent" )
                        .def( "getSpacing", &LayoutContainer::getSpacing )
                        .def( "setSpacing", &LayoutContainer::setSpacing )
                        .def( "getPadding", &LayoutContainer::getPadding )
                        .def( "setPadding", &LayoutContainer::setPadding )
                        .def( "getChildHorizontalAlignment", _getChildHorizontalAlignment )
                        .def( "setChildHorizontalAlignment", _setChildHorizontalAlignment )
                        .def( "getChildVerticalAlignment", _getChildVerticalAlignment )
                        .def( "setChildVerticalAlignment", _setChildVerticalAlignment )
                        .def( "getOffset", &LayoutContainer::getOffset )
                        .def( "setOffset", &LayoutContainer::setOffset )
                        .def( "getUseChildHorizontalAlignment",
                              &LayoutContainer::getUseChildHorizontalAlignment )
                        .def( "setUseChildHorizontalAlignment",
                              &LayoutContainer::setUseChildHorizontalAlignment )
                        .def( "getUseChildVerticalAlignment",
                              &LayoutContainer::getUseChildVerticalAlignment )
                        .def( "setUseChildVerticalAlignment",
                              &LayoutContainer::setUseChildVerticalAlignment )
                        .def( "getUseChildStartOffset", &LayoutContainer::getUseChildStartOffset )
                        .def( "setUseChildStartOffset", &LayoutContainer::setUseChildStartOffset )
                        .scope[def( "typeInfo", LayoutContainer::typeInfo )]];

        module( L )[class_<HorizontalLayout, LayoutContainer, SmartPtr<HorizontalLayout>>(
                        "UIHorizontalLayoutComponent" )
                        .scope[def( "typeInfo", HorizontalLayout::typeInfo )]];

        module( L )[class_<VerticalLayout, LayoutContainer, SmartPtr<VerticalLayout>>(
                        "UIVerticalLayoutComponent" )
                        .scope[def( "typeInfo", VerticalLayout::typeInfo )]];

        module( L )[class_<GridLayout, LayoutContainer, SmartPtr<GridLayout>>( "UIGridLayoutComponent" )
                        .def( "getColumnCount", &GridLayout::getColumnCount )
                        .def( "setColumnCount", &GridLayout::setColumnCount )
                        .def( "getCellSize", &GridLayout::getCellSize )
                        .def( "setCellSize", &GridLayout::setCellSize )
                        .def( "getSpacing", &GridLayout::getSpacing )
                        .def( "setSpacing", &GridLayout::setSpacing )
                        .def( "getModifyChildSize", &GridLayout::getModifyChildSize )
                        .def( "setModifyChildSize", &GridLayout::setModifyChildSize )
                        .scope[def( "typeInfo", GridLayout::typeInfo )]];

        module(
            L )[class_<TableLayout, LayoutContainer, SmartPtr<TableLayout>>( "UITableLayoutComponent" )
                    .def( "getNumRows", &TableLayout::getNumRows )
                    .def( "setNumRows", &TableLayout::setNumRows )
                    .def( "getNumColumns", &TableLayout::getNumColumns )
                    .def( "setNumColumns", &TableLayout::setNumColumns )
                    .def( "getCellWidth", &TableLayout::getCellWidth )
                    .def( "setCellWidth", &TableLayout::setCellWidth )
                    .def( "getCellHeight", &TableLayout::getCellHeight )
                    .def( "setCellHeight", &TableLayout::setCellHeight )
                    .def( "getResizeContent", &TableLayout::getResizeContent )
                    .def( "setResizeContent", &TableLayout::setResizeContent )
                    .def( "getCellNamePrefix", &TableLayout::getCellNamePrefix )
                    .def( "setCellNamePrefix", &TableLayout::setCellNamePrefix )
                    .def( "getCellNameSeparator", &TableLayout::getCellNameSeparator )
                    .def( "setCellNameSeparator", &TableLayout::setCellNameSeparator )
                    .scope[def( "typeInfo", TableLayout::typeInfo )]];

        module( L )[class_<LayoutTransform, Component, SmartPtr<LayoutTransform>>(
                        "UILayoutTransformComponent" )
                        .def( "getMin", &LayoutTransform::getMin )
                        .def( "getAbsoluteMin", &LayoutTransform::getAbsoluteMin )
                        .def( "getMax", &LayoutTransform::getMax )
                        .def( "getAbsoluteMax", &LayoutTransform::getAbsoluteMax )
                        .def( "getPosition", &LayoutTransform::getPosition )
                        .def( "setPosition", &LayoutTransform::setPosition )
                        .def( "getSize", &LayoutTransform::getSize )
                        .def( "setSize", &LayoutTransform::setSize )
                        .def( "getAnchor", &LayoutTransform::getAnchor )
                        .def( "setAnchor", &LayoutTransform::setAnchor )
                        .def( "getAnchorMin", &LayoutTransform::getAnchorMin )
                        .def( "setAnchorMin", &LayoutTransform::setAnchorMin )
                        .def( "getAnchorMax", &LayoutTransform::getAnchorMax )
                        .def( "setAnchorMax", &LayoutTransform::setAnchorMax )
                        .def( "updateAnchorFromAlignment", &LayoutTransform::updateAnchorFromAlignment )
                        .def( "getZOrder", &LayoutTransform::getZOrder )
                        .def( "setZOrder", &LayoutTransform::setZOrder )
                        .def( "getAutoCalculateOrder", &LayoutTransform::getAutoCalculateOrder )
                        .def( "setAutoCalculateOrder", &LayoutTransform::setAutoCalculateOrder )
                        .def( "setHorizontalAlignment", _setLayoutHorizontalAlignment )
                        .def( "getHorizontalAlignment", _getLayoutHorizontalAlignment )
                        .def( "setVerticalAlignment", _setLayoutVerticalAlignment )
                        .def( "getVerticalAlignment", _getLayoutVerticalAlignment )
                        .def( "getAbsolutePosition", &LayoutTransform::getAbsolutePosition )
                        .def( "setAbsolutePosition", &LayoutTransform::setAbsolutePosition )
                        .def( "getAbsoluteSize", &LayoutTransform::getAbsoluteSize )
                        .def( "setAbsoluteSize", &LayoutTransform::setAbsoluteSize )
                        .def( "getRelativePosition", &LayoutTransform::getRelativePosition )
                        .def( "getRelativeSize", &LayoutTransform::getRelativeSize )
                        .def( "getLayoutPtr", &LayoutTransform::getLayoutPtr )
                        .def( "getLayout", &LayoutTransform::getLayout )
                        .def( "setLayout", &LayoutTransform::setLayout )
                        .def( "getUIComponent", &LayoutTransform::getUIComponent )
                        .def( "setUIComponent", &LayoutTransform::setUIComponent )
                        .scope[def( "typeInfo", LayoutTransform::typeInfo )]];

        module( L )[class_<scene::Image, UIComponent, SmartPtr<scene::Image>>( "UIImageComponent" )
                        .def( "getImage", &scene::Image::getImage )
                        .def( "setImage", &scene::Image::setImage )
                        .def( "getTexture", &scene::Image::getTexture )
                        .def( "setTexture", &scene::Image::setTexture )
                        .def( "getTextureName", &scene::Image::getTextureName )
                        .def( "setTextureName", &scene::Image::setTextureName )
                        .def( "getUseTiling", &scene::Image::getUseTiling )
                        .def( "setUseTiling", &scene::Image::setUseTiling )
                        .def( "getBaseMaterialName", &scene::Image::getBaseMaterialName )
                        .def( "setBaseMaterialName", &scene::Image::setBaseMaterialName )
                        .scope[def( "typeInfo", scene::Image::typeInfo )]];

        module( L )[class_<Text, UIComponent, SmartPtr<Text>>( "UITextComponent" )
                        .def( "getTextObject", &Text::getTextObject )
                        .def( "setTextObject", &Text::setTextObject )
                        .def( "getText", &Text::getText )
                        .def( "setText", &Text::setText )
                        .def( "getVerticalAlignment", &Text::getVerticalAlignment )
                        .def( "setVerticalAlignment", &Text::setVerticalAlignment )
                        .def( "getHorizontalAlignment", &Text::getHorizontalAlignment )
                        .def( "setHorizontalAlignment", &Text::setHorizontalAlignment )
                        .scope[def( "typeInfo", Text::typeInfo )]];

        module( L )[class_<InputField, UIComponent, SmartPtr<InputField>>( "UIInputFieldComponent" )
                        .def( "getTextEntry", &InputField::getTextEntry )
                        .def( "setTextEntry", &InputField::setTextEntry )
                        .def( "getText", &InputField::getText )
                        .def( "setText", &InputField::setText )
                        .def( "getPlaceholder", &InputField::getPlaceholder )
                        .def( "setPlaceholder", &InputField::setPlaceholder )
                        .def( "isReadOnly", &InputField::isReadOnly )
                        .def( "setReadOnly", &InputField::setReadOnly )
                        .def( "isSecureEntry", &InputField::isSecureEntry )
                        .def( "setSecureEntry", &InputField::setSecureEntry )
                        .def( "isMultiline", &InputField::isMultiline )
                        .def( "setMultiline", &InputField::setMultiline )
                        .def( "getInputType", _getInputFieldType )
                        .def( "setInputType", _setInputFieldType )
                        .def( "setInputType", _setInputFieldTypeWithHint )
                        .def( "getTextHint", &InputField::getTextHint )
                        .def( "getTextSize", &InputField::getTextSize )
                        .def( "setTextSize", &InputField::setTextSize )
                        .scope[def( "typeInfo", InputField::typeInfo )]];

        module( L )[class_<Toggle, UIComponent, SmartPtr<Toggle>>( "UIToggleComponent" )
                        .def( "isToggled", &Toggle::isToggled )
                        .def( "setToggled", &Toggle::setToggled )
                        .def( "getToggleBgTransform", &Toggle::getToggleBgTransform )
                        .def( "setToggleBgTransform", &Toggle::setToggleBgTransform )
                        .def( "getToggleTransform", &Toggle::getToggleTransform )
                        .def( "setToggleTransform", &Toggle::setToggleTransform )
                        .def( "getToggledColour", &Toggle::getToggledColour )
                        .def( "setToggledColour", &Toggle::setToggledColour )
                        .def( "getUntoggledColour", &Toggle::getUntoggledColour )
                        .def( "setUntoggledColour", &Toggle::setUntoggledColour )
                        .def( "getLabel", &Toggle::getLabel )
                        .def( "setLabel", &Toggle::setLabel )
                        .def( "getTextSize", &Toggle::getTextSize )
                        .def( "setTextSize", &Toggle::setTextSize )
                        .def( "getShowLabel", &Toggle::getShowLabel )
                        .def( "setShowLabel", &Toggle::setShowLabel )
                        .def( "getToggleType", _getComponentToggleType )
                        .def( "setToggleType", _setComponentToggleType )
                        .def( "getToggleState", _getComponentToggleState )
                        .def( "setToggleState", _setComponentToggleState )
                        .def( "getToggledPositionFactor", &Toggle::getToggledPositionFactor )
                        .def( "setToggledPositionFactor", &Toggle::setToggledPositionFactor )
                        .def( "getUntoggledPositionFactor", &Toggle::getUntoggledPositionFactor )
                        .def( "setUntoggledPositionFactor", &Toggle::setUntoggledPositionFactor )
                        .scope[def( "typeInfo", Toggle::typeInfo )]];

        module( L )[class_<ToggleGroup, UIComponent, SmartPtr<ToggleGroup>>( "UIToggleGroupComponent" )
                        .scope[def( "typeInfo", ToggleGroup::typeInfo )]];

        module( L )[class_<Slider, UIComponent, SmartPtr<Slider>>( "UISliderComponent" )
                        .def( "getHandleActor", &Slider::getHandleActor )
                        .def( "setHandleActor", &Slider::setHandleActor )
                        .def( "getBackground", &Slider::getBackground )
                        .def( "setBackground", &Slider::setBackground )
                        .def( "getFill", &Slider::getFill )
                        .def( "setFill", &Slider::setFill )
                        .def( "getValue", &Slider::getValue )
                        .def( "setValue", &Slider::setValue )
                        .def( "getMinValue", &Slider::getMinValue )
                        .def( "setMinValue", &Slider::setMinValue )
                        .def( "getMaxValue", &Slider::getMaxValue )
                        .def( "setMaxValue", &Slider::setMaxValue )
                        .def( "getDirection", _getComponentSliderDirection )
                        .def( "setDirection", _setComponentSliderDirection )
                        .def( "isDragging", &Slider::isDragging )
                        .def( "setDragging", &Slider::setDragging )
                        .scope[def( "typeInfo", Slider::typeInfo )]];

        module( L )[class_<ScrollBar, UIComponent, SmartPtr<ScrollBar>>( "UIScrollBarComponent" )
                        .def( "getHandleActor", &ScrollBar::getHandleActor )
                        .def( "setHandleActor", &ScrollBar::setHandleActor )
                        .def( "getBackground", &ScrollBar::getBackground )
                        .def( "setBackground", &ScrollBar::setBackground )
                        .def( "getFill", &ScrollBar::getFill )
                        .def( "setFill", &ScrollBar::setFill )
                        .def( "getScrollValue", &ScrollBar::getScrollValue )
                        .def( "setScrollValue", &ScrollBar::setScrollValue )
                        .def( "getHandleSize", &ScrollBar::getHandleSize )
                        .def( "setHandleSize", &ScrollBar::setHandleSize )
                        .def( "getDirection", _getScrollBarDirection )
                        .def( "setDirection", _setScrollBarDirection )
                        .def( "getScrollView", &ScrollBar::getScrollView )
                        .def( "setScrollView", &ScrollBar::setScrollView )
                        .scope[def( "typeInfo", ScrollBar::typeInfo )]];

        module( L )[class_<ScrollView, UIComponent, SmartPtr<ScrollView>>( "UIScrollViewComponent" )
                        .def( "getContentPanel", &ScrollView::getContentPanel )
                        .def( "setContentPanel", &ScrollView::setContentPanel )
                        .def( "getScrollSpeed", &ScrollView::getScrollSpeed )
                        .def( "setScrollSpeed", &ScrollView::setScrollSpeed )
                        .def( "getInertia", &ScrollView::getInertia )
                        .def( "setInertia", &ScrollView::setInertia )
                        .def( "getLastDragPosition", &ScrollView::getLastDragPosition )
                        .def( "setLastDragPosition", &ScrollView::setLastDragPosition )
                        .def( "getVelocity", &ScrollView::getVelocity )
                        .def( "setVelocity", &ScrollView::setVelocity )
                        .def( "isDragging", &ScrollView::isDragging )
                        .def( "setDragging", &ScrollView::setDragging )
                        .def( "getScrollBar", &ScrollView::getScrollBar )
                        .def( "setScrollBar", &ScrollView::setScrollBar )
                        .def( "updateScrollBar", &ScrollView::updateScrollBar )
                        .def( "syncWithScrollBar", &ScrollView::syncWithScrollBar )
                        .scope[def( "typeInfo", ScrollView::typeInfo )]];

        module( L )[class_<Layout, UIComponent, SmartPtr<Layout>>( "UILayoutComponent" )
                        .def( "getLayout", &Layout::getLayout )
                        .def( "setLayout", &Layout::setLayout )
                        .def( "getReferenceSize", &Layout::getReferenceSize )
                        .def( "setReferenceSize", &Layout::setReferenceSize )
                        .def( "getElementOrder", &Layout::getElementOrder )
                        .def( "getElementOrderReversed", &Layout::getElementOrderReversed )
                        .def( "getZOrder", &Layout::getZOrder )
                        .def( "getPanelFlags", _getPanelFlags )
                        .def( "setPanelFlags", _setPanelFlags )
                        .def( "hasPanelFlag", _hasPanelFlag )
                        .def( "addPanelFlag", _addPanelFlag )
                        .def( "removePanelFlag", _removePanelFlag )
                        .scope[def( "typeInfo", Layout::typeInfo )]];

        module(
            L )[class_<TableCell, UIComponent, SmartPtr<TableCell>>( "UITableCellComponent" )
                    .def( "getTableLayout", &TableCell::getTableLayout )
                    .def( "setTableLayout", &TableCell::setTableLayout )
                    .def( "getResizeChildLayoutTransforms", &TableCell::getResizeChildLayoutTransforms )
                    .def( "setResizeChildLayoutTransforms", &TableCell::setResizeChildLayoutTransforms )
                    .def( "getUpdateChildLayoutTransforms", &TableCell::getUpdateChildLayoutTransforms )
                    .def( "setUpdateChildLayoutTransforms", &TableCell::setUpdateChildLayoutTransforms )
                    .scope[def( "typeInfo", TableCell::typeInfo )]];

        module( L )[class_<TabItem, UIComponent, SmartPtr<TabItem>>( "UITabItemComponent" )
                        .def( "getLabel", &TabItem::getLabel )
                        .def( "setLabel", &TabItem::setLabel )
                        .def( "getContent", &TabItem::getContent )
                        .def( "setContent", &TabItem::setContent )
                        .scope[def( "typeInfo", TabItem::typeInfo )]];

        module( L )[class_<TabPage, UIComponent, SmartPtr<TabPage>>( "UITabPageComponent" )
                        .def( "addTabItem", &TabPage::addTabItem )
                        .def( "removeTabItem",
                              static_cast<void ( TabPage::* )( u32 )>( &TabPage::removeTabItem ) )
                        .def( "removeTabItem", static_cast<void ( TabPage::* )( SmartPtr<TabItem> )>(
                                                   &TabPage::removeTabItem ) )
                        .def( "getTabItem", &TabPage::getTabItem )
                        .def( "getTabItems", &TabPage::getTabItems )
                        .def( "getTabCount", &TabPage::getTabCount )
                        .def( "getActiveTabIndex", &TabPage::getActiveTabIndex )
                        .def( "setActiveTabIndex", &TabPage::setActiveTabIndex )
                        .def( "getActiveTab", &TabPage::getActiveTab )
                        .scope[def( "typeInfo", TabPage::typeInfo )]];
        module( L )[class_<TabView, UIComponent, SmartPtr<TabView>>( "UITabViewComponent" )
                        .def( "addTabPage", &TabView::addTabPage )
                        .def( "removeTabPage",
                              static_cast<void ( TabView::* )( u32 )>( &TabView::removeTabPage ) )
                        .def( "removeTabPage", static_cast<void ( TabView::* )( SmartPtr<TabPage> )>(
                                                   &TabView::removeTabPage ) )
                        .def( "getTabPage", &TabView::getTabPage )
                        .def( "getTabPages", &TabView::getTabPages )
                        .def( "getTabPageCount", &TabView::getTabPageCount )
                        .def( "setOrientation", _setTabOrientation )
                        .def( "getOrientation", _getTabOrientation )
                        .def( "setTabPosition", _setTabPosition )
                        .def( "getTabPosition", _getTabPosition )
                        .def( "getTabSize", &TabView::getTabSize )
                        .def( "setTabSize", &TabView::setTabSize )
                        .def( "getShowTabHeaders", &TabView::getShowTabHeaders )
                        .def( "setShowTabHeaders", &TabView::setShowTabHeaders )
                        .scope[def( "typeInfo", TabView::typeInfo )]];

        module( L )[class_<Thumbnail, UIComponent, SmartPtr<Thumbnail>>( "UIThumbnailComponent" )
                        .def( "getThumb", &Thumbnail::getThumb )
                        .def( "setThumb", &Thumbnail::setThumb )
                        .def( "getLabelText", &Thumbnail::getLabelText )
                        .def( "setLabelText", &Thumbnail::setLabelText )
                        .def( "getHighlightObject", &Thumbnail::getHighlightObject )
                        .def( "setHighlightObject", &Thumbnail::setHighlightObject )
                        .def( "getHighlightImage", &Thumbnail::getHighlightImage )
                        .def( "setHighlightImage", &Thumbnail::setHighlightImage )
                        .def( "getHighlightColor", &Thumbnail::getHighlightColor )
                        .def( "setHighlightColor", &Thumbnail::setHighlightColor )
                        .def( "getNormalColor", &Thumbnail::getNormalColor )
                        .def( "setNormalColor", &Thumbnail::setNormalColor )
                        .scope[def( "typeInfo", Thumbnail::typeInfo )]];

        module( L )[class_<ToolTip, Component, SmartPtr<ToolTip>>( "UIToolTipComponent" )
                        .def( "getPosition", &ToolTip::getPosition )
                        .def( "setPosition", &ToolTip::setPosition )
                        .def( "getMouseOverTime", &ToolTip::getMouseOverTime )
                        .def( "setMouseOverTime", &ToolTip::setMouseOverTime )
                        .def( "isMouseOver", &ToolTip::isMouseOver )
                        .def( "setMouseOver", &ToolTip::setMouseOver )
                        .def( "getTooltip", &ToolTip::getTooltip )
                        .def( "setTooltip", &ToolTip::setTooltip )
                        .scope[def( "typeInfo", ToolTip::typeInfo )]];
    }
} // namespace workphone
