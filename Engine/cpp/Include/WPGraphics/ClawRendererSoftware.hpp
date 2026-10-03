#ifndef ClawRendererSoftware_h__
#define ClawRendererSoftware_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/Graphics/IRenderer3.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Math/AABB2.hpp>
#include <Workphone/Math/AABB3.hpp>
#include <Workphone/Math/Matrix4.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>

struct wp_renderer;
struct wp_mat4f;

namespace workphone
{
    namespace render
    {
        /**
         * Thin C++ adapter over the C89 CPU software renderer.
         *
         * The C renderer owns the colour/depth buffers and performs clipping,
         * transforms, rasterisation, texturing, blending and scissoring.
         */
        class WPGraphics_API ClawRendererSoftware : public IRenderer3
        {
        public:
            enum class TransformState : u32
            {
                World = 0,
                View = 1,
                Projection = 2,
                Texture0 = 3,
                Texture1 = 4,
                Texture2 = 5,
                Texture3 = 6,
                Count = 7
            };

            ClawRendererSoftware();
            ~ClawRendererSoftware() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            void beginRender() override;
            void endRender() override;
            void flush() override;
            void clear( const ColourF &colour ) override;
            void setRenderTarget( SmartPtr<IRenderTarget> renderTarget ) override;
            SmartPtr<IRenderTarget> getRenderTarget() const override;
            void setViewport( SmartPtr<IViewport> viewport ) override;
            SmartPtr<IViewport> getViewport() const override;
            void render( const SmartPtr<ISharedObject> &renderData, const SmartPtr<ITexture> &texture,
                         const Matrix4F &transform, const ColourF &colour ) override;
            void render( const SmartPtr<ISharedObject> &renderData, const SmartPtr<IMaterial> &material,
                         const Matrix4F &transform, const ColourF &colour ) override;
            void _getObject( void **ppObject ) override;

            void setCamera( SmartPtr<IGraphicsCamera> camera ) override;
            SmartPtr<IGraphicsCamera> getCamera() const override;
            void drawLine( const Vector3<real_Num> &start, const Vector3<real_Num> &end,
                           const ColourF &colour ) override;

            void setTransform( TransformState state, const Matrix4F &matrix );
            Matrix4F getTransform( TransformState state ) const;

            void setMaterial( SmartPtr<IMaterial> material );
            SmartPtr<IMaterial> getMaterial() const;

            SmartPtr<ITexture> getTexture( const String &filename );
            SmartPtr<ITexture> createTexture( const Vector2I &size, const String &name,
                                              PixelFormat format = PixelFormat::PF_A8R8G8B8 );
            void removeTexture( SmartPtr<ITexture> texture );
            void removeAllTextures();

            Vector2I getScreenSize() const;

            void setScissorRect( const AABB2<real_Num> &rect );
            void clearScissorRect();

            void draw3DLine( const Vector3<real_Num> &start, const Vector3<real_Num> &end,
                             const ColourF &colour );
            void draw3DBox( const AABB3<real_Num> &box, const ColourF &colour );
            void draw3DTriangle( const Vector3<real_Num> &a, const Vector3<real_Num> &b,
                                 const Vector3<real_Num> &c, const ColourF &colour );

            void draw2DLine( const Vector2<real_Num> &start, const Vector2<real_Num> &end,
                             const ColourF &colour );
            void draw2DRect( const AABB2<real_Num> &rect, const ColourF &colour );
            void draw2DFilledRect( const AABB2<real_Num> &rect, const ColourF &colour );
            void draw2DImage( const SmartPtr<ITexture> &texture, const AABB2<real_Num> &destRect,
                              const AABB2<real_Num> &srcRect, const ColourF &colour );

            s32 getFPS() const;
            u32 getPrimitiveCountDrawn() const;

            wp_renderer *getNativeRenderer() const;

            WP_CLASS_REGISTER_DECL;

        private:
            void destroyRenderer();
            void applyTransform( TransformState state );
            void apply3DTransforms();
            void beginScreenSpace();
            void endScreenSpace();

            static wp_mat4f toCMatrix( const Matrix4F &matrix );
            static u32 packColour( const ColourF &colour );

            wp_renderer *m_renderer = nullptr;
            Matrix4F m_transforms[static_cast<u32>( TransformState::Count )];

            SmartPtr<IMaterial> m_currentMaterial;
            SmartPtr<IGraphicsCamera> m_camera;
            SmartPtr<IRenderTarget> m_renderTarget;
            SmartPtr<IViewport> m_viewport;
            Array<SmartPtr<ITexture>> m_textures;

            Vector2I m_bufferSize;
            bool m_inFrame = false;
            bool m_screenDepthTest = true;

            s32 m_fps = 0;
            u32 m_primitiveCount = 0;
            u32 m_frameCount = 0;
            f32 m_frameTimer = 0.0f;
        };
    }  // namespace render
}  // namespace workphone

#endif  // ClawRendererSoftware_h__
