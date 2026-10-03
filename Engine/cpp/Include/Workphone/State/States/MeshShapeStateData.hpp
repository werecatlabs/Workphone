#ifndef MeshShapeState_h__
#define MeshShapeState_h__

#include <Workphone/State/States/StateData.hpp>

namespace workphone
{
    class WPCore_API MeshShapeStateData : public StateData
    {
    public:
        MeshShapeStateData();
        ~MeshShapeStateData() override;

        WP_CLASS_REGISTER_DECL;

        SmartPtr<IMeshResource> meshResource;
        bool convex = false;
    };
}  // namespace workphone

#endif  // MeshShapeState_h__
