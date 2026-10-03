#ifndef RenderTextureState_h__
#define RenderTextureState_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>

namespace workphone
{
    class WPCore_API RenderTextureState : public StateData
    {
    public:
        RenderTextureState();
        ~RenderTextureState() override;

        WP_CLASS_REGISTER_DECL;

        AtomicWeakPtr<render::ITexture> texture;
    };
}  // namespace workphone

#endif  // RenderTextureState_h__
