#ifndef ViewportHelper_h__
#define ViewportHelper_h__

#include "WPLuabind/WPLuabindPrerequisites.hpp"
#include <Workphone/Interface/Graphics/IRenderTarget.hpp>
#include <Workphone/Interface/Graphics/IViewport.hpp>

namespace workphone
{
    class ViewportHelper
    {
    public:
        static Vector2I _getActualSize( render::IViewport *vp );
        static void     _setClearEveryFrame( render::IViewport *vp, bool clear );
        static void     _setClearEveryFrameFlags( render::IViewport *vp, bool clear, u32 buffers );

        static SmartPtr<render::IViewport> _addViewportMinArgs(
            render::IRenderTarget *rt, hash32 id, SmartPtr<render::IGraphicsCamera> camera );
        static SmartPtr<render::IViewport> _addViewportZorderArgs(
            render::IRenderTarget *rt, hash32 id, SmartPtr<render::IGraphicsCamera> camera, s32 zorder );
        static SmartPtr<render::IViewport> _addViewportAllArgs( render::IRenderTarget *rt, hash32 id,
                                                                SmartPtr<render::IGraphicsCamera> camera,
                                                                s32 ZOrder, f32 left, f32 top, f32 width,
                                                                f32 height );

        static lua_Integer getVisibilityMask( render::IViewport *vp );

        static void setVisibilityMask( render::IViewport *vp, lua_Integer mask );
    };
} // namespace workphone

#endif // ViewportHelper_h__
