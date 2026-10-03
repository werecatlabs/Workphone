#ifndef TerrainBlendMap_h__
#define TerrainBlendMap_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Scene/Components/SubComponent.hpp>

namespace workphone
{
    namespace scene
    {
        class WPCore_API TerrainBlendMap : public SubComponent
        {
        public:
            /** Creates a TerrainBlendMap object. */
            TerrainBlendMap();
            ~TerrainBlendMap() override;

            /** @copydoc BaseComponent::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc BaseComponent::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            SmartPtr<render::ITerrainBlendMap> getBlendMap() const;

            void setBlendMap( SmartPtr<render::ITerrainBlendMap> blendMap );

            WP_CLASS_REGISTER_DECL;

        protected:
            SmartPtr<render::ITerrainBlendMap> m_blendMap;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // TerrainBlendMap_h__
