#ifndef CollisionMaskDialog_h__
#define CollisionMaskDialog_h__

#include <EditorPrerequisites.hpp>
#include <ui/EditorWindow.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>

namespace workphone
{
    namespace editor
    {
        class CollisionMaskDialog : public EditorWindow
        {
        public:
            enum WidgetId
            {
                MaskTree,
                MaskName,
                MaskValue,
                AddMask,
                RemoveMask,

                Count
            };

            class UIElementListener : public IEventListener
            {
            public:
                UIElementListener();
                ~UIElementListener() override;

                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                SmartPtr<CollisionMaskDialog> getOwner() const;
                void setOwner( SmartPtr<CollisionMaskDialog> owner );

                WP_CLASS_REGISTER_DECL;

            private:
                WeakPtr<CollisionMaskDialog> m_owner;
            };

            CollisionMaskDialog();
            ~CollisionMaskDialog() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            void populate();
            void setWindowVisible( bool visible ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            SmartPtr<ui::IUITreeCtrl> m_tree;
            SmartPtr<ui::IUILabelTextInputPair> m_maskName;
            SmartPtr<ui::IUILabelTextInputPair> m_maskValue;
            SmartPtr<ui::IUIButton> m_addMaskButton;
            SmartPtr<ui::IUIButton> m_removeMaskButton;
            SmartPtr<IEventListener> m_uiListener;
        };
    }  // namespace editor
}  // namespace workphone

#endif  // CollisionMaskDialog_h__
