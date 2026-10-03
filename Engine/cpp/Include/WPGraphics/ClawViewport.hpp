#ifndef ClawViewport_h__
#define ClawViewport_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/Graphics/IViewport.hpp>

namespace workphone
{
    namespace render
    {
        class WPGraphics_API ClawViewport : public IViewport
        {
        public:
            ClawViewport();
            ~ClawViewport() override;

            void setCamera( SmartPtr<IGraphicsCamera> camera ) override;

            SmartPtr<IGraphicsCamera> getCamera() const override;

            hash_type getViewportId() const override;
            void setViewportId( hash_type id ) override;

            s32 getZOrder() const override;
            void setZOrder( s32 zorder ) override;

            Vector2<real_Num> getActualPosition() const override;

            Vector2<real_Num> getPosition() const override;
            void setPosition( const Vector2<real_Num> &position ) override;

            Vector2<real_Num> getSize() const override;
            Vector2<real_Num> getActualSize() const override;

            void setSize( const Vector2<real_Num> &size ) override;
            void setBackgroundColour( const ColourF &colour ) override;

            ColourF getBackgroundColour() const override;
            void setClearEveryFrame( bool clear, u32 buffers ) override;

            bool getClearEveryFrame() const override;
            u32 getClearBuffers() const override;

            void setMaterialScheme( const String &schemeName ) override;
            String getMaterialScheme() const override;

            void setEnableSceneRender( bool enabled ) override;
            bool getEnableSceneRender() const override;

            void setOverlaysEnabled( bool enabled ) override;
            bool getOverlaysEnabled() const override;

            void setEnableUI( bool enabled ) override;
            bool getEnableUI() const override;

            void setSkiesEnabled( bool enabled ) override;
            bool getSkiesEnabled() const override;

            void setShadowsEnabled( bool enabled ) override;
            bool getShadowsEnabled() const override;

            void setVisibilityMask( u32 mask ) override;
            u32 getVisibilityMask() const override;

            void setAutoUpdated( bool autoupdate ) override;
            bool isAutoUpdated() const override;

            void _getObject( void **ppObject ) const override;

            SmartPtr<ITexture> getBackgroundTexture() const override;
            void setBackgroundTexture( SmartPtr<ITexture> texture ) override;

            String getBackgroundTextureName() const override;
            void setBackgroundTextureName( const String &textureName ) override;

            SmartPtr<IRenderTarget> getRenderTarget() const override;
            void setRenderTarget( SmartPtr<IRenderTarget> renderTarget ) override;

            bool isActive() const override;
            void setActive( bool active ) override;

            bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;
            bool handleStateChanged( SmartPtr<IState> &state ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            Vector2<real_Num> m_position;
            Vector2<real_Num> m_actualPosition;
            Vector2<real_Num> m_size;
            Vector2<real_Num> m_actualSize;
            ColourF m_backgroundColour;

            hash_type m_viewportId = 0;
            s32 m_zOrder = 0;
            u32 m_clearBuffers = IGraphicsScene::FBT_COLOUR | IGraphicsScene::FBT_DEPTH;
            u32 m_visibilityMask = 0xFFFFFFFF;

            bool m_clearEveryFrame = true;
            bool m_enableSceneRender = true;
            bool m_overlaysEnabled = true;
            bool m_enableUI = true;
            bool m_skiesEnabled = true;
            bool m_shadowsEnabled = true;
            bool m_active = true;
            bool m_autoUpdated = true;

            SmartPtr<IGraphicsCamera> m_camera;
            SmartPtr<ITexture> m_backgroundTexture;

            SmartPtr<IRenderTarget> m_renderTarget;

            String m_backgroundTextureName;
            String m_materialScheme;
        };
    }  // namespace render
}  // namespace workphone

#endif  // ClawViewport_h__
