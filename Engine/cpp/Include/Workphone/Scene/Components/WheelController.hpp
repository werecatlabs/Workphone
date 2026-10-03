#ifndef WheelController_h__
#define WheelController_h__

#include <Workphone/Scene/Components/Component.hpp>

namespace workphone
{
    namespace scene
    {
        /**
         * @brief Component that manages wheel physics and behavior for vehicles.
         *
         * The WheelController class provides a comprehensive interface for managing
         * wheel physics including suspension, tire friction, steering, and power transmission.
         * It acts as a bridge between the scene component system and the underlying
         * vehicle physics implementation.
         *
         * @details This component handles:
         * - Wheel geometry (radius, position)
         * - Suspension system (spring rate, damping, travel distance)
         * - Tire friction model (forward and sideways slip characteristics)
         * - Steering configuration
         * - Power transmission settings
         *
         * @note This class inherits from Component and follows the standard
         *       component lifecycle (load/unload/properties).
         *
         * @see Component
         * @see vehicle::IWheelComponent
         *
         * @author [Author Name]
         * @version 1.0
         * @since [Version Number]
         */
        class WPCore_API WheelController : public Component
        {
        public:
            static const String massFractionStr;
            static const String radiusStr;
            static const String wheelDampingStr;
            static const String suspensionDistanceStr;
            static const String springRateStr;
            static const String suspensionDamperStr;
            static const String targetPositionStr;
            static const String forwardExtremumSlipStr;
            static const String forwardExtrememValueStr;
            static const String forwardAsymptoteSlipStr;
            static const String forwardAsymptoteValueStr;
            static const String forwardStiffnessStr;
            static const String sidewaysExtremumSlipStr;
            static const String sidewaysExtrememValueStr;
            static const String sidewaysAsymptoteSlipStr;
            static const String sidewaysAsymptoteValueStr;
            static const String sidewaysStiffnessStr;
            static const String isSteeringWheelStr;
            static const String tireModelStr;
            static const String resetStr;

            /**
             * @brief Default constructor.
             *
             * Initializes the wheel controller with default parameter values.
             * Sets up reasonable defaults for wheel radius, mass fraction,
             * suspension characteristics, and tire friction parameters.
             */
            WheelController();

            /**
             * @brief Virtual destructor.
             *
             * Ensures proper cleanup of resources and calls the base class destructor.
             */
            ~WheelController() override;

            /** @copydoc Component::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Component::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Component::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc Component::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** @copydoc Component::getChildObjects */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Gets the underlying wheel component interface.
             *
             * @return SmartPtr<vehicle::IWheelComponent> Pointer to the wheel component interface,
             *         or nullptr if not initialized.
             */
            SmartPtr<vehicle::IWheelComponent> getWheelController() const;

            /**
             * @brief Sets the underlying wheel component interface.
             *
             * @param wheelController Smart pointer to the wheel component implementation.
             *                       Can be nullptr to clear the current controller.
             */
            void setWheelController( SmartPtr<vehicle::IWheelComponent> wheelController );

            /**
             * @brief Gets the mass fraction of the wheel relative to the vehicle.
             *
             * The mass fraction determines how much of the vehicle's total mass
             * is attributed to this wheel for physics calculations.
             *
             * @return f32 Mass fraction value (typically between 0.0 and 1.0).
             */
            f32 getMassFraction() const;

            /**
             * @brief Sets the mass fraction of the wheel.
             *
             * @param massFraction Mass fraction value. Should typically be between 0.0 and 1.0,
             *                    where 0.25 would represent 1/4 of the vehicle's mass.
             */
            void setMassFraction( f32 massFraction );

            /**
             * @brief Gets the wheel radius in world units.
             *
             * @return f32 Wheel radius value.
             */
            f32 getRadius() const;

            /**
             * @brief Sets the wheel radius.
             *
             * The radius affects the wheel's rolling behavior, contact patch size,
             * and overall vehicle dynamics.
             *
             * @param radius Wheel radius in world units. Must be greater than 0.
             */
            void setRadius( f32 radius );

            /**
             * @brief Gets the wheel damping coefficient.
             *
             * @return f32 Current wheel damping value.
             */
            f32 getWheelDamping() const;

