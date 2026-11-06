#ifndef __GraphicsSystemConfiguration_h__
#define __GraphicsSystemConfiguration_h__

#include <Workphone/System/Director.hpp>
#include <optional>

namespace workphone
{
    namespace render
    {
        /** Interface for graphics settings. */
        class WPCore_API GraphicsSettings : public Director
        {
        public:
            /** Constructor. */
            GraphicsSettings();

            /** Captures renderer settings from the published player profile. */
            explicit GraphicsSettings( SmartPtr<Properties> playerProfile );

            /** Virtual destructor. */
            ~GraphicsSettings() override;

            /** Gets a boolean to know if to create a window. */
            bool getCreateWindow() const;

            /** Sets a boolean to know if to create a window. */
            void setCreateWindow( bool createWindow );

            /** Gets a boolean to know if to show the configuration dialog. */
            bool getShowDialog() const;

            /** Sets a boolean to know if to show the configuration dialog. */
            void setShowDialog( bool showDialog );

            bool getCreateRenderUI() const;

            void setCreateRenderUI( bool createRenderUI );

            /** Gets whether presentation waits for vertical synchronization. */
            bool getVSync() const;

            /** Sets whether presentation waits for vertical synchronization. */
            void setVSync( bool enabled );

            s32 getTextureFiltering() const;

            void setTextureFiltering( s32 textureFiltering );

            s32 getAnisotropy() const;

            void setAnisotropy( s32 anisotropy );

            SmartPtr<Properties> getPlayerProfile() const;

            const String &getRendererApi() const { return m_rendererApi; }
            s32 getRendererMode() const { return m_rendererMode; }
            s32 getShaderLanguage() const { return m_shaderLanguage; }
            s32 getShaderModel() const { return m_shaderModel; }
            s32 getPresentMode() const { return m_presentMode; }
            s32 getParticleQuality() const { return m_particleQuality; }
            s32 getShadowQuality() const { return m_shadowQuality; }
            s32 getShadowResolution() const { return m_shadowResolution; }
            f32 getShadowDistance() const { return m_shadowDistance; }
            bool getValidationLayers() const { return m_validationLayers; }
            bool getAllowRendererFallback() const { return m_allowRendererFallback; }
            bool getStaticBatching() const { return m_staticBatching; }
            bool getDynamicBatching() const { return m_dynamicBatching; }
            bool getReflectionProbes() const { return m_reflectionProbes; }
            bool getLightingDisabled() const { return m_lightingDisabled; }
            std::optional<s32> getLightingMode() const
            {
                return m_hasLightingMode ? std::optional{ m_lightingMode } : std::nullopt;
            }
            std::optional<u32> getTextureFilteringOverride() const
            {
                return m_textureFilteringOverride;
            }
            std::optional<u32> getAnisotropyOverride() const { return m_anisotropyOverride; }

            WP_CLASS_REGISTER_DECL;

        protected:
            s32 m_lightingMode = 0;
            s32 m_textureFiltering = 0;
            s32 m_anisotropy = 0;
            bool m_showDialog = false;
            bool m_createWindow = true;
            bool m_createRenderUI = false;
            bool m_vsync = false;
            bool m_reflectionProbes = false;

        private:
            SmartPtr<Properties> m_playerProfile;
            String m_rendererApi = "DX11";
            std::optional<u32> m_textureFilteringOverride;
            std::optional<u32> m_anisotropyOverride;
            s32 m_rendererMode = 0;
            s32 m_shaderLanguage = 0;
            s32 m_shaderModel = 0;
            s32 m_presentMode = 0;
            s32 m_particleQuality = 0;
            s32 m_shadowQuality = 2;
            s32 m_shadowResolution = 2;
            f32 m_shadowDistance = 200.0f;
            bool m_validationLayers = false;
            bool m_allowRendererFallback = true;
            bool m_staticBatching = true;
            bool m_dynamicBatching = true;
            bool m_hasLightingMode = false;
            bool m_lightingDisabled = false;
        };
    }  // namespace render
}  // namespace workphone

#endif  // IGraphicsSystemConfiguration_h__
