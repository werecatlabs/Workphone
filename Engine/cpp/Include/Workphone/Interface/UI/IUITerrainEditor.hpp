#ifndef IUITerrainEditor_h__
#define IUITerrainEditor_h__

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {

        /** Interface for terrain editor UI element. */
        class WPCore_API IUITerrainEditor : public IUIElement
        {
        public:
            /** Hash for a selected terrain texture. */
            static const hash_type selectTerrainTextureHash;

            IUITerrainEditor() : IUIElement( IUITerrainEditor::typeInfo() )
            {
            }

            IUITerrainEditor( u32 poolTypeId ) : IUIElement( poolTypeId )
            {
            }

            /** Destructor. */
            ~IUITerrainEditor() override;

            /** Get the terrain component. */
            virtual SmartPtr<scene::IComponent> getTerrain() const = 0;

            /** Set the terrain component. */
            virtual void setTerrain( SmartPtr<scene::IComponent> terrain ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace ui
}  // namespace workphone

#endif  // IUITerrainEditor_h__
