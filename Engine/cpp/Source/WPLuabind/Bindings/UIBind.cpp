#include <WPLuabind/WPLuabindPCH.hpp>
#include <WPLuabind/Bindings/UIBind.hpp>
#include <WPLuabind/SmartPtrConverter.hpp>
#include <WPLuabind/ParamConverter.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Interface/UI/IUIOutputConsole.hpp>
#include <luabind/luabind.hpp>

namespace workphone
{
    using namespace ui;

    void _setVisible( IUIElement *element, bool isVisible )
    {
        element->setVisible( isVisible );
    }

    void _setEnabled( IUIElement *element, bool isEnabled )
    {
        element->setEnabled( isEnabled );
    }

    void _setEnabledCascade( IUIElement *element, bool isEnabled, bool cascade )
    {
        element->setEnabled( isEnabled, cascade );
    }

    int _getFileBrowserDialogMode( IUIFileBrowser *browser )
    {
        return static_cast<int>( browser->getDialogMode() );
    }

    void _setFileBrowserDialogMode( IUIFileBrowser *browser, int mode )
    {
        browser->setDialogMode( static_cast<IUIFileBrowser::DialogMode>( mode ) );
    }

    int _getFileBrowserFilterMode( IUIFileBrowser *browser )
    {
        return static_cast<int>( browser->getFilterMode() );
    }

    void _setFileBrowserFilterMode( IUIFileBrowser *browser, int mode )
    {
        browser->setFilterMode( static_cast<IUIFileBrowser::FilterMode>( mode ) );
    }

    int _getLayoutState( IUILayoutWindow *window )
    {
        return static_cast<int>( window->getState() );
    }

    void _setLayoutState( IUILayoutWindow *window, int state )
    {
        window->setState( static_cast<IUILayoutWindow::LayoutStates>( state ) );
    }

    int _getWindowFlags( IUILayoutWindow *window )
    {
        return static_cast<int>( window->getWindowFlags() );
    }

    void _setWindowFlags( IUILayoutWindow *window, int flags )
    {
        window->setWindowFlags( static_cast<IUILayoutWindow::WindowFlags>( flags ) );
    }

    bool _hasWindowFlag( IUILayoutWindow *window, int flag )
    {
        return window->hasWindowFlag( static_cast<IUILayoutWindow::WindowFlags>( flag ) );
    }

    int _getToggleType( IUIToggle *toggle )
    {
        return static_cast<int>( toggle->getToggleType() );
    }

    void _setToggleType( IUIToggle *toggle, int type )
    {
        toggle->setToggleType( static_cast<IUIToggle::ToggleType>( type ) );
    }

    int _getToggleState( IUIToggle *toggle )
    {
        return static_cast<int>( toggle->getToggleState() );
    }

    void _setToggleState( IUIToggle *toggle, int state )
    {
        toggle->setToggleState( static_cast<IUIToggle::ToggleState>( state ) );
    }

    int _getTreeNodeType( IUITreeNode *node )
    {
        return static_cast<int>( node->getNodeType() );
    }

    void _setTreeNodeType( IUITreeNode *node, int type )
    {
        node->setNodeType( static_cast<IUITreeNode::Type>( type ) );
    }

    int _getColourFormat( IUIColourPicker *picker )
    {
        return static_cast<int>( picker->getColourFormat() );
    }

    void _setColourFormat( IUIColourPicker *picker, int format )
    {
        picker->setColourFormat( static_cast<IUIColourPicker::ColourFormat>( format ) );
    }

    int _getSliderDirection( IUISlider *slider )
    {
        return static_cast<int>( slider->getDirection() );
    }

    void _setSliderDirection( IUISlider *slider, int direction )
    {
        slider->setDirection( static_cast<Direction>( direction ) );
    }

    int _getTextInputType( IUITextEntry *entry )
    {
        return static_cast<int>( entry->getInputType() );
    }

    int _getMenuItemType( IUIMenuItem *item )
    {
        return static_cast<int>( item->getMenuItemType() );
    }

    void _setMenuItemType( IUIMenuItem *item, int type )
    {
        item->setMenuItemType( static_cast<IUIMenuItem::Type>( type ) );
    }

    int _getMenubarOrientation( IUIMenubar *menubar )
    {
        return static_cast<int>( menubar->getOrientation() );
    }

    void _setMenubarOrientation( IUIMenubar *menubar, int orientation )
    {
        menubar->setOrientation( static_cast<Direction>( orientation ) );
    }

