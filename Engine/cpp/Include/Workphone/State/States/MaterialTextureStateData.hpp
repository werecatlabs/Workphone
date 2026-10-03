#ifndef MaterialTextureState_h__
#define MaterialTextureState_h__

#include <Workphone/State/States/StateData.hpp>

namespace workphone
{

    class WPCore_API MaterialTextureStateData : public StateData
    {
    public:
        MaterialTextureStateData();
        ~MaterialTextureStateData() override;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // MaterialTextureState_h__
