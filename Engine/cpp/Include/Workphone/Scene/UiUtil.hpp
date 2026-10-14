#ifndef WORKPHONE_SCENE_UIUTIL_HPP
#define WORKPHONE_SCENE_UIUTIL_HPP

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/WorkphoneEnums.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Memory/SmartPtr.hpp>

namespace workphone::scene
{

    /** Scene UI factories and placement helpers.
     *
     * Call on the scene's owning thread with a loaded game manager and registered component
     * factories and LayoutTransformSystem installed (as in Application::load). Every actor includes
     * a LayoutTransform. addToScene publishes only the root
     * after construction; otherwise the caller must parent it or add it to a scene. Detached
     * actors are still owned by the game manager: release them with destroyActor, not just reset().
     * Components follow the normal scene load/state lifecycle; factories do not force renderer
     * creation. Dimensions and positions are in layout units, initially centred in the parent.
     *
     * Invalid geometry/alignment throws std::invalid_argument; unavailable services or component
     * factories throw std::runtime_error. Failed construction destroys all actors it allocated.
     * Legacy director arguments are reserved and have no effect; a nonempty hint names the root.
     * Styling, events, and specialized container contents are configured through the components.
     */
    class WPCore_API UiUtil
    {
    public:
        // Ordered by the corresponding enum, including Custom (excluding COUNT).
        static const Array<String> directionNames;
        static const Array<String> verticalAlignmentTypes;
        static const Array<String> horizontalAlignmentTypes;
        static const String directionStr;
        static const String handleStr;
        static const String backgroundStr;
        static const String fillStr;
        static const String scrollValueStr;
        static const String sliderValueStr;
        static const String topStr;
        static const String centerStr;
        static const String bottomStr;
        static const String leftStr;
        static const String rightStr;
        static const String customStr;
        static const String dropDownNameStr;
        static const String optionStr;
        static const String panelStr;

        /// Invalid enums return an empty string. Parsing is case-sensitive and falls back to
        /// LEFT/TOP for unknown strings, preserving the existing property-editor contract.
        static String getHorizontalAlignmentString( HorizontalAlignment alignment );
        static HorizontalAlignment getHorizontalAlignment( const String &str );
        /// Semicolon-delimited enum names with a trailing semicolon, for property editors.
        static String getHorizontalAlignmentTypesString();
        static String getVerticalAlignmentString( VerticalAlignment alignment );
        static VerticalAlignment getVerticalAlignment( const String &str );
        static String getVerticalAlignmentTypesString();

        /// Set finite position and nonnegative size. Requires an existing LayoutTransform.
        static void setRect( SmartPtr<IGameActor> actor, f32 x, f32 y, f32 width, f32 height );
        /// Updates anchors and pivot; Custom preserves both on that axis.
        static void setAlignment( SmartPtr<IGameActor> actor, HorizontalAlignment horizontal,
                                  VerticalAlignment vertical );

        /// Load new components parent-first after assembling the hierarchy. Already loaded
        /// components are skipped, so adding children and calling again is safe. The caller
        /// then enters edit/play state through the normal actor/scene lifecycle. A component
        /// load failure throws std::runtime_error and leaves the hierarchy owned by the caller.
        static void loadHierarchy( SmartPtr<IGameActor> actor );

        static SmartPtr<IGameActor> createText( const String &label, bool addToScene = true );
        static SmartPtr<IGameActor> createButton( const String &label, bool addToScene = true );
        static SmartPtr<IGameActor> createButtonEx( const String &label, f32 x, f32 y, f32 width,
                                                    f32 height, bool addToScene = true );
        /// Backend dropdown by default, with a wired scene trigger/options path when native UI
        /// is disabled. No sample options are inserted.
        static SmartPtr<IGameActor> createDropdown( const String &label, bool addToScene = true );
        static SmartPtr<IGameActor> createDropdownWithOptions( const String &label,
                                                               const Array<String> &options,
                                                               bool addToScene = true );
        /// Empty textureName uses the Image's default, with no hard-coded texture dependency.
        static SmartPtr<IGameActor> createImage( const String &textureName, bool addToScene = true );
        static SmartPtr<IGameActor> createInputField( const String &text, const String &placeholder,
                                                      bool addToScene = true );
        static SmartPtr<IGameActor> createToggle( const String &label, bool toggled,
                                                  bool addToScene = true );
        static SmartPtr<IGameActor> createPanel( SmartPtr<IBuildDirector> director, const String &hint,
                                                 bool addToScene );
        /// Sliders have Background, Fill and Handle children, range [0,1], initial value 0.5.
        static SmartPtr<IGameActor> createSlider( const String &label, SmartPtr<IBuildDirector> director,
                                                  const String &hint, bool addToScene );
        static SmartPtr<IGameActor> createSliderVertical( const String &label,
                                                          SmartPtr<IBuildDirector> director,
                                                          const String &hint, bool addToScene );
        /// Scrollbars have Background, Fill and Handle children, initial scroll value 0.
        static SmartPtr<IGameActor> createScrollbar( const String &label,
                                                     SmartPtr<IBuildDirector> director,
                                                     const String &hint, bool addToScene );
        static SmartPtr<IGameActor> createScrollbarVertical( const String &label,
                                                             SmartPtr<IBuildDirector> director,
                                                             const String &hint, bool addToScene );
        /// Vertical scroll view with Content and a linked ScrollBar child. Add user children
        /// to getContentPanel()->getActor(), size that content, then call updateScrollBar().
        static SmartPtr<IGameActor> createScrollview( const String &label,
                                                      SmartPtr<IBuildDirector> director,
                                                      const String &hint, bool addToScene );
        /// Consistently capitalized alias; the original spelling remains supported.
        static SmartPtr<IGameActor> createScrollView( const String &label, bool addToScene = true );

        /// Empty containers. Populate through their component APIs before entering play state.
        static SmartPtr<IGameActor> createLayout( const String &name, bool addToScene = true );
        static SmartPtr<IGameActor> createHorizontalLayout( const String &name, bool addToScene = true );
        static SmartPtr<IGameActor> createVerticalLayout( const String &name, bool addToScene = true );
        static SmartPtr<IGameActor> createGridLayout( const String &name, s32 columns,
                                                      bool addToScene = true );
        static SmartPtr<IGameActor> createTableLayout( const String &name, bool addToScene = true );
        static SmartPtr<IGameActor> createTableCell( const String &name, bool addToScene = true );
        static SmartPtr<IGameActor> createTabView( const String &name, bool addToScene = true );
        static SmartPtr<IGameActor> createTabPage( const String &name, bool addToScene = true );
        static SmartPtr<IGameActor> createTabItem( const String &label, bool addToScene = true );
        static SmartPtr<IGameActor> createToggleGroup( const String &name, bool addToScene = true );
        static SmartPtr<IGameActor> createToolTip( const String &text, bool addToScene = true );
        /// Thumbnail with a wired Image and Text child.
        static SmartPtr<IGameActor> createThumbnail( const String &label, const String &textureName,
                                                     bool addToScene = true );
    };
}  // namespace workphone::scene

#endif  // WORKPHONE_SCENE_UIUTIL_HPP
