#ifndef PhysicsTypes_h__
#define PhysicsTypes_h__

#include <Workphone/WorkphonePrerequisites.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Specifies how forces are applied to rigid bodies.
         *
         * This enum defines different modes for applying forces to rigid bodies,
         * affecting how the force interacts with the body's mass and current state.
         */
        enum class ForceModeEnum : u8
        {
            /**
             * @brief Adds a continuous force to the rigidbody, using its mass.
             *
             * The force is applied continuously over time, scaled by the body's mass.
             * This is useful for constant forces like gravity or thrust.
             */
            Force = 0,

            /**
             * @brief Adds an instant force impulse to the rigidbody, using its mass.
             *
             * The force is applied as an instantaneous impulse, scaled by the body's mass.
             * This is useful for sudden impacts or explosions.
             */
            Impulse = 1,

            /**
             * @brief Adds an instant velocity change to the rigidbody, ignoring its mass.
             *
             * Directly modifies the body's velocity without considering its mass.
             * This is useful for precise velocity control.
             */
            VelocityChange = 2,

            /**
             * @brief Adds a continuous acceleration to the rigidbody, ignoring its mass.
             *
             * Applies a constant acceleration regardless of the body's mass.
             * This is useful for uniform acceleration effects.
             */
            Acceleration = 5
        };

        /**
         * @brief Flags that control the behavior of physics constraints.
         *
         * These flags determine how constraints behave in the physics simulation,
         * affecting their interaction with actors and the simulation itself.
         */
        enum class ConstraintFlagEnum : u32
        {
            /** Whether the constraint is broken */
            eBROKEN = 1 << 0,

            /** Whether actor1 should get projected to actor0 for this constraint */
            ePROJECT_TO_ACTOR0 = 1 << 1,

            /** Whether actor0 should get projected to actor1 for this constraint */
            ePROJECT_TO_ACTOR1 = 1 << 2,

            /** Whether the actors should get projected for this constraint */
            ePROJECTION = ePROJECT_TO_ACTOR0 | ePROJECT_TO_ACTOR1,

            /** Whether contacts should be generated between the constrained objects */
            eCOLLISION_ENABLED = 1 << 3,

            /** Whether this constraint should generate force reports */
            eREPORTING = 1 << 4,

            /** Whether this constraint should be visualized */
            eVISUALIZATION = 1 << 5,

            /** Whether limits for drive strength are forces rather than impulses */
            eDRIVE_LIMITS_ARE_FORCES = 1 << 6,

            /** Legacy compatibility flag for 3.3 */
            eDEPRECATED_32_COMPATIBILITY = 1 << 7,

            /** Perform preprocessing for improved accuracy on D6 Slerp Drive */
            eIMPROVED_SLERP = 1 << 8
        };

        /**
         * @brief Defines the axes for D6 joint motion and constraints.
         *
         * This enum specifies the different degrees of freedom for a D6 joint,
         * including both linear and angular motion axes.
         */
        enum class D6AxisEnum : u8
        {
            /** Motion along the X axis */
            eX = 0,

            /** Motion along the Y axis */
            eY = 1,

            /** Motion along the Z axis */
            eZ = 2,

            /** Motion around the X axis (twist) */
            eTWIST = 3,

            /** Motion around the Y axis (swing) */
            eSWING1 = 4,

            /** Motion around the Z axis (swing) */
            eSWING2 = 5,

            /** Total number of axes */
            eCOUNT = 6
        };

        /**
         * @brief Specifies which actor in a joint is being referenced.
         *
         * Used to identify which actor in a joint constraint is being operated on.
         */
        enum class JointActorIndexEnum : u8
        {
            /** First actor in the joint */
            eACTOR0,

            /** Second actor in the joint */
            eACTOR1,

            /** Total number of actors */
            COUNT
        };

        /**
         * @brief Defines the drive types for D6 joints.
         *
         * Specifies how different degrees of freedom in a D6 joint can be driven,
         * including both linear and angular motion.
         */
        enum class D6DriveEnum : u8
        {
            /** Drive along the X-axis */
            eX = 0,

            /** Drive along the Y-axis */
            eY = 1,

            /** Drive along the Z-axis */
            eZ = 2,

            /** Drive of displacement from the X-axis */
            eSWING = 3,

            /** Drive of the displacement around the X-axis */
            eTWIST = 4,

            /** Drive of all three angular degrees along a SLERP-path */
            eSLERP = 5,

            /** Total number of drive types */
            eCOUNT = 6
        };

        /**
         * @brief Specifies the motion type for D6 joint degrees of freedom.
         *
         * Defines how each degree of freedom in a D6 joint can move,
         * from completely locked to completely free.
         */
        enum class D6MotionEnum : u8
        {
            /** The DOF is locked, it does not allow relative motion */
            eLOCKED,

            /** The DOF is limited, it only allows motion within a specific range */
            eLIMITED,

            /** The DOF is free and has its full range of motion */
            eFREE
        };

        /**
         * @brief Flags that control the behavior of physics actors.
         *
         * These flags determine how actors behave in the physics simulation,
         * affecting their interaction with the scene and other actors.
         */
        enum class ActorFlagEnum : u32
        {
            /** Enable debug renderer for this actor */
            eVISUALIZATION = ( 1 << 0 ),

            /** Disables scene gravity for this actor */
            eDISABLE_GRAVITY = ( 1 << 1 ),

            /** Enables sleep/wake notifications for this actor */
            eSEND_SLEEP_NOTIFIES = ( 1 << 2 ),

            /** Disables simulation for the actor */
            eDISABLE_SIMULATION = ( 1 << 3 )
        };

        /**
         * @brief Flags that control the behavior of rigid bodies.
         *
         * These flags determine how rigid bodies behave in the physics simulation,
         * affecting their motion, collision detection, and interaction with other bodies.
         */
        enum class RigidBodyFlagEnum : u32
        {
            /** Enable kinematic mode for the body */
            eKINEMATIC = ( 1 << 0 ),

            /** Use kinematic target transform for scene queries */
            eUSE_KINEMATIC_TARGET_FOR_SCENE_QUERIES = ( 1 << 1 ),

            /** Enable continuous collision detection for the body */
            eENABLE_CCD = ( 1 << 2 ),

            /** Enable friction in continuous collision detection */
            eENABLE_CCD_FRICTION = ( 1 << 3 )
        };

        /**
         * @brief Flags that control the behavior of D6 joint drives.
         *
         * These flags determine how drives in D6 joints behave,
         * affecting their response to forces and motion.
         */
        enum class D6JointDriveFlagEnum : u8
        {
            /** Drive spring is for the acceleration at the joint rather than the force */
            eACCELERATION = 1
        };

        /**
         * @brief Structure for physics collision filtering data.
         *
         * This structure contains the data used for collision filtering,
         * allowing control over which objects can collide with each other.
         */
        struct FilterData
        {
            /**
             * @brief Default constructor.
             *
             * Initializes all filter words to zero.
             */
            FilterData()
            {
                word0 = word1 = word2 = word3 = 0;
            }

            /**
             * @brief Constructor to set filter data initially.
             *
             * @param w0 First filter word
             * @param w1 Second filter word
             * @param w2 Third filter word
             * @param w3 Fourth filter word
             */
            FilterData( u32 w0, u32 w1, u32 w2, u32 w3 ) :
                word0( w0 ),
                word1( w1 ),
                word2( w2 ),
                word3( w3 )
            {
            }

            /**
             * @brief Resets the structure to the default state.
             *
             * Sets all filter words to zero.
             */
            void setToDefault()
            {
                *this = FilterData();
            }

            /** First filter word */
            u32 word0;

            /** Second filter word */
            u32 word1;

            /** Third filter word */
            u32 word2;

            /** Fourth filter word */
            u32 word3;
        };

        /**
         * @brief Defines the spatial partitioning method for the physics scene.
         */
        enum class SpatialPartitioningMethodEnum : u8
        {
            None = 0,
            Grid,
            Octree,
            BVH,
            Count
        };

        /**
         * @brief Options for spatial partitioning behavior.
         */
        struct SpatialPartitioningOptions
        {
            real_Num updateRateMultiplier = 1.0f;
            real_Num sleepThreshold = 0.5f;
            u32 updateFrequency = 1;
        };

        /**
         * @brief Strategy for updating contact manifolds.
         */
        enum class ContactUpdateStrategyEnum : u8
        {
            Always = 0,
            Fixed,
            Distance,
            Count
        };

        /**
         * @brief Options for contact manifold update behavior.
         */
        struct ContactOptions
        {
            ContactUpdateStrategyEnum strategy = ContactUpdateStrategyEnum::Always;
            real_Num separationThreshold = 0.01f;
            u32 fixedUpdateFrequency = 1;
            real_Num distanceFrequencyScale = 1.0f;
        };

        /**
         * @brief How two materials' friction coefficients combine into one.
         *
         * Mirrors the C wp_friction_combine_mode. Values match the C enum so the
         * WPPhysics (C-backed) material implementation can cast freely between them.
         */
        enum class FrictionCombineMode : u8
        {
            Average = 0,
            Min = 1,
            Max = 2,
            Multiply = 3
        };

        /**
         * @brief How two materials' restitution coefficients combine into one.
         *
         * Mirrors the C wp_restitution_combine_mode.
         */
        enum class RestitutionCombineMode : u8
        {
            Average = 0,
            Min = 1,
            Max = 2,
            Multiply = 3
        };
    }  // end namespace physics
}  // namespace workphone

#endif  // PhysicsTypes_h__
