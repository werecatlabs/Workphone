#ifndef ObjectBrowserDialog_h__
#define ObjectBrowserDialog_h__

#include <Workphone/Interface/System/IEventListener.hpp>
#include <ui/EditorWindow.hpp>

namespace workphone
{
    namespace editor
    {
        class ObjectBrowserDialog : public EditorWindow
        {
        public:
            enum WidgetId
            {
                Tree,
                AddComponent,

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

                SmartPtr<ObjectBrowserDialog> getOwner() const;

                void setOwner( SmartPtr<ObjectBrowserDialog> owner );

                WP_CLASS_REGISTER_DECL;

            private:
                WeakPtr<ObjectBrowserDialog> m_owner;
            };

            ObjectBrowserDialog();
            ~ObjectBrowserDialog() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            void populate();

            String getSelectedObject() const;
            void setSelectedObject( const String &selectedObject );

            SmartPtr<ui::IUITreeCtrl> getTree() const;
            void setTree( SmartPtr<ui::IUITreeCtrl> tree );

            void setWindowVisible( bool visible ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            SmartPtr<ui::IUITreeCtrl> m_tree;
            SmartPtr<IEventListener> m_uiListener;

            SmartPtr<ui::IUIButton> m_addComponentButton;

            String m_selectedObject;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // AddBodyDialog_h__
