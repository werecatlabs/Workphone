//
// Created by Zane Desir on 11/11/2021.
//

#ifndef WP_MATERIALWINDOW_H
#define WP_MATERIALWINDOW_H

#include <EditorPrerequisites.hpp>
#include <ui/ScriptWindow.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>

namespace workphone
{
    namespace editor
    {

        class MaterialWindow : public ScriptWindow
        {
        public:
            enum class Types
            {
                Albedo,
                Normal,
                Metallic,
                Emission,

                Count
            };

            MaterialWindow( SmartPtr<ui::IUIWindow> parent );
            ~MaterialWindow() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            void updateSelection() override;

            SmartPtr<render::IMaterial> getMaterial() const;
            void setMaterial( SmartPtr<render::IMaterial> material );

            SmartPtr<ui::IUITreeCtrl> getTree() const;
            void setTree( SmartPtr<ui::IUITreeCtrl> tree );

            SmartPtr<PropertiesWindow> getPropertiesWindow() const;

            void setPropertiesWindow( SmartPtr<PropertiesWindow> propertiesWindow );

            WP_CLASS_REGISTER_DECL;

        protected:
            class DropdownListener : public IEventListener
            {
            public:
                DropdownListener();
                ~DropdownListener() override;

                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                MaterialWindow *getOwner() const;

                void setOwner( MaterialWindow *owner );

                WP_CLASS_REGISTER_DECL;

            private:
                MaterialWindow *m_owner = nullptr;
            };

            class TreeCtrlListener : public IEventListener
            {
            public:
                TreeCtrlListener();
                ~TreeCtrlListener() override;

                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                MaterialWindow *getOwner() const;

                void setOwner( MaterialWindow *owner );

                WP_CLASS_REGISTER_DECL;

            private:
                MaterialWindow *m_owner = nullptr;
            };

            void buildTree();

            void addMaterialToTree( SmartPtr<render::IMaterial> material,
                                    SmartPtr<ui::IUITreeNode> node );

            void addObjectToTree( SmartPtr<ISharedObject> object, SmartPtr<ui::IUITreeNode> node );

            void handleDropdownSelection();

            //--- Data-driven shader UI (Esoterica integration, step 4) -------------------------
            /** @brief Build the data-driven shader picker dropdown + the parameter control container. */
            void buildShaderUI();
            /** @brief Apply the shader selected in m_shaderDropdown to the current material. */
            void handleShaderDropdownSelection();
            /** @brief (Re)build the per-parameter editor controls from the material's bound shader. */
            void refreshShaderParameterControls();

            SmartPtr<ui::IUIDropdown> m_dropdown;

            SmartPtr<ui::IUITreeCtrl> m_tree;
            SmartPtr<IEventListener> m_treeListener;
            AtomicSmartPtr<render::IMaterial> m_material;

            SmartPtr<PropertiesWindow> m_propertiesWindow;

            //--- Data-driven shader UI (Esoterica integration, step 4) -------------------------
            /** Forward decls for the listeners owned by the shader UI (defined in the cpp). */
            class ShaderDropdownListener;
            class ParamValueListener;

            /** Dropdown listing every registered MaterialShader (DefaultPBR/Unlit/ColorOnly/...). */
            SmartPtr<ui::IUIDropdown> m_shaderDropdown;
            SmartPtr<IEventListener> m_shaderDropdownListener;

            /** Container holding the auto-generated, per-parameter editor controls. */
            SmartPtr<ui::IUIElement> m_shaderParamContainer;
            SmartPtr<IEventListener> m_paramValueListener;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // WP_MATERIALWINDOW_H
