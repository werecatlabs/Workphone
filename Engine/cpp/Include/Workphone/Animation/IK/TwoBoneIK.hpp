#ifndef TwoBoneIK_h__
#define TwoBoneIK_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <array>

namespace workphone
{
    class IBone;
}

namespace workphone::animation
{
    /** Matches Esoterica's choice of blending the target before IK or the solved pose after IK. */
    enum class IKBlendMode
    {
        Effector,
        Pose
    };

    /** Detailed outcome for editor diagnostics and animation task debugging. */
    enum class TwoBoneIKStatus
    {
        NotSolved,
        Solved,
        InvalidEffector,
        InvalidChain,
        DegenerateChain
    };

    struct WPCore_API TwoBoneIKSettings
    {
        Vector3<real_Num> poleTarget = Vector3<real_Num>::unitZ();
        f32 chainRotationWeight = 0.0f;
        f32 blendWeight = 1.0f;
        IKBlendMode blendMode = IKBlendMode::Effector;
        bool matchTargetOrientation = true;
    };

    struct WPCore_API TwoBoneIKResult
    {
        bool success = false;
        bool targetClamped = false;
        f32 effectorError = 0.0f;
        TwoBoneIKStatus status = TwoBoneIKStatus::NotSolved;
    };

    /**
     * Analytic root/mid/effector IK solver adapted from Esoterica's TwoBoneSolver.
     * Value-space solving is exposed separately so animation jobs can run without touching a skeleton.
     */
    class WPCore_API TwoBoneIKSolver
    {
    public:
        using ChainTransforms = std::array<Transform3<real_Num>, 3>;

        static TwoBoneIKResult solve( ChainTransforms &modelSpaceTransforms,
                                      const ChainTransforms &modelSpaceReferenceTransforms,
                                      const Transform3<real_Num> &targetTransform,
                                      const TwoBoneIKSettings &settings = {} );

        static TwoBoneIKResult solve( SmartPtr<IBone> effectorBone,
                                      const Transform3<real_Num> &targetTransform,
                                      const TwoBoneIKSettings &settings = {} );

        static bool getModelSpaceTransform( SmartPtr<IBone> bone, Transform3<real_Num> &transform );
    };
}  // namespace workphone::animation

#endif  // TwoBoneIK_h__
