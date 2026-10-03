#pragma once

#if WP_GRAPHICS_SYSTEM_OGRENEXT
#    include <OgreRenderable.h>
#    include <OgreRenderOperation.h>
#    include <cstddef>

struct ImDrawList;

namespace workphone::render
{

    class ImguiRenderableOgreNext : public Ogre::Renderable
    {
    public:
        ImguiRenderableOgreNext();
        ~ImguiRenderableOgreNext();

        // Uploads one complete ImGui draw list.
        void updateVertexData( const ImDrawVert *vtxBuf, const ImDrawIdx *idxBuf, unsigned int vtxCount,
                               unsigned int idxCount );

        // Selects the range used by the next draw command without re-uploading the draw list.
        void setDrawRange( unsigned int vertexOffset, unsigned int indexOffset,
                           unsigned int indexCount );

        //Overrides from Renderable
        void getWorldTransforms( Ogre::Matrix4 *xform ) const;

        void getRenderOperation( Ogre::v1::RenderOperation &op, bool casterPass );

        const Ogre::LightList &getLights( void ) const;

    private:
        Ogre::v1::RenderOperation mRenderOp;
        std::size_t mVertexBufferCapacity = 0;
        std::size_t mIndexBufferCapacity = 0;
        unsigned int mVertexCount = 0;
        unsigned int mIndexCount = 0;
    };

}  // namespace workphone::render

#endif
