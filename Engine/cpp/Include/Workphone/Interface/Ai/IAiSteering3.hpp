#ifndef IAiSteering3_h__
#define IAiSteering3_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{

    /** A class that represents the steering of an AI agent.
     */
    class WPCore_API IAiSteering3 : public ISharedObject
    {
    public:
        enum class summing_method
        {
            weighted_average,
            prioritized,
            dithered
        };

        enum class behavior_type
        {
            none = 0x00000,
            seek = 0x00002,
            arrive = 0x00008,
            wander = 0x00010,
            separation = 0x00040,
            wall_avoidance = 0x00200,
        };

        /** Virtual destructor. */
        ~IAiSteering3() override;

        // calculates and sums the steering forces from any active behaviors
        virtual Vector3<real_Num> calculate() = 0;

        // calculates the component of the steering force that is parallel
        // with the entity heading
        virtual f32 forwardComponent() = 0;

        // calculates the component of the steering force that is perpendicular
        // with the entity heading
        virtual f32 sideComponent() = 0;

        virtual Vector3<real_Num> getTarget() const = 0;
        virtual void setTarget( const Vector3<real_Num> &target ) = 0;

        virtual Vector3<real_Num> getDirection() const = 0;

        virtual Vector3<real_Num> getPosition() const = 0;
        virtual void setPosition( const Vector3<real_Num> &position ) = 0;

        virtual void setTargetAgent1( SmartPtr<scene::IGameActor> agent ) = 0;
        virtual void setTargetAgent2( SmartPtr<scene::IGameActor> agent ) = 0;

        virtual Vector3<real_Num> getForce() const = 0;

        virtual void setSummingMethod( u32 sm ) = 0;

        virtual void seekOn() = 0;
        virtual void arriveOn() = 0;
        virtual void wanderOn() = 0;
        virtual void separationOn() = 0;
        virtual void wallAvoidanceOn() = 0;

        virtual void seekOff() = 0;
        virtual void arriveOff() = 0;
        virtual void wanderOff() = 0;
        virtual void separationOff() = 0;
        virtual void wallAvoidanceOff() = 0;

        virtual bool getSeekIsOn() = 0;
        virtual bool getArriveIsOn() = 0;
        virtual bool getWanderIsOn() = 0;
        virtual bool getSeparationIsOn() = 0;
        virtual bool getWallAvoidanceIsOn() = 0;

        virtual Array<Vector3<real_Num>> getFeelers() const = 0;

        virtual f32 getWanderJitter() const = 0;
        virtual f32 getWanderDistance() const = 0;
        virtual f32 getWanderRadius() const = 0;

        virtual f32 getSeparationWeight() const = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // IAiSteering3_h__
