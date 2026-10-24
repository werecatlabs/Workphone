#ifndef AnimationIKSystem_h__
#define AnimationIKSystem_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Animation/IK/TwoBoneIK.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/System/SourceBudgetContainers.hpp>

namespace workphone
{
    class ISkeleton;
    class Properties;
}  // namespace workphone

namespace workphone::animation
{
    struct WPCore_API TwoBoneIKConstraint
    {
        String id;
        String effectorBone;
        Transform3<real_Num> targetTransform;
        TwoBoneIKSettings settings;
        bool enabled = true;

        bool isValid() const;
    };

    struct WPCore_API AnimationIKSolveResult
    {
        String constraintId;
        TwoBoneIKResult result;
    };

    /** Post-animation IK pass owned by an Animator component. */
    class WPCore_API AnimationIKSystem : public SourceBudgetObject
    {
    public:
        /** Admitted commands and outputs prepared before any animation/bone writes. */
        class WPCore_API PreparedSolve : public SourceBudgetObject
        {
        public:
            const Array<AnimationIKSolveResult> &getResults() const;
            Array<AnimationIKSolveResult> takeResults() noexcept;
        private:
            friend class AnimationIKSystem;
            struct Command
            {
                SmartPtr<IBone> effector;
                Transform3<real_Num> target;
                TwoBoneIKSettings settings;
            };
            Array<Command> m_commands = makeSourceBudgetSnapshot<Command>();
            Array<AnimationIKSolveResult> m_results = makeSourceBudgetSnapshot<AnimationIKSolveResult>();
            Array<SmartPtr<IBone>> m_hierarchy = makeSourceBudgetSnapshot<SmartPtr<IBone>>();
        };
        bool addConstraint( const TwoBoneIKConstraint &constraint );
        bool updateConstraint( const TwoBoneIKConstraint &constraint );
        bool removeConstraint( const String &id );
        void clear();

        TwoBoneIKConstraint *findConstraint( const String &id );
        const TwoBoneIKConstraint *findConstraint( const String &id ) const;
        const Array<TwoBoneIKConstraint> &getConstraints() const;

        bool setTarget( const String &id, const Transform3<real_Num> &targetTransform );
        bool setPoleTarget( const String &id, const Vector3<real_Num> &poleTarget );
        bool setWeight( const String &id, f32 weight );

        /** Serializes all constraints for scene persistence and property editors. */
        SmartPtr<Properties> getProperties() const;

        /** Atomically replaces constraints from serialized properties. */
        bool setProperties( SmartPtr<Properties> properties );

        Array<AnimationIKSolveResult> solve( ISkeleton *skeleton ) const;
        PreparedSolve prepareSolve( ISkeleton *skeleton ) const;
        void solvePrepared( PreparedSolve &prepared ) const;

    private:
        Array<TwoBoneIKConstraint> m_constraints = makeSourceBudgetSnapshot<TwoBoneIKConstraint>();
    };
}  // namespace workphone::animation

#endif  // AnimationIKSystem_h__
