#ifndef EntityWindow_h__
#define EntityWindow_h__

#include <EditorPrerequisites.hpp>
#include "ui/EditorWindow.hpp"
#include <Workphone/Interface/System/IStateListener.hpp>

namespace workphone
{
    namespace editor
    {
        class ProjectWindow : public EditorWindow
        {
        public:
            enum
            {
                MENU_ADD_SCRIPT_ID,
                MENU_ADD_BUILDER_ID,
                MENU_ADD_COMPONENT_ID,
                MENU_ADD_FSM_ID,
                ENTITY_TREE_ID,

                MENU_ADD_CAMERA,
                MENU_REMOVE_CAMERA,

                APPLICATION_MENU_ADD_EVENT,

                PW_ADD_NEW_ENTITY,
                PW_ADD_EXISTING_ENTITY,

                PW_ADD_ENT_SCENE_NODE,
                PW_ADD_ENT_GFX_OBJ,
                PW_REMOVE_ENT_GFX_OBJ,
                PW_REMOVE_ENT_SCENE_NODE,

                PW_ADD_ENT_SHAPE,
                PW_ADD_ENT_BODY,
                PW_REMOVE_ENT_SHAPE,
                PW_REMOVE_ENT_BODY,

                PW_ADD_ENT_SOUND,
                PW_REMOVE_ENT_SOUND,

                PW_ENTITY_REMOVE,
                PW_GENERATE_SCRIPT,

                PW_ADD_NEW_GUI,
                PW_ADD_EXISTING_GUI,

                PW_COMPONENT_REMOVE,

                PW_FSM_REMOVE,

                PW_EVENT_ADD,
                PW_EVENT_REMOVE,
                PW_EVENT_MODIFY,

                PW_FSM_EVENT_ADD,
                PW_FSM_EVENT_REMOVE,
                PW_FSM_EVENT_MODIFY,

                PW_FSM_STATE_ADD,
                PW_FSM_STATE_REMOVE,

                PW_ENT_SCRIPT_REMOVE,

                PW_ADD_NEW_SCRIPT,
                PW_ADD_EXISTING_SCRIPT,
                PW_ADD_SCRIPT_FOLDER,
                PW_REMOVE_SCRIPT,
                PW_RENAME_SCRIPT,

                PW_ADD_TERRAIN,
                PW_ADD_MESH,
                PW_ADD_DESTRUCTIBLE_MESH,
                PW_ADD_PARTICLE_SYSTEM,

                PW_ADD_SCENE,

                PW_ADD_VEHICLE_4_WHEEL,
                PW_ADD_VEHICLE_AIRPLANE,
                PW_ADD_VEHICLE_HELICOPTER,
                PW_ADD_VEHICLE_TANK,

                PW_ADD_GOAL,
                PW_ADD_EVALUATOR,

                PW_ADD_MAP,
                PW_MAP_REFRESH,

                PW_ADD_MATERIAL,
                PW_ADD_TECHNIQUE,
            };

            ProjectWindow();
            ~ProjectWindow() override;

            /** @copydoc ISharedObject::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ISharedObject::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            void buildTree();

            bool isValid() const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            SmartPtr<ProjectAssetsWindow> m_projectAssetsWindow;
            SmartPtr<FileWindow> m_fileWindow;

            SmartPtr<ISharedObject> m_selectedObject;
            SmartPtr<ISharedObject> m_selectedEntity;

            std::map<String, bool> treeState;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // EntityWindow_h__
