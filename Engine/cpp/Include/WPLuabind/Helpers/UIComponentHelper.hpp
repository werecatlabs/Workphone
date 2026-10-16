#ifndef UIComponentHelper_h__
#define UIComponentHelper_h__

#include <Workphone/Workphone.hpp>
#include <lua.hpp>

namespace workphone
{
    class UIComponentHelper
    {
    public:
        static void buttonSetClickHandler( scene::Button *button, scene::IComponent *component,
                                           const String &functionName );
        static lua_Integer getInputFieldType( const scene::InputField *inputField );
        static void setInputFieldType( scene::InputField *inputField, lua_Integer inputType );
        static void setInputFieldTypeWithHint( scene::InputField *inputField, lua_Integer inputType,
                                               const String &textHint );
        static lua_Integer getPanelFlags( const scene::Layout *layout );
        static void setPanelFlags( scene::Layout *layout, lua_Integer flags );
        static bool hasPanelFlag( const scene::Layout *layout, lua_Integer flag );
        static void addPanelFlag( scene::Layout *layout, lua_Integer flag );
        static void removePanelFlag( scene::Layout *layout, lua_Integer flag );
        static lua_Integer getChildHorizontalAlignment( const scene::LayoutContainer *layout );
        static void setChildHorizontalAlignment( scene::LayoutContainer *layout,
                                                 lua_Integer alignment );
        static lua_Integer getChildVerticalAlignment( const scene::LayoutContainer *layout );
        static void setChildVerticalAlignment( scene::LayoutContainer *layout, lua_Integer alignment );
        static lua_Integer getLayoutHorizontalAlignment( const scene::LayoutTransform *transform );
        static void setLayoutHorizontalAlignment( scene::LayoutTransform *transform,
                                                  lua_Integer alignment );
        static lua_Integer getLayoutVerticalAlignment( const scene::LayoutTransform *transform );
        static void setLayoutVerticalAlignment( scene::LayoutTransform *transform, lua_Integer alignment );
        static lua_Integer getScrollBarDirection( const scene::ScrollBar *scrollBar );
        static void setScrollBarDirection( scene::ScrollBar *scrollBar, lua_Integer direction );
        static lua_Integer getComponentSliderDirection( const scene::Slider *slider );
        static void setComponentSliderDirection( scene::Slider *slider, lua_Integer direction );
        static lua_Integer getComponentToggleType( const scene::Toggle *toggle );
        static void setComponentToggleType( scene::Toggle *toggle, lua_Integer type );
        static lua_Integer getComponentToggleState( const scene::Toggle *toggle );
        static void setComponentToggleState( scene::Toggle *toggle, lua_Integer state );
        static lua_Integer getTabOrientation( const scene::TabView *tabView );
        static void setTabOrientation( scene::TabView *tabView, lua_Integer orientation );
        static lua_Integer getTabPosition( const scene::TabView *tabView );
        static void setTabPosition( scene::TabView *tabView, lua_Integer position );
    };
}  // namespace workphone

#endif  // UIComponentHelper_h__
