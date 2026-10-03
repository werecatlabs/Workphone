#ifndef FrustumStateData_h__
#define FrustumStateData_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Matrix4.hpp>

namespace workphone
{

    class WPCore_API FrustumStateData : public StateData
    {
    public:
        FrustumStateData();
        ~FrustumStateData() override;

        real_Num nearClipDistance = real_Num( 0.0001 );
        real_Num farClipDistance = real_Num( 100000.0 );
        real_Num aspectRatio = real_Num( 1.33333333333333 );
        real_Num fovy = Math<real_Num>::pi() / real_Num( 4.0 );

        s32 screenWidth = 0;
        s32 screenHeight = 0;

        u32 projType = 0;
        u32 flags = 0;

        Matrix4<real_Num> viewMatrix;
        Matrix4<real_Num> projectionMatrix;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // CameraState_h__
