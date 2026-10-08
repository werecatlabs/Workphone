#ifndef CCarController_h__
#define CCarController_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <WPVehiclePhysics/CVehicleController.hpp>
#include <Workphone/Vehicle/Vehicle.hpp>
#include <Workphone/Core/FixedArray.hpp>

namespace workphone
{
    class WPVehiclePhysics_API CCarController : public CVehicleController<Vehicle>
    {
    public:
        CCarController();
        CCarController( const CCarController &other ) = delete;
        ~CCarController() override;

        /** Used to update object. */
        void update() override;

        /** Used to update object. */
        void postUpdate() override;

        void loadDefaults();
        void reset() override;

        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;

        SmartPtr<IWheelComponent> getWheelController( u32 index ) const override;

        /** @copydoc IVehicleController::setState */
        void setState( State state ) override;

        /** @copydoc IVehicleController::getState */
        State getState() const override;

        SmartPtr<IDriveTrain> getDriveTrain() const override;

        void setDriveTrain( SmartPtr<IDriveTrain> driveTrain ) override;

        VehicleDriveType getDriveType() const override;

        void setDriveType( VehicleDriveType driveType ) override;

        SmartPtr<Properties> getProperties() const override;
        void                 setProperties( SmartPtr<Properties> properties ) override;

        physics_Num getEditSteeringScale() const;
        void        setEditSteeringScale( physics_Num editSteeringScale );

        physics_Num getPlaySteeringScale() const;
        void        setPlaySteeringScale( physics_Num playSteeringScale );

        physics_Num getDefaultMass() const;
        void        setDefaultMass( physics_Num defaultMass );

        WP_CLASS_REGISTER_DECL;

    protected:
        SmartPtr<IDriveTrain>                    m_driveTrain; ///< The drive train of the vehicle.
        FixedArray<SmartPtr<IWheelComponent>, 4> m_wheels;
        VehicleDriveType                         m_driveType =
            VehicleDriveType::RearWheelDrive; ///< The drive type configuration.
        physics_Num m_editSteeringScale = static_cast<physics_Num>( 70.0 );
        bool m_keyboardInput = true;
        physics_Num m_steeringRate = 0;
        physics_Num m_steeringWheelbase = 0;
        physics_Num m_steeringAcceleration = 0;
        physics_Num m_playSteeringScale = static_cast<physics_Num>( 50.0 );
        physics_Num m_defaultMass = static_cast<physics_Num>( 1000.0 );
    };
} // namespace workphone

#endif // CCarController_h__
