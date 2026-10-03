#ifndef TagDialog_h__
#define TagDialog_h__

#include <EditorPrerequisites.hpp>
#include <ui/EditorWindow.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>

namespace workphone
{
    namespace editor
    {
        class TagDialog : public EditorWindow
        {
        public:
            enum WidgetId
            {
                TagTree,
                TagName,
                AddTagOption,
                RemoveTagOption,
                AddToActor,
                RemoveFromActor,

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

                SmartPtr<TagDialog> getOwner() const;
                void setOwner( SmartPtr<TagDialog> owner );

                WP_CLASS_REGISTER_DECL;

            private:
                WeakPtr<TagDialog> m_owner;
            };

            TagDialog();
            ~TagDialog() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            void populate();
            void setWindowVisible( bool visible ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            String getEditedTag() const;
            void updateActorTagDisplay();

            SmartPtr<ui::IUITreeCtrl> m_tree;
            SmartPtr<ui::IUILabelTextInputPair> m_tagName;
            SmartPtr<ui::IUIButton> m_addTagOptionButton;
            SmartPtr<ui::IUIButton> m_removeTagOptionButton;
            SmartPtr<ui::IUIButton> m_addToActorButton;
            SmartPtr<ui::IUIButton> m_removeFromActorButton;
            SmartPtr<IEventListener> m_uiListener;
        };
    }  // namespace editor
}  // namespace workphone

#endif  // TagDialog_h__
