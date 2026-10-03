#ifndef Constraint_h__
#define Constraint_h__

#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Atomics/AtomicFloat.hpp>
#include <Workphone/Interface/Physics/PhysicsTypes.hpp>
#include <Workphone/System/Job.hpp>
#include <Workphone/Math/Transform3.hpp>

namespace workphone
{
    namespace scene
    {
        /**
         * @brief Component that represents a constraint between two rigid bodies.
         *
         * The Constraint component wraps a physics engine constraint object and
         * exposes configuration (type, motion axes, break limits) and lifecycle
         * operations used by the scene/component system to create, update and
         * destroy the underlying physics constraint.
         *
         * Typical usage:
         * - Attach to a scene object that also has one or two `Rigidbody` components.
         * - Configure axes, type and break limits via properties or programmatically.
         * - The component will create/destroy the runtime physics constraint when
         *   loaded/unloaded or when bodies change.
         *
         * Notes:
         * - The component stores weak references to the connected rigidbodies
         *   so that they can be safely released without creating reference cycles.
         * - All physics-specific operations are delegated to `physics::IPhysicsConstraint3`.
         */
        class WPCore_API Constraint : public Component
        {
        public:
            /** @brief Supported constraint types. */
            enum class Type
            {
                /** Generic 6 degree-of-freedom constraint. */
                D6,

                /** Completely fixed constraint (no relative motion). */
                Fixed,

                /** Number of enum entries. */
                Count
            };

            /** @brief Property key for the constraint type enum. */
            static const String ConstraintTypeStr;

            /** @brief Property key for connected body A. */
            static const String BodyAStr;

            /** @brief Property key for connected body B. */
            static const String BodyBStr;

            /** @brief Property key for X-axis motion mode. */
            static const String XMotionStr;

            /** @brief Property key for Y-axis motion mode. */
            static const String YMotionStr;

            /** @brief Property key for Z-axis motion mode. */
            static const String ZMotionStr;

            /** @brief Property key for swing-1 angular motion mode. */
            static const String Swing1MotionStr;

            /** @brief Property key for swing-2 angular motion mode. */
            static const String Swing2MotionStr;

            /** @brief Property key for twist angular motion mode. */
            static const String TwistMotionStr;

            /** @brief Property key for the break force threshold. */
            static const String BreakForceStr;

            /** @brief Property key for the break torque threshold. */
            static const String BreakTorqueStr;

            /** @brief Display names for each constraint type, indexed by Type enum value. */
            static const Array<String> ConstraintTypeNames;

            /** @brief Display names for D6 motion modes (Locked, Limited, Free). */
            static const Array<String> AxisMotionNames;

            /** @brief Constructor. Initializes the component with default values. */
            Constraint();

            /** @brief Destructor. Ensures the physics constraint is destroyed. */
            ~Constraint() override;

            /**
             * @copydoc Component::load
             *
             * Load configuration and create the runtime physics constraint if possible.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc Component::reload
             *
             * Reload configuration and re-create the underlying constraint if required.
             */
            void reload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc Component::unload
             *
             * Destroy the runtime physics constraint and clear any temporary state.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc Component::updateFlags
             *
             * Called when component flags change; used to respond to runtime state changes.
             *
             * @param flags New flags bitfield.
             * @param oldFlags Previous flags bitfield.
             */
            void updateFlags( u32 flags, u32 oldFlags ) override;

            /**
             * @copydoc Component::getProperties
             *
             * Returns a `Properties` object representing this constraint's configurable values
             * (type, axes, break limits, connected bodies etc.).
             *
             * @return SmartPtr to a Properties object containing the current settings.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @copydoc Component::setProperties
             *
             * Apply settings from a `Properties` object to this constraint and update the
             * runtime physics state as necessary.
             *
             * @param properties Properties to apply.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @copydoc Component::getChildObjects
             *
             * Returns any child objects owned by this component.
             *
             * @return Array of child ISharedObject pointers.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Get the first connected rigidbody (body A).
             * @return SmartPtr to the `Rigidbody` for body A, or null if not assigned.
             */
            SmartPtr<Rigidbody> getBodyA() const;

            /**
             * @brief Set the first connected rigidbody (body A).
             * @param body SmartPtr to the Rigidbody to connect as body A.
             */
            void setBodyA( SmartPtr<Rigidbody> body );

            /**
             * @brief Get the second connected rigidbody (body B).
             * @return SmartPtr to the `Rigidbody` for body B, or null if not assigned.
             */
            SmartPtr<Rigidbody> getBodyB() const;

            /**
             * @brief Set the second connected rigidbody (body B).
             * @param body SmartPtr to the Rigidbody to connect as body B.
             */
            void setBodyB( SmartPtr<Rigidbody> body );

            /**
             * @brief Get the configured constraint type.
             * @return The constraint `Type`.
             */
            Type getType() const;

            /**
             * @brief Set the constraint type.
             * @param type The new constraint `Type`.
             *
             * Changing the type may recreate the underlying physics constraint.
             */
            void setType( Type type );

            /**
             * @brief Get the underlying physics constraint object.
             * @return SmartPtr to the engine-specific `physics::IPhysicsConstraint3`.
             */
            SmartPtr<physics::IPhysicsConstraint3> getConstraint() const;

            /**
             * @brief Set the underlying physics constraint object directly.
             * @param constraint SmartPtr to an existing `physics::IPhysicsConstraint3`.
             *
             * Use with care: assigning a custom constraint replaces the component-managed
             * constraint instance.
             */
            void setConstraint( SmartPtr<physics::IPhysicsConstraint3> constraint );

