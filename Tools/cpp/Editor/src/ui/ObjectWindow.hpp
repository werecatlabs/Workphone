#ifndef ObjectWindow_h__
#define ObjectWindow_h__

#include "EditorTypes.hpp"
#include "ui/EditorWindow.hpp"

namespace workphone
{
    namespace editor
    {
        /**
         * @class ObjectWindow
         * @brief Editor window that hosts the object inspector and related sub-windows.
         *
         * The window tracks the currently selected object type and coordinates the
         * specialized child windows used to inspect actors, materials, files,
         * resources, and properties.
         */
        class ObjectWindow : public EditorWindow
        {
        public:
            /**
             * @brief Identifiers for the local window controls.
             */
            enum
            {
                CANCEL_BTN_ID,
            };

            /**
             * @brief Creates an object inspector window without an explicit parent.
             */
            ObjectWindow();

            /**
             * @brief Creates an object inspector window attached to a parent UI window.
             * @param parent The parent UI window to attach to.
             */
            ObjectWindow( SmartPtr<ui::IUIWindow> parent );

            /**
             * @brief Destroys the window and releases any owned UI resources.
             */
            ~ObjectWindow() override;

            /**
             * @brief Loads the inspector and creates its child windows.
             * @param data Optional shared object payload supplied by the editor framework.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Reloads the inspector content for the current selection.
             * @param data Optional shared object payload supplied by the editor framework.
             */
            void reload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the inspector and destroys its child windows.
             * @param data Optional shared object payload supplied by the editor framework.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Gets the root UI window owned by this inspector.
             * @return The root UI window if one has been created; otherwise, a null pointer.
             */
            SmartPtr<ui::IUIWindow> getWindow() const;

            /**
             * @brief Sets the root UI window owned by this inspector.
             * @param parent The root UI window to track.
             */
            void setWindow( SmartPtr<ui::IUIWindow> parent );

            /**
             * @brief Updates the inspector state to match the current editor selection.
             */
            void updateSelection() override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Root UI window used to host the inspector content.
             */
            SmartPtr<ui::IUIWindow> m_window;

            /**
             * @brief Child window responsible for actor inspection.
             */
            SmartPtr<ActorWindow> m_actorWindow;

            /**
             * @brief Child window responsible for material inspection.
             */
            SmartPtr<EditorWindow> m_materialWindow;

            /**
             * @brief Child window responsible for property inspection.
             */
            SmartPtr<PropertiesWindow> m_propertiesWindow;

            /**
             * @brief Child window responsible for browsing files in the selection.
             */
            SmartPtr<FileViewWindow> m_fileViewWindow;

            /**
             * @brief Child window responsible for resource inspection.
             */
            SmartPtr<ResourceWindow> m_resourceWindow;

            /**
             * @brief Current object classification derived from the editor selection.
             */
            Atomic<ObjectType> m_objectType = ObjectType::None;

            /**
             * @brief Current resource classification derived from the editor selection.
             */
            Atomic<ObjectType> m_resourceType = ObjectType::None;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // ObjectWindow_h__
