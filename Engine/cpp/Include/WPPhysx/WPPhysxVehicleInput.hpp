#ifndef WPPhysxVehicleInput_h__
#define WPPhysxVehicleInput_h__

#include <WPPhysx/WPPhysxPrerequisites.hpp>
#include <Workphone/Interface/Physics/IPhysicsVehicleInput3.hpp>

/**
 * @file WPPhysxVehicleInput.hpp
 * @brief Adapter between the engine vehicle input interface and PhysX raw input
 *        data structure.
 *
 * This file declares `PhysxVehicleInput`, which forwards digital and analog
 * control values to a `physx::PxVehicleDrive4WRawInputData` structure used by
 * the PhysX vehicle simulation. The adapter does not own the underlying
 * PhysX input structure; the caller is responsible for managing its lifetime.
 */

namespace workphone::physics
{
    /**
     * @brief Implements `IPhysicsVehicleInput3` using PhysX raw input data.
     *
     * The `PhysxVehicleInput` class provides setters and getters for both
     * digital (boolean) and analog (numeric) vehicle controls. It stores a
     * non-owning pointer to a `physx::PxVehicleDrive4WRawInputData` instance
     * and maps interface calls to that structure.
     */
    class PhysxVehicleInput : public IPhysicsVehicleInput3
    {
    public:
        /**
         * @brief Create an adapter with no bound PhysX input.
         *
         * `m_inputs` is initialized to nullptr. Use `setInputs` to bind an
         * existing PhysX raw input object before using the adapter.
         */
        PhysxVehicleInput();

        /**
         * @brief Default virtual destructor.
         */
        ~PhysxVehicleInput() override;

        /** Set the digital (on/off) accelerator input. */
        void setDigitalAccel( bool accelKeyPressed ) override;

        /** Set the digital (on/off) brake input. */
        void setDigitalBrake( bool brakeKeyPressed ) override;

        /** Set the digital (on/off) handbrake input. */
        void setDigitalHandbrake( bool handbrakeKeyPressed ) override;

        /** Set the digital (on/off) steer-left input. */
        void setDigitalSteerLeft( bool steerLeftKeyPressed ) override;

        /** Set the digital (on/off) steer-right input. */
        void setDigitalSteerRight( bool steerRightKeyPressed ) override;

        /** Get the current digital accelerator state. */
        bool getDigitalAccel() const override;

        /** Get the current digital brake state. */
        bool getDigitalBrake() const override;

        /** Get the current digital handbrake state. */
        bool getDigitalHandbrake() const override;

        /** Get the current digital steer-left state. */
        bool getDigitalSteerLeft() const override;

        /** Get the current digital steer-right state. */
        bool getDigitalSteerRight() const override;

        /**
         * @brief Set the analog accelerator value.
         *
         * Typically the range is [0,1], but the exact semantics are
         * determined by the engine and PhysX configuration.
         */
        void setAnalogAccel( physics_Num accel ) override;

        /** Set the analog brake value. */
        void setAnalogBrake( physics_Num brake ) override;

        /** Set the analog handbrake value. */
        void setAnalogHandbrake( physics_Num handbrake ) override;

        /**
         * @brief Set the analog steer value.
         *
         * Usually a signed value where negative is left and positive is
         * right (range typically -1..1).
         */
        void setAnalogSteer( physics_Num steer ) override;

        /** Get the analog accelerator value. */
        physics_Num getAnalogAccel() const override;

        /** Get the analog brake value. */
        physics_Num getAnalogBrake() const override;

        /** Get the analog handbrake value. */
        physics_Num getAnalogHandbrake() const override;

        /** Get the analog steer value. */
        physics_Num getAnalogSteer() const override;

        /** Set digital gear-up input. */
        void setGearUp( bool gearUpKeyPressed ) override;

        /** Set digital gear-down input. */
        void setGearDown( bool gearDownKeyPressed ) override;

        /** Get the digital gear-up state. */
        bool getGearUp() const override;

        /** Get the digital gear-down state. */
        bool getGearDown() const override;

        /**
         * @brief Access the underlying PhysX raw input data pointer.
         * @return Pointer to `PxVehicleDrive4WRawInputData`, or nullptr if
         *         not bound.
         */
        physx::PxVehicleDrive4WRawInputData *getInputs() const;

        /**
         * @brief Bind an existing PhysX raw input data structure.
         *
         * The adapter does not take ownership; the caller must ensure the
         * lifetime of `inputs` is valid while the adapter is used.
         *
         * @param inputs Pointer to PhysX raw input data (may be nullptr).
         */
        void setInputs( physx::PxVehicleDrive4WRawInputData *inputs );

    protected:
        /**
         * @brief Non-owning pointer to PhysX vehicle raw input structure.
         *
         * This pointer is used to forward control values to the PhysX
         * vehicle. It may be nullptr when not bound.
         */
        physx::PxVehicleDrive4WRawInputData *m_inputs;
    };
} // namespace workphone::physics

#endif // WPPhysxVehicleInput_h__