    void bindUI( lua_State *L )
    {
        using namespace luabind;

        module( L )[class_<IUIDragSource, IEventListener, SmartPtr<IUIDragSource>>( "IUIDragSource" )
                        .scope[def( "typeInfo", IUIDragSource::typeInfo )]];

        module( L )[class_<IUIDropTarget, IEventListener, SmartPtr<IUIDropTarget>>( "IUIDropTarget" )
                        .scope[def( "typeInfo", IUIDropTarget::typeInfo )]];

        module( L )[class_<IUIElement, core::IPrototype, SmartPtr<IUIElement>>( "IUIElement" )
                        .def( "handleEvent", &IUIElement::handleEvent )
                        .def( "getElementId", &IUIElement::getElementId )
                        .def( "setElementId", &IUIElement::setElementId )
                        .def( "getLabel", &IUIElement::getLabel )
                        .def( "setLabel", &IUIElement::setLabel )
                        .def( "setPosition", &IUIElement::setPosition )
                        .def( "getPosition", &IUIElement::getPosition )
                        .def( "getAbsolutePosition", &IUIElement::getAbsolutePosition )
                        .def( "setSize", &IUIElement::setSize )
                        .def( "getSize", &IUIElement::getSize )
                        .def( "getScale", &IUIElement::getScale )
                        .def( "setScale", &IUIElement::setScale )
                        .def( "setFocus", &IUIElement::setFocus )
                        .def( "isInFocus", &IUIElement::isInFocus )
                        .def( "setVisible", &IUIElement::setVisible )
                        .def( "isVisible", &IUIElement::isVisible )
                        .def( "setEnabled", &IUIElement::setEnabled )
                        .def( "isEnabled", &IUIElement::isEnabled )
                        .def( "setHighlighted", &IUIElement::setHighlighted )
                        .def( "isHighlighted", &IUIElement::isHighlighted )
                        .def( "setHovered", &IUIElement::setHovered )
                        .def( "isHovered", &IUIElement::isHovered )
                        .def( "getParent", &IUIElement::getParent )
                        .def( "setParent", &IUIElement::setParent )
                        .def( "getNumChildren", &IUIElement::getNumChildren )
                        .def( "getChildren", &IUIElement::getChildren )
                        .def( "addChild", &IUIElement::addChild )
                        .def( "removeChild", &IUIElement::removeChild )
                        .def( "remove", &IUIElement::remove )
                        .def( "removeAllChildren", &IUIElement::removeAllChildren )
                        .def( "destroyAllChildren", &IUIElement::destroyAllChildren )
                        .def( "hasChildById", &IUIElement::hasChildById )
                        .def( "findChildById", &IUIElement::findChildById )
                        .def( "getSiblingIndex", &IUIElement::getSiblingIndex )
                        .def( "getLayout", &IUIElement::getLayout )
                        .def( "setLayout", &IUIElement::setLayout )
                        .def( "getContainer", &IUIElement::getContainer )
                        .def( "setContainer", &IUIElement::setContainer )
                        .def( "getOwner", &IUIElement::getOwner )
                        .def( "setOwner", &IUIElement::setOwner )
                        .def( "getDragSource", ( SmartPtr<IUIDragSource> & (IUIElement::*)() ) &
                                                   IUIElement::getDragSource )
                        .def( "setDragSource", &IUIElement::setDragSource )
                        .def( "getDropTarget", ( SmartPtr<IUIDropTarget> & (IUIElement::*)() ) &
                                                   IUIElement::getDropTarget )
                        .def( "setDropTarget", &IUIElement::setDropTarget )
                        .def( "getOrder", &IUIElement::getOrder )
                        .def( "setOrder", &IUIElement::setOrder )
                        .def( "getSameLine", &IUIElement::getSameLine )
                        .def( "setSameLine", &IUIElement::setSameLine )
                        .def( "getColour", &IUIElement::getColour )
                        .def( "setColour", &IUIElement::setColour )
                        .def( "getHandleInputEvents", &IUIElement::getHandleInputEvents )
                        .def( "setHandleInputEvents", &IUIElement::setHandleInputEvents )
                        .def( "sortZOrder", &IUIElement::sortZOrder )
                        .def( "updateZOrder", &IUIElement::updateZOrder )
                        .def( "invalidate", &IUIElement::invalidate )
                        .def( "getStateContext", &IUIElement::getStateContext )
                        .def( "setStateContext", &IUIElement::setStateContext )
                        .def( "getStateListener", &IUIElement::getStateListener )
                        .def( "setStateListener", &IUIElement::setStateListener )
                        .def( "getRenderChildren", &IUIElement::getRenderChildren )
                        .def( "setRenderChildren", &IUIElement::setRenderChildren )
                        .def( "onActivate", &IUIElement::onActivate )
                        .def( "onDeactivate", &IUIElement::onDeactivate )
                        .def( "onSelect", &IUIElement::onSelect )
                        .def( "onDeselect", &IUIElement::onDeselect )
                        .def( "onGainFocus", &IUIElement::onGainFocus )
                        .def( "onLostFocus", &IUIElement::onLostFocus )
                        .scope[def( "typeInfo", IUIElement::typeInfo )]];

        module( L )[class_<IUIApplication, ISharedObject, SmartPtr<IUIApplication>>( "IUIApplication" )
                        .def( "handleWindowEvent", &IUIApplication::handleWindowEvent )
                        .def( "handleInputEvent", &IUIApplication::handleInputEvent )
                        .def( "getMenubar", &IUIApplication::getMenubar )
                        .def( "setMenubar", &IUIApplication::setMenubar )
                        .def( "getToolbar", &IUIApplication::getToolbar )
                        .def( "setToolbar", &IUIApplication::setToolbar )
                        .def( "getWindowSize", &IUIApplication::getWindowSize )
                        .def( "setWindowSize", &IUIApplication::setWindowSize )
                        .scope[def( "typeInfo", IUIApplication::typeInfo )]];

        module( L )[class_<IUIButton, IUIElement, SmartPtr<IUIButton>>( "IUIButton" )
                        .def( "getLabel", &IUIButton::getLabel )
                        .def( "setLabel", &IUIButton::setLabel )
                        .def( "setTextSize", &IUIButton::setTextSize )
                        .def( "getTextSize", &IUIButton::getTextSize )
                        .scope[def( "typeInfo", IUIButton::typeInfo )]];

        module( L )[class_<IUICheckbox, IUIElement, SmartPtr<IUICheckbox>>( "IUICheckbox" )
                        .def( "setValue", &IUICheckbox::setValue )
                        .def( "getValue", &IUICheckbox::getValue )
                        .scope[def( "typeInfo", IUICheckbox::typeInfo )]];

        module( L )[class_<IUIColourPicker, IUIElement, SmartPtr<IUIColourPicker>>( "IUIColourPicker" )
                        .def( "getLabel", &IUIColourPicker::getLabel )
                        .def( "setLabel", &IUIColourPicker::setLabel )
                        .def( "setColour", &IUIColourPicker::setColour )
                        .def( "getColour", &IUIColourPicker::getColour )
                        .def( "setGradient", &IUIColourPicker::setGradient )
                        .def( "getGradient", &IUIColourPicker::getGradient )
                        .def( "setColourFormat", _setColourFormat )
                        .def( "getColourFormat", _getColourFormat )
                        //.enum_( "ColourFormat" )[value( "RGB", IUIColourPicker::ColourFormat::RGB ),
                        //                         value( "HSL", IUIColourPicker::ColourFormat::HSL ),
                        //                         value( "HEX", IUIColourPicker::ColourFormat::HEX )]
                        .scope[def( "typeInfo", IUIColourPicker::typeInfo )]];

        module( L )[class_<IUICollapsingHeader, IUIElement, SmartPtr<IUICollapsingHeader>>(
                        "IUICollapsingHeader" )
                        .def( "getLabel", &IUICollapsingHeader::getLabel )
                        .def( "setLabel", &IUICollapsingHeader::setLabel )
                        .scope[def( "typeInfo", IUICollapsingHeader::typeInfo )]];

        module( L )[class_<IUIDropdown, IUIElement, SmartPtr<IUIDropdown>>( "IUIDropdown" )
                        .def( "getOptions", &IUIDropdown::getOptions )
                        .def( "setOptions", &IUIDropdown::setOptions )
                        .def( "addOption", &IUIDropdown::addOption )
                        .def( "getSelectedOption", &IUIDropdown::getSelectedOption )
                        .def( "setSelectedOption", &IUIDropdown::setSelectedOption )
                        .scope[def( "typeInfo", IUIDropdown::typeInfo )]];

        module( L )[class_<IUIImage, IUIElement, SmartPtr<IUIImage>>( "IUIImage" )
                        .def( "setTexture", &IUIImage::setTexture )
                        .def( "getTexture", &IUIImage::getTexture )
                        .def( "getSpriteSize", &IUIImage::getSpriteSize )
                        .def( "setSpriteSize", &IUIImage::setSpriteSize )
                        .def( "getBorderLeft", &IUIImage::getBorderLeft )
                        .def( "setBorderLeft", &IUIImage::setBorderLeft )
                        .def( "getBorderRight", &IUIImage::getBorderRight )
                        .def( "setBorderRight", &IUIImage::setBorderRight )
                        .def( "getBorderTop", &IUIImage::getBorderTop )
                        .def( "setBorderTop", &IUIImage::setBorderTop )
                        .def( "getBorderBottom", &IUIImage::getBorderBottom )
                        .def( "setBorderBottom", &IUIImage::setBorderBottom )
                        .def( "getUseTiling", &IUIImage::getUseTiling )
                        .def( "setUseTiling", &IUIImage::setUseTiling )
                        .def( "getUseNineSlice", &IUIImage::getUseNineSlice )
                        .def( "setUseNineSlice", &IUIImage::setUseNineSlice )
                        .def( "getTileScaleX", &IUIImage::getTileScaleX )
                        .def( "setTileScaleX", &IUIImage::setTileScaleX )
                        .def( "getTileScaleY", &IUIImage::getTileScaleY )
                        .def( "setTileScaleY", &IUIImage::setTileScaleY )
                        .scope[def( "typeInfo", IUIImage::typeInfo )]];

        module( L )[class_<IUILabelDropdownPair, IUIElement, SmartPtr<IUILabelDropdownPair>>(
                        "IUILabelDropdownPair" )
                        .def( "getLabel", &IUILabelDropdownPair::getLabel )
                        .def( "setLabel", &IUILabelDropdownPair::setLabel )
                        .def( "getOptions", &IUILabelDropdownPair::getOptions )
                        .def( "setOptions", &IUILabelDropdownPair::setOptions )
                        .def( "getSelectedOption", &IUILabelDropdownPair::getSelectedOption )
                        .def( "setSelectedOption", &IUILabelDropdownPair::setSelectedOption )
                        .def( "removeOption", &IUILabelDropdownPair::removeOption )
                        .def( "addOption", &IUILabelDropdownPair::addOption )
                        .scope[def( "typeInfo", IUILabelDropdownPair::typeInfo )]];

        module( L )[class_<IUILabelTogglePair, IUIElement, SmartPtr<IUILabelTogglePair>>(
                        "IUILabelTogglePair" )
                        .def( "getLabel", &IUILabelTogglePair::getLabel )
                        .def( "setLabel", &IUILabelTogglePair::setLabel )
                        .def( "getValue", &IUILabelTogglePair::getValue )
                        .def( "setValue", &IUILabelTogglePair::setValue )
                        .def( "getShowLabel", &IUILabelTogglePair::getShowLabel )
                        .def( "setShowLabel", &IUILabelTogglePair::setShowLabel )
                        .scope[def( "typeInfo", IUILabelTogglePair::typeInfo )]];

        module( L )[class_<IUILabelTextInputPair, IUIElement, SmartPtr<IUILabelTextInputPair>>(
                        "IUILabelTextInputPair" )
                        .def( "getLabel", &IUILabelTextInputPair::getLabel )
                        .def( "setLabel", &IUILabelTextInputPair::setLabel )
                        .def( "getValue", &IUILabelTextInputPair::getValue )
                        .def( "setValue", &IUILabelTextInputPair::setValue )
                        .scope[def( "typeInfo", IUILabelTextInputPair::typeInfo )]];

        module( L )[class_<IUILabelSliderPair, IUIElement, SmartPtr<IUILabelSliderPair>>(
                        "IUILabelSliderPair" )
                        .def( "getLabel", &IUILabelSliderPair::getLabel )
                        .def( "setLabel", &IUILabelSliderPair::setLabel )
                        .def( "getValue", &IUILabelSliderPair::getValue )
                        .def( "setValue", &IUILabelSliderPair::setValue )
                        .def( "getMinValue", &IUILabelSliderPair::getMinValue )
                        .def( "setMinValue", &IUILabelSliderPair::setMinValue )
                        .def( "getMaxValue", &IUILabelSliderPair::getMaxValue )
                        .def( "setMaxValue", &IUILabelSliderPair::setMaxValue )
                        .scope[def( "typeInfo", IUILabelSliderPair::typeInfo )]];

        module( L )[class_<IUIPropertyGrid, IUIElement, SmartPtr<IUIPropertyGrid>>( "IUIPropertyGrid" )
                        .scope[def( "typeInfo", IUIPropertyGrid::typeInfo )]];

        module(
            L )[class_<IUISlider, IUIElement, SmartPtr<IUISlider>>( "IUISlider" )
                    .def( "getValue", &IUISlider::getValue )
                    .def( "setValue", &IUISlider::setValue )
                    .def( "getMinValue", &IUISlider::getMinValue )
                    .def( "setMinValue", &IUISlider::setMinValue )
                    .def( "getMaxValue", &IUISlider::getMaxValue )
                    .def( "setMaxValue", &IUISlider::setMaxValue )
                    .def( "getDirection", _getSliderDirection )
                    .def( "setDirection", _setSliderDirection )
                    .def( "isDragging", &IUISlider::isDragging )
                    .def( "setDragging", &IUISlider::setDragging )
                    .scope[def( "typeInfo", IUISlider::typeInfo )]
                    .enum_(
                        "Direction" )[value( "Horizontal", static_cast<s32>( Direction::Horizontal ) ),
                                      value( "Vertical", static_cast<s32>( Direction::Vertical ) )]];

        module( L )[class_<IUISpinner, IUIElement, SmartPtr<IUISpinner>>( "IUISpinner" )
                        .def( "setText", &IUISpinner::setText )
                        .def( "getText", &IUISpinner::getText )
                        .def( "incrementValue", &IUISpinner::incrementValue )
                        .def( "decrementValue", &IUISpinner::decrementValue )
                        .def( "setValue", &IUISpinner::setValue )
                        .def( "getValue", &IUISpinner::getValue )
                        .def( "setMinValue", &IUISpinner::setMinValue )
                        .def( "getMinValue", &IUISpinner::getMinValue )
                        .def( "setMaxValue", &IUISpinner::setMaxValue )
                        .def( "getMaxValue", &IUISpinner::getMaxValue )
                        .scope[def( "typeInfo", IUISpinner::typeInfo )]];

        module( L )[class_<IUITabBar, IUIElement, SmartPtr<IUITabBar>>( "IUITabBar" )
                        .def( "addTabItem", &IUITabBar::addTabItem )
                        .def( "removeTabItem", &IUITabBar::removeTabItem )
                        .scope[def( "typeInfo", IUITabBar::typeInfo )]];

        module( L )[class_<IUITabItem, IUIElement, SmartPtr<IUITabItem>>( "IUITabItem" )
                        .def( "getLabel", &IUITabItem::getLabel )
                        .def( "setLabel", &IUITabItem::setLabel )
                        .scope[def( "typeInfo", IUITabItem::typeInfo )]];

        module( L )[class_<IUIText, IUIElement, SmartPtr<IUIText>>( "IUIText" )
                        .def( "getText", &IUIText::getText )
                        .def( "setText", &IUIText::setText )

                        .def( "getTextSize", &IUIText::getTextSize )
                        .def( "setTextSize", &IUIText::setTextSize )

                        .def( "setVerticalAlignment", &IUIText::setVerticalAlignment )
                        .def( "getVerticalAlignment", &IUIText::getVerticalAlignment )
                        .def( "setHorizontalAlignment", &IUIText::setHorizontalAlignment )
                        .def( "getHorizontalAlignment", &IUIText::getHorizontalAlignment )

                        .scope[def( "typeInfo", IUIText::typeInfo )]];

        module(
            L )[class_<IUITextEntry, IUIText, SmartPtr<IUITextEntry>>( "IUITextEntry" )
                    .def( "setPlaceholder", &IUITextEntry::setPlaceholder )
                    .def( "getPlaceholder", &IUITextEntry::getPlaceholder )
                    .def( "setReadOnly", &IUITextEntry::setReadOnly )
                    .def( "isReadOnly", &IUITextEntry::isReadOnly )
                    .def( "setSecureEntry", &IUITextEntry::setSecureEntry )
                    .def( "isSecureEntry", &IUITextEntry::isSecureEntry )
                    .def( "setMultiline", &IUITextEntry::setMultiline )
                    .def( "isMultiline", &IUITextEntry::isMultiline )
                    .def( "setInputType", &IUITextEntry::setInputType )
                    .def( "getInputType", _getTextInputType )
                    .def( "getTextHint", &IUITextEntry::getTextHint )
                    .scope[def( "typeInfo", IUITextEntry::typeInfo )]
                    .enum_( "InputType" )
                        [value( "Text", static_cast<lua_Integer>( IUITextEntry::InputType::Text ) ),
                         value( "Multiline",
                                static_cast<lua_Integer>( IUITextEntry::InputType::Multiline ) ),
                         value( "Password",
                                static_cast<lua_Integer>( IUITextEntry::InputType::Password ) ),
                         value( "Email", static_cast<lua_Integer>( IUITextEntry::InputType::Email ) )]];

        module( L )[class_<IUITreeCtrl, IUIElement, SmartPtr<IUITreeCtrl>>( "IUITreeCtrl" )
                        .def( "clear", &IUITreeCtrl::clear )
                        .def( "addRoot", &IUITreeCtrl::addRoot )
                        .def( "addNode", &IUITreeCtrl::addNode )
                        .def( "expand", &IUITreeCtrl::expand )
                        .def( "getTreeNodes", &IUITreeCtrl::getTreeNodes )
                        .def( "setTreeNodes", &IUITreeCtrl::setTreeNodes )
                        .def( "getSelectedTreeNodes", &IUITreeCtrl::getSelectedTreeNodes )
                        .def( "setSelectedTreeNodes", &IUITreeCtrl::setSelectedTreeNodes )
                        .def( "addSelectedTreeNode", &IUITreeCtrl::addSelectedTreeNode )
                        .def( "clearSelectedTreeNodes", &IUITreeCtrl::clearSelectedTreeNodes )
                        .def( "getSelectedTreeNode", &IUITreeCtrl::getSelectedTreeNode )
                        .def( "setSelectedTreeNode", &IUITreeCtrl::setSelectedTreeNode )
                        .def( "getRoot", &IUITreeCtrl::getRoot )
                        .def( "setRoot", &IUITreeCtrl::setRoot )
                        .def( "getDragSourceElement", &IUITreeCtrl::getDragSourceElement )
                        .def( "setDragSourceElement", &IUITreeCtrl::setDragSourceElement )
                        .def( "getDropDestinationElement", &IUITreeCtrl::getDropDestinationElement )
                        .def( "setDropDestinationElement", &IUITreeCtrl::setDropDestinationElement )
                        .def( "isMultiSelect", &IUITreeCtrl::isMultiSelect )
                        .def( "setMultiSelect", &IUITreeCtrl::setMultiSelect )
                        .def( "getSelectedSiblingIndex", &IUITreeCtrl::getSelectedSiblingIndex )
                        .def( "setSelectedSiblingIndex", &IUITreeCtrl::setSelectedSiblingIndex )
                        .def( "selectRange", &IUITreeCtrl::selectRange )
                        .def( "getNodeById", &IUITreeCtrl::getNodeById )
                        .scope[def( "typeInfo", IUITreeCtrl::typeInfo )]
                        .enum_( "constants" )[value( "clearHash", IUITreeCtrl::clearHash )]

        ];

        module( L )[class_<IUIMenu, IUIElement, SmartPtr<IUIMenu>>( "IUIMenu" )
                        .def( "addMenuItem", &IUIMenu::addMenuItem )
                        .def( "removeMenuItem", &IUIMenu::removeMenuItem )
                        .def( "getMenuItems", &IUIMenu::getMenuItems )
                        .def( "setMenuItems", &IUIMenu::setMenuItems )
                        .def( "setCursorPosition", &IUIMenu::setCursorPosition )
                        .def( "getCursorPosition", &IUIMenu::getCursorPosition )
                        .def( "setNumListItems", &IUIMenu::setNumListItems )
                        .def( "getNumListItems", &IUIMenu::getNumListItems )
                        .def( "setCurrentItemIndex", &IUIMenu::setCurrentItemIndex )
                        .def( "getCurrentItemIndex", &IUIMenu::getCurrentItemIndex )
                        .def( "incrementCursor", &IUIMenu::incrementCursor )
                        .def( "decrementCursor", &IUIMenu::decrementCursor )
                        .def( "getNumMenuItems", &IUIMenu::getNumMenuItems )
                        .def( "getLabel", &IUIMenu::getLabel )
                        .def( "setLabel", &IUIMenu::setLabel )
                        .scope[def( "typeInfo", IUIMenu::typeInfo )]];

        module( L )[class_<IUIMenuItem, IUIElement, SmartPtr<IUIMenuItem>>( "IUIMenuItem" )
                        .def( "getMenuItemType", _getMenuItemType )
                        .def( "setMenuItemType", _setMenuItemType )
                        .def( "getText", &IUIMenuItem::getText )
                        .def( "setText", &IUIMenuItem::setText )
                        .def( "getHelp", &IUIMenuItem::getHelp )
                        .def( "setHelp", &IUIMenuItem::setHelp )
                        .scope[def( "typeInfo", IUIMenuItem::typeInfo )]];

        module( L )[class_<IUIMenubar, IUIElement, SmartPtr<IUIMenubar>>( "IUIMenubar" )
                        .def( "addMenu", &IUIMenubar::addMenu )
                        .def( "removeMenu", &IUIMenubar::removeMenu )
                        .def( "getMenus", &IUIMenubar::getMenus )
                        .def( "setMenus", &IUIMenubar::setMenus )
                        .def( "findMenuByLabel", &IUIMenubar::findMenuByLabel )
                        .def( "getActiveMenu", &IUIMenubar::getActiveMenu )
                        .def( "setActiveMenu", &IUIMenubar::setActiveMenu )
                        .def( "getHighlightedMenu", &IUIMenubar::getHighlightedMenu )
                        .def( "setHighlightedMenu", &IUIMenubar::setHighlightedMenu )
                        .def( "navigateNext", &IUIMenubar::navigateNext )
                        .def( "navigatePrevious", &IUIMenubar::navigatePrevious )
                        .def( "activateHighlightedMenu", &IUIMenubar::activateHighlightedMenu )
                        .def( "closeAllMenus", &IUIMenubar::closeAllMenus )
                        .def( "isKeyboardNavigationEnabled", &IUIMenubar::isKeyboardNavigationEnabled )
                        .def( "setKeyboardNavigationEnabled", &IUIMenubar::setKeyboardNavigationEnabled )
                        .def( "isAutoCloseEnabled", &IUIMenubar::isAutoCloseEnabled )
                        .def( "setAutoCloseEnabled", &IUIMenubar::setAutoCloseEnabled )
                        .def( "getOrientation", _getMenubarOrientation )
                        .def( "setOrientation", _setMenubarOrientation )
                        .def( "handleKeyboardInput", &IUIMenubar::handleKeyboardInput )
                        .def( "hasOpenMenu", &IUIMenubar::hasOpenMenu )
                        .def( "getHighlightedMenuIndex", &IUIMenubar::getHighlightedMenuIndex )
                        .def( "setHighlightedMenuIndex", &IUIMenubar::setHighlightedMenuIndex )
                        .scope[def( "typeInfo", IUIMenubar::typeInfo )]];

        module( L )[class_<IUIWindow, IUIElement, SmartPtr<IUIWindow>>( "IUIWindow" )
                        .def( "setLabel", &IUIWindow::setLabel )
                        .def( "getLabel", &IUIWindow::getLabel )
                        .def( "setContextMenu", &IUIWindow::setContextMenu )
                        .def( "getContextMenu", &IUIWindow::getContextMenu )
                        .def( "hasBorder", &IUIWindow::hasBorder )
                        .def( "setHasBorder", &IUIWindow::setHasBorder )
                        .def( "isDocked", &IUIWindow::isDocked )
                        .def( "setDocked", &IUIWindow::setDocked )
                        .scope[def( "typeInfo", IUIWindow::typeInfo )]];

        module( L )[class_<IUIAbout, IUIElement, SmartPtr<IUIAbout>>( "IUIAbout" )
                        .scope[def( "typeInfo", IUIAbout::typeInfo )]];

        module( L )[class_<IUIAnimatedMaterial, IUIElement, SmartPtr<IUIAnimatedMaterial>>(
                        "IUIAnimatedMaterial" )
                        .def( "setMaterialName", &IUIAnimatedMaterial::setMaterialName )
                        .def( "getMaterialName", &IUIAnimatedMaterial::getMaterialName )
                        .def( "play", &IUIAnimatedMaterial::play )
                        .def( "pause", &IUIAnimatedMaterial::pause )
                        .def( "stop", &IUIAnimatedMaterial::stop )
                        .scope[def( "typeInfo", IUIAnimatedMaterial::typeInfo )]];

        module( L )[class_<IUIAnimator, IUIElement, SmartPtr<IUIAnimator>>( "IUIAnimator" )];

        module( L )[class_<IUIAnimatorPosition, IUIAnimator, SmartPtr<IUIAnimatorPosition>>(
                        "IUIAnimatorPosition" )
                        .def( "setStart", &IUIAnimatorPosition::setStart )
                        .def( "getStart", &IUIAnimatorPosition::getStart )
                        .def( "setEnd", &IUIAnimatorPosition::setEnd )
                        .def( "getEnd", &IUIAnimatorPosition::getEnd )];
        module(
            L )[class_<IUIAnimatorScale, IUIAnimator, SmartPtr<IUIAnimatorScale>>( "IUIAnimatorScale" )];

        module( L )[class_<IUIBar, IUIElement, SmartPtr<IUIBar>>( "IUIBar" )
                        .def( "setPoints", &IUIBar::setPoints )
                        .def( "setMaxPoints", &IUIBar::setMaxPoints )
                        .scope[def( "typeInfo", IUIBar::typeInfo )]];

        module( L )[class_<IUICursor, IUIElement, SmartPtr<IUICursor>>( "IUICursor" )
                        .def( "setMaterialName", &IUICursor::setMaterialName )
                        .scope[def( "typeInfo", IUICursor::typeInfo )]];

        module( L )[class_<IUIDataGrid, IUIElement, SmartPtr<IUIDataGrid>>( "IUIDataGrid" )
                        .scope[def( "typeInfo", IUIDataGrid::typeInfo )]];

        module( L )[class_<IUIDial, IUIElement, SmartPtr<IUIDial>>( "IUIDial" )
                        .def( "setNeedlePosition", &IUIDial::setNeedlePosition )
                        .scope[def( "typeInfo", IUIDial::typeInfo )]];

        module( L )[class_<IUIDialogBox, IUIElement, SmartPtr<IUIDialogBox>>( "IUIDialogBox" )
                        .def( "show", &IUIDialogBox::show )
                        .scope[def( "typeInfo", IUIDialogBox::typeInfo )]];

        module( L )[class_<IUIEvent, IEvent, SmartPtr<IUIEvent>>( "IUIEvent" )
                        .scope[def( "typeInfo", IUIEvent::typeInfo )]];

        module( L )[class_<IUIEventWindow, IUIWindow, SmartPtr<IUIEventWindow>>( "IUIEventWindow" )
                        .def( "getEvents", &IUIEventWindow::getEvents )
                        .def( "setEvents", &IUIEventWindow::setEvents )
                        .scope[def( "typeInfo", IUIEventWindow::typeInfo )]];

        module( L )[class_<IUIFileBrowser, IUIDialogBox, SmartPtr<IUIFileBrowser>>( "IUIFileBrowser" )
                        .def( "getFilePath", &IUIFileBrowser::getFilePath )
                        .def( "setFilePath", &IUIFileBrowser::setFilePath )
                        .def( "getFileExtension", &IUIFileBrowser::getFileExtension )
                        .def( "setFileExtension", &IUIFileBrowser::setFileExtension )
                        .def( "getDialogMode", _getFileBrowserDialogMode )
                        .def( "setDialogMode", _setFileBrowserDialogMode )
                        .def( "getFilterMode", _getFileBrowserFilterMode )
                        .def( "setFilterMode", _setFileBrowserFilterMode )
                        .scope[def( "typeInfo", IUIFileBrowser::typeInfo )]];

        module( L )[class_<IUIFrame, IUIElement, SmartPtr<IUIFrame>>( "IUIFrame" )
                        .scope[def( "typeInfo", IUIFrame::typeInfo )]];

        module( L )[class_<IUIGrid, IUIElement, SmartPtr<IUIGrid>>( "IUIGrid" )
                        .scope[def( "typeInfo", IUIGrid::typeInfo )]];

        /* module( L )[class_<IUIHorizontalLayout, IUILayoutContainer,
                          SmartPtr<IUIHorizontalLayout>>( "IUIHorizontalLayout" )
                        .scope[def( "typeInfo", IUIHorizontalLayout::typeInfo )]];*/

        module( L )[class_<IUIImageArray, IUIElement, SmartPtr<IUIImageArray>>( "IUIImageArray" )
                        .def( "getImage", &IUIImageArray::getImage )
                        .scope[def( "typeInfo", IUIImageArray::typeInfo )]];

        module( L )[class_<IUIInputManager, IUIElement, SmartPtr<IUIInputManager>>( "IUIInputManager" )
                        .scope[def( "typeInfo", IUIInputManager::typeInfo )]];

        module( L )[class_<IUILayoutContainer, IUIElement, SmartPtr<IUILayoutContainer>>(
                        "IUILayoutContainer" )
                        .scope[def( "typeInfo", IUILayoutContainer::typeInfo )]];

        module( L )[class_<IUILayoutWindow, IUIElement, SmartPtr<IUILayoutWindow>>( "IUILayoutWindow" )
                        .def( "getState", _getLayoutState )
                        .def( "setState", _setLayoutState )
                        .def( "getParentWindow", &IUILayoutWindow::getParentWindow )
                        .def( "setParentWindow", &IUILayoutWindow::setParentWindow )
                        .def( "getWindowFlags", _getWindowFlags )
                        .def( "setWindowFlags", _setWindowFlags )
                        .def( "hasWindowFlag", _hasWindowFlag )
                        .scope[def( "typeInfo", IUILayoutWindow::typeInfo )]];

        module(
            L )[class_<IUIProfileWindow, IUIElement, SmartPtr<IUIProfileWindow>>( "IUIProfileWindow" )
                    .def( "getProfile", &IUIProfileWindow::getProfile )
                    .def( "setProfile", &IUIProfileWindow::setProfile )
                    .scope[def( "typeInfo", IUIProfileWindow::typeInfo )]];

        module(
            L )[class_<IUIProfilerWindow, IUIElement, SmartPtr<IUIProfilerWindow>>( "IUIProfilerWindow" )
                    .def( "addProfile", &IUIProfilerWindow::addProfile )
                    .def( "removeProfile", &IUIProfilerWindow::removeProfile )
                    .def( "getProfiles", &IUIProfilerWindow::getProfiles )
                    .def( "setProfiles", &IUIProfilerWindow::setProfiles )
                    .scope[def( "typeInfo", IUIProfilerWindow::typeInfo )]];

        module(
            L )[class_<IUIOutputConsole, IUIElement, SmartPtr<IUIOutputConsole>>( "IUIOutputConsole" )];

        module( L )[class_<IUIProgressBar, IUIElement, SmartPtr<IUIProgressBar>>( "IUIProgressBar" )
                        .def( "getValue", &IUIProgressBar::getValue )
                        .def( "setValue", &IUIProgressBar::setValue )
                        .def( "getMinValue", &IUIProgressBar::getMinValue )
                        .def( "setMinValue", &IUIProgressBar::setMinValue )
                        .def( "getMaxValue", &IUIProgressBar::getMaxValue )
                        .def( "setMaxValue", &IUIProgressBar::setMaxValue )
                        .def( "getShowText", &IUIProgressBar::getShowText )
                        .def( "setShowText", &IUIProgressBar::setShowText )
                        .def( "getFillColour", &IUIProgressBar::getFillColour )
                        .def( "setFillColour", &IUIProgressBar::setFillColour )
                        .def( "getBackgroundColour", &IUIProgressBar::getBackgroundColour )
                        .def( "setBackgroundColour", &IUIProgressBar::setBackgroundColour )
                        .def( "getBorderColour", &IUIProgressBar::getBorderColour )
                        .def( "setBorderColour", &IUIProgressBar::setBorderColour )
                        .def( "getTextColour", &IUIProgressBar::getTextColour )
                        .def( "setTextColour", &IUIProgressBar::setTextColour )
                        .def( "getBorderWidth", &IUIProgressBar::getBorderWidth )
                        .def( "setBorderWidth", &IUIProgressBar::setBorderWidth )
                        .def( "getRounding", &IUIProgressBar::getRounding )
                        .def( "setRounding", &IUIProgressBar::setRounding )
                        .scope[def( "typeInfo", IUIProgressBar::typeInfo )]];

        module( L )[class_<IUIRenderWindow, IUIWindow, SmartPtr<IUIRenderWindow>>( "IUIRenderWindow" )
                        .def( "getWindow", &IUIRenderWindow::getWindow )
                        .def( "setWindow", &IUIRenderWindow::setWindow )
                        .def( "getRenderTexture", &IUIRenderWindow::getRenderTexture )
                        .def( "setRenderTexture", &IUIRenderWindow::setRenderTexture )
                        .scope[def( "typeInfo", IUIRenderWindow::typeInfo )]];

        module(
            L )[class_<IUIScrollingText, IUIElement, SmartPtr<IUIScrollingText>>( "IUIScrollingText" )
                    .scope[def( "typeInfo", IUIScrollingText::typeInfo )]];

        module( L )[class_<IUISearchBar, IUIElement, SmartPtr<IUISearchBar>>( "IUISearchBar" )
                        .scope[def( "typeInfo", IUISearchBar::typeInfo )]];

        module( L )[class_<IUISeparator, IUIElement, SmartPtr<IUISeparator>>( "IUISeparator" )
                        .def( "setHorizontal", &IUISeparator::setHorizontal )
                        .def( "isHorizontal", &IUISeparator::isHorizontal )
                        .def( "setThickness", &IUISeparator::setThickness )
                        .def( "getThickness", &IUISeparator::getThickness )
                        .def( "setMargin", &IUISeparator::setMargin )
                        .def( "getMargin", &IUISeparator::getMargin )
                        .scope[def( "typeInfo", IUISeparator::typeInfo )]];

        module(
            L )[class_<IUITerrainEditor, IUIElement, SmartPtr<IUITerrainEditor>>( "IUITerrainEditor" )
                    .def( "getTerrain", &IUITerrainEditor::getTerrain )
                    .def( "setTerrain", &IUITerrainEditor::setTerrain )
                    .scope[def( "typeInfo", IUITerrainEditor::typeInfo )]];

        module( L )[class_<IUIToggle, IUIButton, SmartPtr<IUIToggle>>( "IUIToggle" )
                        .def( "setToggled", &IUIToggle::setToggled )
                        .def( "isToggled", &IUIToggle::isToggled )
                        .def( "getToggleType", _getToggleType )
                        .def( "setToggleType", _setToggleType )
                        .def( "getToggleState", _getToggleState )
                        .def( "setToggleState", _setToggleState )
                        .def( "getShowLabel", &IUIToggle::getShowLabel )
                        .def( "setShowLabel", &IUIToggle::setShowLabel )
                        .scope[def( "typeInfo", IUIToggle::typeInfo )]];

        module( L )[class_<IUIToggleGroup, IUIElement, SmartPtr<IUIToggleGroup>>( "IUIToggleGroup" )
                        .def( "addToggleButton", &IUIToggleGroup::addToggleButton )
                        .def( "removeToggleButton", &IUIToggleGroup::removeToggleButton )
                        .def( "handleSetToggled", &IUIToggleGroup::handleSetToggled )
                        .def( "getToggledButton", &IUIToggleGroup::getToggledButton )
                        .scope[def( "typeInfo", IUIToggleGroup::typeInfo )]];

        module( L )[class_<IUIToolbar, IUIElement, SmartPtr<IUIToolbar>>( "IUIToolbar" )
                        .scope[def( "typeInfo", IUIToolbar::typeInfo )]];

        module( L )[class_<IUITreeNode, IUIElement, SmartPtr<IUITreeNode>>( "IUITreeNode" )
                        .def( "getNodeData", &IUITreeNode::getNodeData )
                        .def( "setNodeData", &IUITreeNode::setNodeData )
                        .def( "getNodeUserData", &IUITreeNode::getNodeUserData )
                        .def( "setNodeUserData", &IUITreeNode::setNodeUserData )
                        .def( "getNodeType", _getTreeNodeType )
                        .def( "setNodeType", _setTreeNodeType )
                        .def( "getOwnerTree", &IUITreeNode::getOwnerTree )
                        .def( "setOwnerTree", &IUITreeNode::setOwnerTree )
                        .def( "getTreeNodeId", &IUITreeNode::getTreeNodeId )
                        .def( "setTreeNodeId", &IUITreeNode::setTreeNodeId )
                        .def( "isExpanded", &IUITreeNode::isExpanded )
                        .def( "setExpanded", &IUITreeNode::setExpanded )
                        .def( "isSelected", &IUITreeNode::isSelected )
                        .def( "setSelected", &IUITreeNode::setSelected )
                        .scope[def( "typeInfo", IUITreeNode::typeInfo )]];

        module( L )[class_<IUIVector2, IUIElement, SmartPtr<IUIVector2>>( "IUIVector2" )
                        .def( "getValue", &IUIVector2::getValue )
                        .def( "setValue", &IUIVector2::setValue )
                        .def( "getLabel", &IUIVector2::getLabel )
                        .def( "setLabel", &IUIVector2::setLabel )
                        .scope[def( "typeInfo", IUIVector2::typeInfo )]];

        module( L )[class_<IUIVector3, IUIElement, SmartPtr<IUIVector3>>( "IUIVector3" )
                        .def( "getValue", &IUIVector3::getValue )
                        .def( "setValue", &IUIVector3::setValue )
                        .def( "getLabel", &IUIVector3::getLabel )
                        .def( "setLabel", &IUIVector3::setLabel )
                        .scope[def( "typeInfo", IUIVector3::typeInfo )]];

        module( L )[class_<IUIVector4, IUIElement, SmartPtr<IUIVector4>>( "IUIVector4" )
                        .def( "getValue", &IUIVector4::getValue )
                        .def( "setValue", &IUIVector4::setValue )
                        .def( "getLabel", &IUIVector4::getLabel )
                        .def( "setLabel", &IUIVector4::setLabel )
                        .scope[def( "typeInfo", IUIVector4::typeInfo )]];

        module( L )[class_<IUIManager, ISharedObject, SmartPtr<IUIManager>>( "IUIManager" )
                        .def( "loadFont", &IUIManager::loadFont )
                        .def( "unloadFont", &IUIManager::unloadFont )
                        .def( "render", &IUIManager::render )
                        .def( "messagePump", &IUIManager::messagePump )
                        .def( "addApplication", &IUIManager::addApplication )
                        .def( "removeApplication", &IUIManager::removeApplication )
                        .def( "getApplicationPtr", &IUIManager::getApplicationPtr )
                        .def( "getApplication", &IUIManager::getApplication )
                        .def( "setApplication", &IUIManager::setApplication )
                        .def( "addElement", &IUIManager::addElement )
                        .def( "removeElement", &IUIManager::removeElement )
                        .def( "removeElements", &IUIManager::removeElements )
                        .def( "clear", &IUIManager::clear )
                        .def( "getCursor", &IUIManager::getCursor )
                        .def( "findElement", &IUIManager::findElement )
                        .def( "isDragging", &IUIManager::isDragging )
                        .def( "setDragging", &IUIManager::setDragging )
                        .def( "getMainWindow", &IUIManager::getMainWindow )
                        .def( "setMainWindow", &IUIManager::setMainWindow )
                        .def( "invalidate", &IUIManager::invalidate )
                        .def( "loadObject", &IUIManager::loadObject )
                        .def( "unloadObject", &IUIManager::unloadObject )
                        .def( "getOverlay", &IUIManager::getOverlay )
                        .def( "setOverlay", &IUIManager::setOverlay )
                        .scope[def( "typeInfo", IUIManager::typeInfo )]];

        module( L )[class_<IUIVerticalLayout, IUILayoutContainer, SmartPtr<IUIVerticalLayout>>(
                        "IUIVerticalLayout" )
                        .scope[def( "typeInfo", IUIVerticalLayout::typeInfo )]];
    }
} // namespace workphone
