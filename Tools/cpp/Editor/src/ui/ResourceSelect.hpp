#ifndef ResourceSelect_h__
#define ResourceSelect_h__

#include <EditorPrerequisites.hpp>
#include <ui/EditorWindow.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>

namespace workphone
{
    namespace editor
    {

        class ResourceSelect : public ISharedObject
        {
        public:
            ResourceSelect();
            ~ResourceSelect();

            /** @copydoc ISharedObject::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ISharedObject::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

        protected:
            SmartPtr<ui::IUIText> m_text;
            SmartPtr<ui::IUIImage> m_image;
            SmartPtr<ui::IUIButton> m_button;
            SmartPtr<ui::IUIText> m_label;
            SmartPtr<render::IMaterial> m_material;
            SmartPtr<render::IGraphicsTerrain> m_terrain;

            u32 m_index = 0;
            hash_type m_hash = 0;
        };

    }  // namespace editor
}  // namespace workphone

#endif  // ResourceSelect_h__
