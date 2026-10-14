# UiUtil scene UI factories

`workphone::scene::UiUtil` creates scene actors, not immediate-mode widgets. All factories add
`LayoutTransform` and configure the requested component. Use a loaded game manager with the
Workphone component factories and `LayoutTransformSystem` installed (as in `Application::load`), on the scene's owning thread.

## Create and place UI

```cpp
using namespace workphone;
using namespace workphone::scene;

auto window = UiUtil::createLayout("Settings", true);
UiUtil::setRect(window, 0, 0, 640, 480);
auto save = UiUtil::createButton("Save", false);
window->addChild(save);
UiUtil::setRect(save, 20, 20, 160, 48);
UiUtil::setAlignment(save, HorizontalAlignment::LEFT, VerticalAlignment::TOP);
auto quality = UiUtil::createDropdownWithOptions("Quality", {"Low", "Medium", "High"}, false);
window->addChild(quality);
UiUtil::setRect(quality, 20, 90, 240, 48);
UiUtil::loadHierarchy(window);
window->setState(IGameActor::State::Edit, true);
```

A `Layout` provides the renderer's UI window. Parent controls to that hierarchy and let the normal
scene component load/state lifecycle create renderer elements. `loadHierarchy` loads new components parent-first, skips already loaded components, and synchronizes child scene references after parenting. It can be called again after adding children. `addToScene=true` inserts the root
actor once after construction, with its children attached. `false` leaves it detached for parenting
or later scene insertion. Detached actors still occupy game-manager slots; use
`gameManager->destroyActor(actor)` when finished, which destroys children too.

`setRect` accepts finite layout coordinates and nonnegative dimensions, including zero for collapsed
controls. It does not automatically resize a composite's internal track/content; resize their
`LayoutTransform`s through the control's component references when customizing a composite.
`setAlignment` updates anchors; `CUSTOM` retains the current anchors on that axis. Invalid inputs
throw `std::invalid_argument` before modifying placement or allocating a factory actor. Missing
services/factories throw `std::runtime_error`. Failed composite construction releases all its actors.

## Factory coverage

- Text and buttons, including positioned `createButtonEx`.
- Dropdowns with empty or caller-provided ordered options; no sample options are added. They support backend combo widgets and a composed button/options panel.
- Images, panels, input fields, toggles, thumbnails, and tooltips.
- Horizontal/vertical sliders and scrollbars with wired Background, Fill, and Handle actors.
- Vertical scroll views with Content and a linked ScrollBar.
- Layout windows, horizontal/vertical/grid layouts, tables/cells, tabs/pages/items, and toggle groups.

Container factories create empty containers. Add their contents and configure the relevant
component API, such as `GridLayout::setCellSize`, `TabView::addTabPage`, `TabPage::addTabItem`,
`TabItem::setContent`, or `TableCell::setTableLayout`, before entering play state. Labels, events,
colours, fonts, input modes, textures, and ranges remain configurable through the components.
Panels and track parts do not depend on hard-coded PNG filenames; an empty image texture name
uses the renderer's default Image appearance.

Sliders start at 0.5 in [0,1]. Scrollbars start at zero with a 50-unit handle on a 200-unit track.
A scroll view starts with a 300x200 viewport and 270x200 content. Populate and resize its content,
then synchronize the scrollbar:

```cpp
auto items = UiUtil::createScrollView("Items", false);
window->addChild(items);
auto view = items->getComponent<ScrollView>();
auto content = view->getContentPanel();
content->getActor()->addChild(UiUtil::createText("First item", false));
content->setSize(Vector2<real_Num>(270, 800));
view->updateScrollBar();
```

The view disables its scrollbar when content fits. Scrollbar replacement and view unloading
clear the previous scrollbar's back-reference. Applications still own layout changes and must call
`updateScrollBar()` after changing content/viewport dimensions.

## Lua

Lua exposes the same factories as static `UiUtil` functions. Pass `addToScene` explicitly;
C++ default arguments are not Lua overloads. Options and the alignment/direction name arrays are
ordinary one-based Lua tables.

```lua
local button = UiUtil.createButtonEx("Save", 20, 20, 160, 48, false)
local quality = UiUtil.createDropdownWithOptions("Quality", {"Low", "High"}, false)
UiUtil.setRect(quality, 20, 90, 240, 48)
UiUtil.setAlignment(quality, UiUtil.getHorizontalAlignment("Left"),
                   UiUtil.getVerticalAlignment("Top"))
```

Dropdown options must be a dense sequence of strings. Invalid alignment integers are rejected
before enum conversion, including large integers that would otherwise wrap around.

## Compatibility and validation

Existing signatures, public identifiers, case-sensitive alignment parsing, LEFT/TOP fallbacks for
unknown strings, and trailing-semicolon property-editor lists remain supported. `Custom` now
round-trips correctly. `createScrollview` remains as the legacy spelling; `createScrollView` is an
alias. Legacy director parameters remain reserved; a nonempty hint now names the root actor.
Button/dropdown/slider labels survive renderer creation, and `Button::setTextStr` also updates native
button labels when there is no separate Text child.

Native regression cases in `ComponentTestsUI.cpp` cover all factory ownership modes, labels and
content, alignment, placement validation, slider/scrollbar endpoints, scroll-view synchronization,
and rollback after a missing component factory. `script_uiutil_factory_contracts` in
`ScriptTests.cpp` exercises the Lua tables, factories, placement, and invalid inputs. These contracts
do not replace a rendered UI/input smoke test in the Editor, particularly for clipping, native
widget appearance, focus, pointer interaction, and specialized container population.

## Backend widgets and scene controls

`UIComponent::getUseNativeUI()` / `setUseNativeUI(bool)` selects the active backend widget for
Button, Slider, and Dropdown when true (the default), or the composed scene representation when
false. For WPGraphics, the backend is the WorkphoneCore fork of Nuklear. Primitive Image/Text
parts still draw through that backend. Other components retain their existing rendering paths.

```cpp
auto volume = UiUtil::createSlider("Volume", nullptr, "", false);
volume->getComponent<Slider>()->setUseNativeUI(false); // scene track/fill/handle
```

Both representations preserve the component value/selection and callbacks. Changing modes hides
the previous representation. The `useNativeUI` property is serialized and exposed through Lua.
Claw provides combo, horizontal slider, and vertical slider adapters. Its layout draw pass clips
scroll-content drawing and hit testing to the viewport, including nested scroll views.

`SampleGame` contains a visual showcase of both modes, text entry, toggles, and a scrollable list.
Run `SampleGame.exe` with its binary directory as the working directory so the sample plugin
configuration and assets resolve. The sample sources are in `Samples/cpp/Game`.