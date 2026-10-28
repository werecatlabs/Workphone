#ifndef WPVehiclePrerequisites_h__
#define WPVehiclePrerequisites_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Physics/PhysicsTypes.hpp>
#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{
    class CCarController;
    class CDriveTrain;
    class CVehicleManager;
    class WheelControllerPacejka;

    constexpr physics_Num MinInput = static_cast<physics_Num>( -1.0 );
    constexpr physics_Num MaxInput = static_cast<physics_Num>( 1.0 );
    constexpr physics_Num MaxSteeringScale = static_cast<physics_Num>( 1080.0 );
    constexpr physics_Num MaxWheelForce = static_cast<physics_Num>( 1.0e8 );
    constexpr physics_Num MaxWheelVelocity = static_cast<physics_Num>( 1.0e5 );
    constexpr physics_Num MaxDebugForce = static_cast<physics_Num>( 1.0e8 );

}  // namespace workphone

namespace workphone::vehicle
{
    class DroneProp;

    class CAerofoil;
    class CAircraft;

    template <class T>
    class CAircraftAttachment;

    class Curve;
    class CAircraftBody;
    class CAircraftControlSurface;
    class EngineSimple;
    class CAircraftPropWash;
    class GroundEffect;
    class CAircraftWing;
    class EngineSimple;
    class InputController;

    class CEMotor;
    class CBatteryPackStandard;
    class CESController;
    class CGovernorUnit;
    class CFlybarlessUnit;
    class CGyroUnit;

    // Type declarations

    class HelicopterBody;
    class ClimbTransitionData;
    class TFlyBar;
    class TFoilData;
    class TFoilLookup;
    class GearTrain;
    class TLinkage;
    class Lookup;
    class VehicleParam;
    class Rotor;
    class RotorHead;
    class RotorSector;
    class Servo;
    class Surface;
    class TailLinkage;

    class FrameOfRef;
    using physics_Vec = Vector3<physics_Num>;

    // constants and Types for XMLPar unit
    extern const int ABool;
    extern const int AInt;
    extern const int ASingle;
    extern const int AVec;

    class VehicleParam
    {
    public:
        VehicleParam();

        String m_name;

        // used to specify the variable type (Float, int, bool etc)
        int m_vType;

        union
        {
            bool m_bVal;
            int m_iVal;
            float m_sVal;
        };

        physics_Vec m_vVal = physics_Vec::zero();

        String toString() const;
    };  // TParam
}  // namespace workphone::vehicle

using namespace workphone::vehicle;  // hack

#ifdef WP_PLATFORM_WIN32
#    ifndef _WP_STATIC_LIB_
#        ifdef WPVehicle_EXPORTS
#            define WPVehiclePhysics_API __declspec( dllexport )
#        else
#            define WPVehiclePhysics_API __declspec( dllimport )
#        endif  // WP_EXPORT
#    else
#        define WPVehiclePhysics_API
#    endif  // _WP_STATIC_LIB_
#else
#    define WPVehiclePhysics_API
#endif

#ifdef WP_PLATFORM_WIN32
#    ifndef _WP_STATIC_LIB_
#        ifdef WPVehicle_EXPORTS
#            define WPVehiclePhysics_API __declspec( dllexport )
#        else
#            define WPVehiclePhysics_API __declspec( dllimport )
#        endif  // WP_EXPORT
#    else
#        define WPVehiclePhysics_API
#    endif  // _WP_STATIC_LIB_
#else
#    define WPVehiclePhysics_API
#endif

#endif  // WPVehiclePrerequisites_h__
