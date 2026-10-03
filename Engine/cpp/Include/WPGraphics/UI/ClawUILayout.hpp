#ifndef _ClawUILAYOUT_H
#define _ClawUILAYOUT_H

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/UI/UILayout.hpp>
#include <WPGraphics/UI/ClawUIElement.hpp>

namespace workphone
{
    namespace ui
    {
        class ClawUILayout : public ClawUIElement<UILayout>
        {
        public:
            enum LayoutStates
            {
                FS_IDLE,
                FS_FADEIN,
                FS_FADEOUT,

                FS_COUNT
            };

            ClawUILayout();
            ~ClawUILayout() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            bool handleEvent( const SmartPtr<IInputEvent> &event ) override;

            void update() override;

            bool handleStateChanged( SmartPtr<IState> &state ) override;

            SmartPtr<IFSM> getFSM();
            const SmartPtr<IFSM> &getFSM() const;

            u8 GetStateFromName( const String &stateName ) const;
            String GetStateNameFromId( u8 stateId ) const;

            /** Adds a child to this ui item. */
            void addChild( SmartPtr<IUIElement> pGUIItem ) override;

            /** Removes a child of the ui item. */
            bool removeChild( SmartPtr<IUIElement> pGUIItem ) override;

            SmartPtr<IUIWindow> getParentWindow() const override;

            void setParentWindow( SmartPtr<IUIWindow> uiWindow ) override;

            void invalidate() override;

            void updateZOrder() override;

            SmartPtr<Properties> getProperties() const override;

            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            void draw( struct wp_context *ctx ) override;

            WP_CLASS_REGISTER_DECL;

        private:
            SmartPtr<IUIWindow> m_uiWindow;

            SmartPtr<IFSM> m_fsm;

            static u32 m_nameExt;
        };
    }  // end namespace ui
}  // namespace workphone

#endif
