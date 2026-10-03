#ifndef AssetEditor_h__
#define AssetEditor_h__

#include "ui/EditorWindow.hpp"
#include <Workphone/Core/Array.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>

namespace workphone
{
    namespace editor
    {

        class AssetEditor : public EditorWindow
        {
        public:
            AssetEditor();
            ~AssetEditor() override;

            /** @copydoc ISharedObject::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ISharedObject::unload */
            void unload( SmartPtr<ISharedObject> data ) override;
        };

    }  // namespace editor
}  // namespace workphone

#endif  // AssetEditor_h__
