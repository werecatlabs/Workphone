#ifndef UICoreRenderer_h__
#define UICoreRenderer_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <WPGraphicsOgreNext/Wrapper/CRenderer.hpp>
#include <OgrePrerequisites.h>
#include <OgreHlmsSamplerblock.h>
#include <OgreDescriptorSetSampler.h>
#include "workphone_prerequisites.h"

namespace Ogre
{
    class PsoCacheHelper;
}

namespace workphone
{
    namespace render
    {
        class UIRenderer : public CRenderer
        {
        public:
            UIRenderer();
            ~UIRenderer();

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            ::wp_context *getContext();

            void beginFrame();
            void endFrame();

            void render();

            void setDisplaySize( float width, float height );

            void createFontTexture();
            void createMaterial();
            void setupBuffers( size_t maxVertices, size_t maxIndices );
            void uploadBuffers( struct wp_buffer *vbuf, struct wp_buffer *ibuf );
            void draw( struct wp_buffer *cmds );

            void updateProjectionMatrix( float width, float height );

            static UIRenderer &getSingleton( void );
            static UIRenderer *getSingletonPtr( void );

            void setCamera( SmartPtr<IGraphicsCamera> camera ) override;

            SmartPtr<IGraphicsCamera> getCamera() const override;

        private:
            ::wp_context *m_ctx = nullptr;
            wp_font_atlas *m_atlas = nullptr;
            wp_draw_null_texture *m_texNull = nullptr;
            wp_buffer *m_cmds = nullptr;

            Ogre::TextureGpu *m_fontTexture = nullptr;

            Ogre::PsoCacheHelper *m_psoCache = nullptr;
            Ogre::Pass *m_pass = nullptr;
            const Ogre::HlmsSamplerblock *m_samplerblock = nullptr;
            const Ogre::DescriptorSetSampler *m_descSetSampler = nullptr;

            Ogre::HighLevelGpuProgramPtr m_vertexShaderUnified;
            Ogre::HighLevelGpuProgramPtr m_pixelShaderUnified;
            Ogre::HighLevelGpuProgramPtr m_vertexShaderD3D11;
            Ogre::HighLevelGpuProgramPtr m_pixelShaderD3D11;
            Ogre::HighLevelGpuProgramPtr m_vertexShaderMetal;
            Ogre::HighLevelGpuProgramPtr m_pixelShaderMetal;
            Ogre::HighLevelGpuProgramPtr m_vertexShaderGL;
            Ogre::HighLevelGpuProgramPtr m_pixelShaderGL;

            Ogre::VertexBufferPacked *m_vbo = nullptr;
            Ogre::IndexBufferPacked *m_ibo = nullptr;
            Ogre::VertexArrayObject *m_vao = nullptr;
            Ogre::IndirectBufferPacked *m_indirectBuffer = nullptr;

            size_t m_maxVertices = 0;
            size_t m_maxIndices = 0;

            f32 m_width = 1280.0f;
            f32 m_height = 720.0f;

            static UIRenderer *s_instance;
        };

    }  // namespace render
}  // namespace workphone

#endif  // UICoreRenderer_h__