            /**
             * @brief Sets the wheel damping coefficient.
             *
             * Wheel damping affects how quickly the wheel's rotation velocity
             * decreases when no torque is applied. Higher values create more
             * resistance to wheel spin.
             *
             * @param wheelDamping Damping coefficient. Higher values = more damping.
             */
            void setWheelDamping( f32 wheelDamping );

            /**
             * @brief Gets the maximum suspension travel distance.
             *
             * @return f32 Suspension travel distance in world units.
             */
            f32 getSuspensionDistance() const;

            /**
             * @brief Sets the maximum suspension travel distance.
             *
             * This defines how far the wheel can move up and down relative
             * to its rest position. Affects ride comfort and handling.
             *
             * @param suspensionDistance Maximum travel distance in world units.
             *                          Must be greater than 0.
             */
            void setSuspensionDistance( f32 suspensionDistance );

            /**
             * @brief Resets the wheel controller to its default state.
             *
             * This method reinitializes all wheel parameters to their default values
             * and clears any accumulated state from the physics simulation.
             */
            void reset();

            /**
             * @brief Gets the current tire model used for simulation.
             * @return The active tire model
             */
            TireModel getTireModel() const;

            /**
             * @brief Sets the tire model to use for simulation.
             * @param tireModel The tire model to activate
             */
            void setTireModel( TireModel tireModel );

            WP_CLASS_REGISTER_DECL;

        protected:
            /** @brief Pointer to the underlying wheel component implementation. */
            SmartPtr<vehicle::IWheelComponent> m_wheelController;

            /** @brief Mass fraction of this wheel relative to the total vehicle mass (default: 0.05). */
            f32 m_massFraction = 0.05f;

            /** @brief Wheel radius in world units (default: 0.5). */
            f32 m_radius = 0.5f;

            /** @brief Wheel rotation damping coefficient (default: 1.0). */
            f32 m_wheelDamping = 1.0f;

            /** @brief Maximum suspension travel distance in world units (default: 0.3). */
            f32 m_suspensionDistance = 0.3f;

            /** @brief Suspension spring rate - force per unit compression (default: 1.0). */
            f32 m_springRate = 1.0f;

            /** @brief Suspension damper coefficient - resistance to compression/extension velocity
             * (default: 1.0). */
            f32 m_suspensionDamper = 1.0f;

            /** @brief Target position for the suspension in world coordinates (default: 0.0). */
            f32 m_targetPosition = 0.0f;

            /** @brief Forward slip ratio at which maximum tire force is achieved (default: 10.0). */
            f32 m_forwardExtremumSlip = 10.0f;

            /** @brief Maximum forward tire force value at extremum slip (default: 10.0). */
            f32 m_forwardExtrememValue = 10.0f;

            /** @brief Forward slip ratio at which tire force approaches asymptotic value
             * (default: 10.0). */
            f32 m_forwardAsymptoteSlip = 10.0f;

            /** @brief Asymptotic forward tire force value at high slip ratios (default: 10.0). */
            f32 m_forwardAsymptoteValue = 10.0f;

            /** @brief Forward tire stiffness - affects grip and responsiveness (default: 10.0). */
            f32 m_forwardStiffness = 10.0f;

            /** @brief Sideways slip angle at which maximum lateral tire force is achieved
             * (default: 1.0). */
            f32 m_sidewaysExtremumSlip = 1.0f;

            /** @brief Maximum sideways tire force value at extremum slip (default: 1.0). */
            f32 m_sidewaysExtrememValue = 1.0f;

            /** @brief Sideways slip angle at which tire force approaches asymptotic value
             * (default: 1.0). */
            f32 m_sidewaysAsymptoteSlip = 1.0f;

            /** @brief Asymptotic sideways tire force value at high slip angles (default: 1.0). */
            f32 m_sidewaysAsymptoteValue = 1.0f;

            /** @brief Sideways tire stiffness - affects cornering grip (default: 1.0). */
            f32 m_sidewaysStiffness = 1.0f;

            /** @brief Current steering angle applied to this wheel in radians (default: 1.0). */
            f32 m_steeringAngle = 1.0f;

            TireModel m_tireModel = TireModel::Simple;  ///< Current tire model for simulation

            /** @brief Flag indicating whether this wheel can be steered (default: false). */
            bool m_isSteeringWheel = false;

            /** @brief Flag indicating whether this wheel receives power from the engine (default:
             * false). */
            bool m_isPoweredWheel = false;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // WheelController_h__
