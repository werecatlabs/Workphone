#include <WPLuabind/WPLuabindPCH.hpp>
#include <WPLuabind/Bindings/ComponentBind.hpp>
#include <WPLuabind/Helpers/UIComponentHelper.hpp>
#include <WPLuabind/Helpers/UiUtilHelper.hpp>
#include <WPLuabind/SmartPtrConverter.hpp>
#include <WPLuabind/ParamConverter.hpp>
#include <WPLuabind/StringConverter.hpp>
#include <Workphone/Workphone.hpp>
#include <luabind/luabind.hpp>

namespace workphone
{
    namespace
    {
        // Lua's SmartPtr converter does not accept nil when matching overloads.
        // The director is unused by these factories, so omit it from simple calls.
        SmartPtr<scene::IGameActor> createSliderWithoutDirector( const String &label, bool addToScene )
        {
            return scene::UiUtil::createSlider( label, nullptr, "", addToScene );
        }

        SmartPtr<scene::IGameActor> createSliderVerticalWithoutDirector( const String &label,
                                                                     bool addToScene )
        {
            return scene::UiUtil::createSliderVertical( label, nullptr, "", addToScene );
        }
    }  // namespace

    void bindComponentUI( lua_State *L )
    {
        using namespace scene;
        using namespace luabind;

        /*module( L )[class_<HorizontalAlignment>( "HorizontalAlignment" )
        .enum_<u32>( "HorizontalAlignment" )
            .value( "LEFT", HorizontalAlignment::LEFT )
            .value( "CENTER", HorizontalAlignment::CENTER )
            .value( "RIGHT", HorizontalAlignment::RIGHT )
            .value( "CUSTOM", HorizontalAlignment::CUSTOM );*/

        module(
            L )[class_<UiUtil>( "UiUtil" )
                    .scope[def( "getHorizontalAlignmentString", &UiUtilHelper::getHorizontalAlignmentString ),
                           def( "getHorizontalAlignment", &UiUtilHelper::getHorizontalAlignment ),
                           def( "getHorizontalAlignmentTypesString",
                                &UiUtil::getHorizontalAlignmentTypesString ),
                           def( "getVerticalAlignmentString", &UiUtilHelper::getVerticalAlignmentString ),
                           def( "getVerticalAlignment", &UiUtilHelper::getVerticalAlignment ),
                           def( "getVerticalAlignmentTypesString",
                                &UiUtil::getVerticalAlignmentTypesString ),
                           def( "createText", &UiUtil::createText ),
                           def( "createButton", &UiUtil::createButton ),
                           def( "createButtonEx", &UiUtil::createButtonEx ),
                           def( "setRect", &UiUtil::setRect ), 
                           def( "setAlignment", &UiUtilHelper::setAlignment ),
                           def( "loadHierarchy", &UiUtil::loadHierarchy ),
                           def( "createDropdown", &UiUtil::createDropdown ),
                           def( "createDropdownWithOptions", &UiUtilHelper::createDropdownWithOptions ),
                           def( "createImage", &UiUtil::createImage ),
                           def( "createInputField", &UiUtil::createInputField ),
                           def( "createToggle", &UiUtil::createToggle ),
                           def( "createLayout", &UiUtil::createLayout ),
                           def( "createHorizontalLayout", &UiUtil::createHorizontalLayout ),
                           def( "createVerticalLayout", &UiUtil::createVerticalLayout ),
                           def( "createGridLayout", &UiUtil::createGridLayout ),
                           def( "createTableLayout", &UiUtil::createTableLayout ),
                           def( "createTableCell", &UiUtil::createTableCell ),
                           def( "createTabView", &UiUtil::createTabView ),
                           def( "createTabPage", &UiUtil::createTabPage ),
                           def( "createTabItem", &UiUtil::createTabItem ),
                           def( "createToggleGroup", &UiUtil::createToggleGroup ),
                           def( "createToolTip", &UiUtil::createToolTip ),
                           def( "createThumbnail", &UiUtil::createThumbnail ),
                           def( "createScrollView", &UiUtil::createScrollView ),
                           def( "createPanel", &UiUtil::createPanel ),
                           def( "createSlider", &UiUtil::createSlider ),
                           def( "createSlider", &createSliderWithoutDirector ),
                           def( "createSliderVertical", &UiUtil::createSliderVertical ),
                           def( "createSliderVertical", &createSliderVerticalWithoutDirector ),
                           def( "createScrollbar", &UiUtil::createScrollbar ),
                           def( "createScrollbarVertical", &UiUtil::createScrollbarVertical ),
                           def( "createScrollview", &UiUtil::createScrollview )]];

        object uiUtil = globals( L )["UiUtil"];
        uiUtil["directionNames"] = UiUtilHelper::stringTable( L, UiUtil::directionNames );
        uiUtil["verticalAlignmentTypes"] = UiUtilHelper::stringTable( L, UiUtil::verticalAlignmentTypes );
        uiUtil["horizontalAlignmentTypes"] = UiUtilHelper::stringTable( L, UiUtil::horizontalAlignmentTypes );
        uiUtil["directionStr"] = UiUtil::directionStr;
        uiUtil["handleStr"] = UiUtil::handleStr;
        uiUtil["backgroundStr"] = UiUtil::backgroundStr;
        uiUtil["fillStr"] = UiUtil::fillStr;
        uiUtil["scrollValueStr"] = UiUtil::scrollValueStr;
        uiUtil["sliderValueStr"] = UiUtil::sliderValueStr;
        uiUtil["topStr"] = UiUtil::topStr;
        uiUtil["centerStr"] = UiUtil::centerStr;
        uiUtil["bottomStr"] = UiUtil::bottomStr;
        uiUtil["leftStr"] = UiUtil::leftStr;
        uiUtil["rightStr"] = UiUtil::rightStr;
        uiUtil["customStr"] = UiUtil::customStr;
        uiUtil["dropDownNameStr"] = UiUtil::dropDownNameStr;
        uiUtil["optionStr"] = UiUtil::optionStr;
        uiUtil["panelStr"] = UiUtil::panelStr;

        // UIComponent binding
        module( L )[class_<UIComponent, Component, SmartPtr<UIComponent>>( "UIComponent" )
                        .def( "getUseNativeUI", &UIComponent::getUseNativeUI )
                        .def( "setUseNativeUI", &UIComponent::setUseNativeUI )
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
                        .def( "setClickHandler", &UIComponentHelper::buttonSetClickHandler )
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
                        .def( "getChildHorizontalAlignment", &UIComponentHelper::getChildHorizontalAlignment )
                        .def( "setChildHorizontalAlignment", &UIComponentHelper::setChildHorizontalAlignment )
                        .def( "getChildVerticalAlignment", &UIComponentHelper::getChildVerticalAlignment )
                        .def( "setChildVerticalAlignment", &UIComponentHelper::setChildVerticalAlignment )
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

        module( L )[class_<GridLayout, LayoutContainer, SmartPtr<GridLayout>>( "GridLayout" )
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
            L )[class_<TableLayout, LayoutContainer, SmartPtr<TableLayout>>( "TableLayout" )
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
                        "LayoutTransform" )
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
                        .def( "setHorizontalAlignment", &UIComponentHelper::setLayoutHorizontalAlignment )
                        .def( "getHorizontalAlignment", &UIComponentHelper::getLayoutHorizontalAlignment )
                        .def( "setVerticalAlignment", &UIComponentHelper::setLayoutVerticalAlignment )
                        .def( "getVerticalAlignment", &UIComponentHelper::getLayoutVerticalAlignment )
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

        module( L )[class_<scene::Image, UIComponent, SmartPtr<scene::Image>>( "Image" )
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

        module( L )[class_<Text, UIComponent, SmartPtr<Text>>( "Text" )
                        .def( "getTextObject", &Text::getTextObject )
                        .def( "setTextObject", &Text::setTextObject )
                        .def( "getText", &Text::getText )
                        .def( "setText", &Text::setText )
                        .def( "getVerticalAlignment", &Text::getVerticalAlignment )
                        .def( "setVerticalAlignment", &Text::setVerticalAlignment )
                        .def( "getHorizontalAlignment", &Text::getHorizontalAlignment )
                        .def( "setHorizontalAlignment", &Text::setHorizontalAlignment )
                        .scope[def( "typeInfo", Text::typeInfo )]];

        module( L )[class_<InputField, UIComponent, SmartPtr<InputField>>( "InputField" )
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
                        .def( "getInputType", &UIComponentHelper::getInputFieldType )
                        .def( "setInputType", &UIComponentHelper::setInputFieldType )
                        .def( "setInputType", &UIComponentHelper::setInputFieldTypeWithHint )
                        .def( "getTextHint", &InputField::getTextHint )
                        .def( "getTextSize", &InputField::getTextSize )
                        .def( "setTextSize", &InputField::setTextSize )
                        .scope[def( "typeInfo", InputField::typeInfo )]];

        module( L )[class_<Toggle, UIComponent, SmartPtr<Toggle>>( "Toggle" )
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
                        .def( "getToggleType", &UIComponentHelper::getComponentToggleType )
                        .def( "setToggleType", &UIComponentHelper::setComponentToggleType )
                        .def( "getToggleState", &UIComponentHelper::getComponentToggleState )
                        .def( "setToggleState", &UIComponentHelper::setComponentToggleState )
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
                        .def( "getDirection", &UIComponentHelper::getComponentSliderDirection )
                        .def( "setDirection", &UIComponentHelper::setComponentSliderDirection )
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
                        .def( "getDirection", &UIComponentHelper::getScrollBarDirection )
                        .def( "setDirection", &UIComponentHelper::setScrollBarDirection )
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
                        .def( "getPanelFlags", &UIComponentHelper::getPanelFlags )
                        .def( "setPanelFlags", &UIComponentHelper::setPanelFlags )
                        .def( "hasPanelFlag", &UIComponentHelper::hasPanelFlag )
                        .def( "addPanelFlag", &UIComponentHelper::addPanelFlag )
                        .def( "removePanelFlag", &UIComponentHelper::removePanelFlag )
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
                        .def( "setOrientation", &UIComponentHelper::setTabOrientation )
                        .def( "getOrientation", &UIComponentHelper::getTabOrientation )
                        .def( "setTabPosition", &UIComponentHelper::setTabPosition )
                        .def( "getTabPosition", &UIComponentHelper::getTabPosition )
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
}  // namespace workphone