            /**
             * @brief Get X axis motion configuration for a D6 constraint.
             * @return physics::D6MotionEnum representing motion type for X axis.
             */
            physics::D6MotionEnum getAxisX() const;

            /**
             * @brief Set X axis motion configuration for a D6 constraint.
             * @param axisX Motion enum value for X axis.
             */
            void setAxisX( physics::D6MotionEnum axisX );

            /**
             * @brief Get Y axis motion configuration for a D6 constraint.
             * @return physics::D6MotionEnum representing motion type for Y axis.
             */
            physics::D6MotionEnum getAxisY() const;

            /**
             * @brief Set Y axis motion configuration for a D6 constraint.
             * @param axisY Motion enum value for Y axis.
             */
            void setAxisY( physics::D6MotionEnum axisY );

            /**
             * @brief Get Z axis motion configuration for a D6 constraint.
             * @return physics::D6MotionEnum representing motion type for Z axis.
             */
            physics::D6MotionEnum getAxisZ() const;

            /**
             * @brief Set Z axis motion configuration for a D6 constraint.
             * @param axisZ Motion enum value for Z axis.
             */
            void setAxisZ( physics::D6MotionEnum axisZ );

            /**
             * @brief Get the first swing axis motion for D6 constraints.
             * @return physics::D6MotionEnum representing swing1 motion.
             */
            physics::D6MotionEnum getSwing1() const;

            /**
             * @brief Set the first swing axis motion for D6 constraints.
             * @param swing1 Motion enum value for swing1.
             */
            void setSwing1( physics::D6MotionEnum swing1 );

            /**
             * @brief Get the second swing axis motion for D6 constraints.
             * @return physics::D6MotionEnum representing swing2 motion.
             */
            physics::D6MotionEnum getSwing2() const;

            /**
             * @brief Set the second swing axis motion for D6 constraints.
             * @param swing2 Motion enum value for swing2.
             */
            void setSwing2( physics::D6MotionEnum swing2 );

            /**
             * @brief Get the twist axis motion for D6 constraints.
             * @return physics::D6MotionEnum representing twist motion.
             */
            physics::D6MotionEnum getTwist() const;

            /**
             * @brief Set the twist axis motion for D6 constraints.
             * @param twist Motion enum value for twist.
             */
            void setTwist( physics::D6MotionEnum twist );

            /**
             * @brief Get the force threshold at which the constraint will break.
             * @return Break force (units depend on physics engine) or max value if unbounded.
             */
            real_Num getBreakForce() const;

            /**
             * @brief Set the force threshold at which the constraint will break.
             * @param breakForce Force value above which the constraint should break.
             */
            void setBreakForce( real_Num breakForce );

            /**
             * @brief Get the torque threshold at which the constraint will break.
             * @return Break torque (units depend on physics engine) or max value if unbounded.
             */
            real_Num getBreakTorque() const;

            /**
             * @brief Set the torque threshold at which the constraint will break.
             * @param breakTorque Torque value above which the constraint should break.
             */
            void setBreakTorque( real_Num breakTorque );

            /**
             * @brief Ensure the runtime physics state reflects this component's settings.
             *
             * This will create, update or destroy the underlying physics constraint as needed
             * to match the current configuration and connected rigidbodies.
             */
            void updatePhysicsState();

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Create and configure the underlying physics constraint.
             *
             * This method queries the connected rigidbodies and the configured settings
             * (type, axes, break limits) and constructs the appropriate `physics::IPhysicsConstraint3`
             * instance.
             */
            void createConstraint();

            /**
             * @brief Destroy the underlying physics constraint if it exists.
             *
             * Called during unload or when configuration changes that require recreation.
             */
            void destroyConstraint();

            /**
             * @brief Handle FSM events for this component.
             *
             * Overrides Component::handleComponentEvent to respond to lifecycle/state changes.
             *
             * @param state Current state identifier.
             * @param eventType Event that occurred.
             * @return FSMReturnType result of handling the event.
             */
            FSMReturnType handleComponentEvent( u32 state, FSMEvent eventType ) override;

            /// The engine-specific physics constraint managed by this component.
            SmartPtr<physics::IPhysicsConstraint3> m_constraint;

            /// Weak reference to the first connected rigidbody (body A).
            WeakPtr<Rigidbody> m_body0;

            /// Weak reference to the second connected rigidbody (body B).
            WeakPtr<Rigidbody> m_body1;

            /// Configured constraint type.
            Type m_type = Type::D6;

            /// Per-axis motion settings for D6 constraints.
            physics::D6MotionEnum m_axisX = physics::D6MotionEnum::eFREE;
            physics::D6MotionEnum m_axisY = physics::D6MotionEnum::eFREE;
            physics::D6MotionEnum m_axisZ = physics::D6MotionEnum::eFREE;

            /// Swing/twist settings for angular motion control on D6 constraints.
            physics::D6MotionEnum m_swing1 = physics::D6MotionEnum::eFREE;
            physics::D6MotionEnum m_swing2 = physics::D6MotionEnum::eFREE;
            physics::D6MotionEnum m_twist = physics::D6MotionEnum::eFREE;

            /// Limits at which the constraint should break. Default is unbounded (max).
            real_Num m_breakForce = std::numeric_limits<real_Num>::max();
            real_Num m_breakTorque = std::numeric_limits<real_Num>::max();
        };
    }  // namespace scene
}  // namespace workphone

#endif  // Constraint_h__
