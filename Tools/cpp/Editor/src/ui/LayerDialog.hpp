#ifndef LayerDialog_h__
#define LayerDialog_h__

#include <EditorPrerequisites.hpp>
#include <ui/EditorWindow.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>

namespace workphone
{
    namespace editor
    {
        class LayerDialog : public EditorWindow
        {
        public:
            enum WidgetId
            {
                LayerTree,
                LayerName,
                AddLayer,
                RemoveLayer,

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

                SmartPtr<LayerDialog> getOwner() const;
                void setOwner( SmartPtr<LayerDialog> owner );

                WP_CLASS_REGISTER_DECL;

            private:
                WeakPtr<LayerDialog> m_owner;
            };

            LayerDialog();
            ~LayerDialog() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            void populate();
            void setWindowVisible( bool visible ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            SmartPtr<ui::IUITreeCtrl> m_tree;
            SmartPtr<ui::IUILabelTextInputPair> m_layerName;
            SmartPtr<ui::IUIButton> m_addLayerButton;
            SmartPtr<ui::IUIButton> m_removeLayerButton;
            SmartPtr<IEventListener> m_uiListener;
        };
    }  // namespace editor
}  // namespace workphone

#endif  // LayerDialog_h__
