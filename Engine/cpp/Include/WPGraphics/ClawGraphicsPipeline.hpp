#ifndef ClawGraphicsPipeline_h__
#define ClawGraphicsPipeline_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Graphics/GraphicsPipeline.hpp>
#include <Workphone/Core/Array.hpp>



namespace workphone
{
    namespace render
    {

        class WPGraphics_API ClawGraphicsPipeline final : public GraphicsPipeline
        {
        public:
            ClawGraphicsPipeline();
            ~ClawGraphicsPipeline() override;

            ClawGraphicsPipeline( const ClawGraphicsPipeline & ) = delete;
            ClawGraphicsPipeline &operator=( const ClawGraphicsPipeline & ) = delete;

            bool initialize( s32 width, s32 height ) override;
            void shutdown() override;
            void beginFrame() override;
            void endFrame() override;
            void render() override;

            /** Capture/present the CPU software renderer. GPU backends stay on their native path. */
            bool captureSoftwareFrame( wp_renderer *renderer );
            bool presentSoftwareFrame( wp_renderer *renderer ) const;

            void setQualityLevel( QualityLevel level ) override;
            void setTaaSettings( const TaaSettings &settings ) override;
            void setTaoaSettings( const TaoaSettings &settings ) override;
            void setHdrSettings( const HdrSettings &settings ) override;
            void setCsmSettings( const CsmSettings &settings ) override;

            void enableTAA( bool enabled ) override;
            void enableGTAO( bool enabled ) override;
            void enableSSR( bool enabled ) override;
            void enableContactShadows( bool enabled ) override;
            void enableDOF( bool enabled ) override;
            void enableBloom( bool enabled ) override;
            void enableExposure( bool enabled ) override;
            void enableMotionBlur( bool enabled ) override;
            void enableCSM( bool enabled ) override;

            const f32 *getOutputBuffer( s32 *outWidth = nullptr,
                                        s32 *outHeight = nullptr ) const override;
            const f32 *getGBufferNormal() const override;
            const f32 *getGBufferDepth() const override;
            const f32 *getGBufferVelocity() const override;
            void setDebugView( DebugView view ) override;

            WP_CLASS_REGISTER_DECL;

        private:
            void applyNativeSettings();

            wp_render_pipeline *m_nativePipeline = nullptr;
            Matrix4F m_previousViewProjection = Matrix4F::identity();

            bool m_hasPreviousViewProjection = false;
            bool m_hasFrameInputs = false;

            Array<f32> m_hdrInput;
            Array<f32> m_normalInput;
            Array<f32> m_depthInput;
            Array<f32> m_velocityInput;

            mutable Array<f32> m_rgbaOutput;
            mutable Array<u8> m_ldrPresentation;
        };

    }  // namespace render
}  // namespace workphone

#endif  // ClawGraphicsPipeline_h__
