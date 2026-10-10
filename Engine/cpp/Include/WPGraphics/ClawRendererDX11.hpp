#ifndef ClawRendererDX11_h__
#define ClawRendererDX11_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <workphone_graphics_particle_simulation.h>
#include <Workphone/Interface/Graphics/IRenderer3.hpp>
#include <Workphone/Math/Matrix4.hpp>
#include <Workphone/Memory/AtomicSmartPtr.hpp>
#include <WPGraphics/ClawCubemap.hpp>

struct wp_renderer;
struct wp_mat4f;
struct wp_graphics_mesh;
struct wp_material_dx11;
struct wp_instance_pntc_dx11;

namespace workphone
{
    namespace render
    {
        class ClawMesh;
        class ClawTerrain;

        /**
         * @class ClawRendererDX11
         * @brief Thin C++ adapter over the C89 Direct3D 11 renderer.
         *
         * The underlying C renderer owns the Direct3D device, swap chain,
         * render targets, shaders, pipeline state and transient geometry buffers.
         * This class maps Workphone's C++ graphics interfaces to that API,
         * providing a convenient object‑oriented wrapper.
         */
        class WPGraphics_API ClawRendererDX11 : public IRenderer3
        {
        public:
            /**
             * @brief Default constructor. Initializes internal state.
             */
            ClawRendererDX11();

            /**
             * @brief Destructor. Ensures the native renderer is properly
             *        destroyed.
             */
            ~ClawRendererDX11() override;

            /**
             * @brief Load resources required by the renderer.
             * @param data Shared object containing initialization data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload resources and clean up.
             * @param data Shared object containing teardown data.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** Begin a new rendering frame. */
            void beginRender() override;

            /** End the current rendering frame and present the swap chain. */
            void endRender() override;

            /** Flush any pending GPU commands. */
            void flush() override;

            /**
             * @brief Clear the current render target with a solid colour.
             * @param colour Colour to clear the target with.
             */
            void clear( const ColourF &colour ) override;

            /**
             * @brief Set the active render target.
             * @param renderTarget Smart pointer to the render target.
             */
            void setRenderTarget( SmartPtr<IRenderTarget> renderTarget ) override;

            /**
             * @brief Retrieve the currently bound render target.
             * @return Smart pointer to the render target.
             */
            SmartPtr<IRenderTarget> getRenderTarget() const override;

            /**
             * @brief Set the viewport used for subsequent drawing.
             * @param viewport Smart pointer to the viewport object.
             */
            void setViewport( SmartPtr<IViewport> viewport ) override;

            /**
             * @brief Get the currently active viewport.
             * @return Smart pointer to the viewport.
             */
            SmartPtr<IViewport> getViewport() const override;
            /**
             * @brief Render geometry using a texture.
             * @param renderData Shared object containing geometry data.
             * @param texture   Texture to apply.
             * @param transform World‑view‑projection matrix.
             * @param colour    Modulation colour.
             */
            void render( const SmartPtr<ISharedObject> &renderData, const SmartPtr<ITexture> &texture,
                         const Matrix4F &transform, const ColourF &colour ) override;

            /**
             * @brief Render geometry using a material.
             * @param renderData Shared object containing geometry data.
             * @param material   Material to use for shading.
             * @param transform  World‑view‑projection matrix.
             * @param colour     Modulation colour.
             */
            void render( const SmartPtr<ISharedObject> &renderData, const SmartPtr<IMaterial> &material,
                         const Matrix4F &transform, const ColourF &colour ) override;
            /**
             * @brief Retrieve the underlying native renderer object.
             * @param ppObject Pointer to receive the native object.
             */
            void _getObject( void **ppObject ) override;

            /**
             * @brief Set the active graphics camera.
             * @param camera Smart pointer to the camera.
             */
            void setCamera( SmartPtr<IGraphicsCamera> camera ) override;

            /**
             * @brief Get the currently active graphics camera.
             * @return Smart pointer to the camera.
             */
            SmartPtr<IGraphicsCamera> getCamera() const override;

            /**
             * @brief Draw a line between two points.
             * @param start   Starting position.
             * @param end     Ending position.
             * @param colour  Line colour.
             */
            void drawLine( const Vector3<real_Num> &start, const Vector3<real_Num> &end,
                           const ColourF &colour ) override;

