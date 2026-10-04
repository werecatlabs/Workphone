#ifndef _FBPhysics2Defs_H
#define _FBPhysics2Defs_H

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>  // hack

#ifdef WP_PLATFORM_WIN32
#    ifndef _WP_STATIC_LIB_
#        ifdef WPPhysics_EXPORTS
#            define WPPhysics_API __declspec( dllexport )
#        else
#            define WPPhysics_API __declspec( dllimport )
#        endif  // WP_EXPORT
#    else
#        define WPPhysics_API
#    endif  // _WP_STATIC_LIB_
#else
#    define WPPhysics_API
#endif

#define _INF_BREAK( value ) \
    if( value != value )    \
    {                       \
        _asm int 3          \
    }
#define _BREAK( value ) \
    if( value )         \
    {                   \
        _asm int 3      \
    }

// Forward declaration for WorkphonePhysics collision shape
struct wp_collision_shape;

namespace workphone
{
    static const u32 FPF_ENABLE = ( 1 << 0 );          /* 0x00 */
    static const u32 FPF_ENABLECOLLISION = ( 1 << 1 ); /* 0x01 */

    enum PhysicsBodyTypes
    {
        PBT_PARTICLE,
        PBT_RIGID,

        PBT_COUNT
    };

    enum PhysicsParticleTypes
    {
        PPT_PROJECTILE,
        PPT_FLUID,

        PPT_COUNT
    };

    namespace physics
    {
        enum
        {
            dContactMu2 = 0x001,
            dContactFDir1 = 0x002,
            dContactBounce = 0x004,
            dContactSoftERP = 0x008,
            dContactSoftCFM = 0x010,
            dContactMotion1 = 0x020,
            dContactMotion2 = 0x040,
            dContactSlip1 = 0x080,
            dContactSlip2 = 0x100,

            dContactApprox0 = 0x0000,
            dContactApprox1_1 = 0x1000,
            dContactApprox1_2 = 0x2000,
            dContactApprox1 = 0x3000
        };

        // forward declarations
        class CRigidBody2;

        class BU_Joint;
        class ContactJoint;

        // forward declarations
        class Particle2;

        class CollisionRecord;

        class Solver2;

        class CollisionManager;

        class CBoxShape2;
        class CPhysicsVehicleWheel;
    }  // namespace physics
}  // namespace workphone

#endif
