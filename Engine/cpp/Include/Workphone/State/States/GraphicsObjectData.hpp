#ifndef GraphicsObjectState_h__
#define GraphicsObjectState_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Interface/Graphics/IGraphicsObject.hpp>
#include <Workphone/Math/AABB3.hpp>

namespace workphone
{

    class WPCore_API GraphicsObjectData : public StateData
    {
    public:
        GraphicsObjectData();
        GraphicsObjectData( u32 poolTypeId );
        ~GraphicsObjectData() override;

        AABB3<real_Num> localAABB;
        hash_type renderTechnique = 0;

        u32 visibilityMask = std::numeric_limits<u32>::max();
        u32 renderQueueGroup = (u32)RenderQueueGroupID::RENDER_QUEUE_MAIN;
        u32 flags = render::IGraphicsObject::visibleFlag | render::IGraphicsObject::castShadowsFlag |
                    render::IGraphicsObject::receiveShadowsFlag;
        u32 zorder = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // GraphicsObjectState_h__