            wp_renderer *getNativeRenderer() const;

            /** Camera-fitted directional shadows, rendered before the colour pass. */
            bool beginShadowMap( s32 mapSize = 2048 );
            void endShadowMap();
            void disableShadows();

            /**
             * @brief Render a mesh object.
             * @param mesh      Pointer to the mesh to render.
             * @param transform World transform applied to the mesh.
             */
            void renderMesh( ClawMesh *mesh, const Matrix4F &transform );
            /** Draw fixed-LOD instances through the same material/submesh path.
             * Returns submitted instance-sections; zero means no output. */
            u64 renderMeshInstances( ClawMesh *mesh, const wp_instance_pntc_dx11 *instances,
                                     u32 count );
            /** Draw an immutable particle snapshot; does not advance simulation. */
            void renderParticles( const Array<wp_particle_sample> &particles,
                                  const Matrix4F &world, const Vector3F &scale,
                                  const SmartPtr<IMaterial> &material = nullptr );

            /**
             * @brief Render terrain.
             * @param terrain Smart pointer to the terrain object.
             */
            void renderTerrain( const SmartPtr<ClawTerrain> &terrain );

            /**
             * @brief Render sky geometry.
             * @param sky Smart pointer to the sky object.
             */
            void renderSky( const SmartPtr<ISky> &sky );

            /**
             * @brief Configure scene lighting parameters.
             * @param ambient    Ambient light colour.
             * @param direction  Direction of the main light.
             * @param colour     Light colour.
             * @param intensity  Light intensity multiplier.
             */
            void setSceneLighting( const ColourF &ambient, const Vector3F &direction,
                                   const ColourF &colour, f32 intensity );
            void setSceneFog( u32 mode, const ColourF &colour, f32 density, f32 start, f32 end );
            /**
             * @brief Release a mesh previously handed to the native renderer.
             * @param mesh Pointer to the native mesh structure.
             */
            static void forgetMesh( const wp_graphics_mesh *mesh );

            WP_CLASS_REGISTER_DECL;

        private:
            u64 renderMeshBatch( ClawMesh *mesh, const Matrix4F &transform,
                                 const wp_instance_pntc_dx11 *instances, u32 instanceCount );
            void applySceneLighting( wp_material_dx11 &material ) const;

            /**
             * @brief Destroy the underlying native renderer and release resources.
             */
            void destroyRenderer();

            /**
             * @brief Apply world transform to the native rendering pipeline.
             * @param world World transformation matrix.
             */
            void setTransforms( const Matrix4F &world );

            /**
             * @brief Convert a Matrix4F to the C‑style wp_mat4f structure.
             * @param matrix Matrix to convert.
             * @return Corresponding wp_mat4f value.
             */
            static wp_mat4f toCMatrix( const Matrix4F &matrix );

            /**
             * @brief Pack a ColourF into a 32‑bit ARGB integer.
             * @param colour Colour to pack.
             * @return 32‑bit packed colour.
             */
            static u32 packColour( const ColourF &colour );

            wp_renderer *m_renderer = nullptr;
            ClawCubemap m_environment;
            mutable ClawCubemap m_previewEnvironment;
            bool m_hasSkyEnvironment = false;

            AtomicSmartPtr<IGraphicsCamera> m_camera;
            AtomicSmartPtr<IRenderTarget> m_renderTarget;
            AtomicSmartPtr<IViewport> m_viewport;

            ColourF m_ambientLight = ColourF( 0.12f, 0.12f, 0.12f, 1.0f );
            Vector3F m_lightDirection = Vector3F( -0.35f, -0.8f, -0.45f );
            ColourF m_lightColour = ColourF::White;
            f32 m_lightIntensity = 3.0f;
            wp_vec4f m_fogColour{}, m_fogParams{};

            u32 m_rtWidth = 1280;
            u32 m_rtHeight = 720;
            u32 m_windowWidth = 1280;
            u32 m_windowHeight = 720;
            u32 m_primitiveCount = 0;
            bool m_inFrame = false;
            bool m_shadowPass = false;
            bool m_shadowsEnabled = false;
            Matrix4F m_shadowMatrix = Matrix4F::identity();
        };
    }  // namespace render
}  // namespace workphone

#endif  // ClawRendererDX11_h__
