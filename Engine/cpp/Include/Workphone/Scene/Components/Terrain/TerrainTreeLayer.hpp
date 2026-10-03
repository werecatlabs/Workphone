#ifndef TerrainTreeLayer_h__
#define TerrainTreeLayer_h__

#include <Workphone/Scene/Components/SubComponent.hpp>

namespace workphone
{
    namespace scene
    {
        class WPCore_API TerrainTreeLayer : public SubComponent
        {
        public:
            static const String BaseTextureStr;
            static const String IndexStr;
            static const String PrefabPathStr;
            static const String DensityStr;

            TerrainTreeLayer();
            ~TerrainTreeLayer() override;

            /** @copydoc BaseComponent::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc BaseComponent::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc BaseComponent::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc BaseComponent::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            SmartPtr<render::ITexture> getBaseTexture() const;

            void setBaseTexture( SmartPtr<render::ITexture> baseTexture );

            s32 getIndex() const;

            void setIndex( s32 index );

            SmartPtr<IGamePrefab> getPrefab() const;

            void setPrefab( SmartPtr<IGamePrefab> prefab );

            String getPrefabPath() const;

            void setPrefabPath( const String &prefabPath );

            s32 getDensity() const;

            void setDensity( s32 density );

            WP_CLASS_REGISTER_DECL;

        protected:
            s32 m_index = 0;

            SmartPtr<render::ITexture> m_baseTexture;

            // The prefab for your object
            SmartPtr<IGamePrefab> m_prefab;

            String m_prefabPath;

            // The number of instances to generate
            s32 m_density = 100;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // TerrainTreeLayer_h__
