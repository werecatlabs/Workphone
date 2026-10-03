#ifndef MultiRotorProp_h__
#define MultiRotorProp_h__

#include "WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp"
#include "Workphone/Interface/Memory/ISharedObject.hpp"
#include "Workphone/Math/Ray3.hpp"

namespace workphone
{
    namespace vehicle
    {
        /**
         * @brief Represents a single rotor/propeller used on multirotor drones.
         *
         * DroneProp encapsulates geometry, state and simple aerodynamic
         * calculations (thrust, inflow, ground effect) for a single prop.
         * It is designed to be lightweight and used as a shared object by the
         * vehicle simulation.
         */
        class WPVehiclePhysics_API DroneProp : public ISharedObject
        {
        public:
            /// Default constructor
            DroneProp();
            /// Virtual destructor from ISharedObject
            ~DroneProp() override;

            /**
             * @brief Initialise the prop with an identifier (e.g. for debugging
             *        or user data lookups).
             * @param objectID Identifier string for this prop instance.
             */
            void initialise( const std::string &objectID );

            /// Perform any internal initialisation required before simulation
            void init();

            /**
             * @brief Calculate thrust and related state for this prop over the
             *        timestep dt. Updates internal m_thrust and inflow values.
             * @param dt Time step (seconds).
             */
            void calcThrust( const double &dt );

            /**
             * @brief Estimate ground effect based on a world-space position.
             * @param position World-space point used to compute ground effect.
             */
            void calcGroundEffect( const Vector3<real_Num> &position );

            /**
             * @brief Provide low-level inputs (e.g. rpm) to the prop for the
             *        current timestep and update internal dynamics.
             * @param rpm Rotational speed (revolutions per minute or model units).
             * @param dt Time step (seconds).
             */
            void input( real_Num rpm, const double &dt );

            /* Position and identification accessors */
            Vector3<real_Num> getLocalPos() const;
            void              setLocalPos( const Vector3<real_Num> &localPos );

            int  getDebugId() const;
            void setDebugId( int debugId );

            /* Geometry and performance parameters */
            real_Num getDiameter() const;
            void     setDiameter( real_Num diameter );

            real_Num getPitch() const;
            void     setPitch( real_Num pitch );

            real_Num getMaxThrust() const;
            void     setMaxThrust( real_Num maxThrust );

            /* Index within a vehicle or prop array */
            s32  getIndex() const;
            void setIndex( s32 index );

            /* Multipliers used in thrust/inflow scaling */
            real_Num getVoMultiplier() const;
            void     setVoMultiplier( real_Num voMultiplier );

            real_Num getGroundEffectMultiplier() const;
            void     setGroundEffectMultiplier( real_Num groundEffectMultiplier );

            real_Num getThrustMultiplier() const;
            void     setThrustMultiplier( real_Num thrustMultiplier );

            /* Debugging helpers */
            bool getUseDebugPos() const;
            void setUseDebugPos( bool useDebugPos );

            /* Runtime outputs */
            real_Num getThrust() const;
            void     setThrust( real_Num thrust );

            /// Populate user-specific data after creation
            void setupUserData();

            /* Local position of the propeller relative to vehicle origin */
            Vector3<real_Num> m_localPos;
            Vector3<real_Num> m_worldPos; ///< Cached world-space position

            /* Geometric parameters */
            real_Num m_diameter; ///< Prop diameter
            real_Num m_pitch;    ///< Propeller pitch

            /* Aerodynamic/dynamic state */
            real_Num m_vo;           ///< Induced velocity (inflow)
            real_Num m_thrust;       ///< Current thrust output
            real_Num m_propInflow;   ///< Prop inflow term used in calculations
            real_Num m_groundEffect; ///< Ground effect factor applied to thrust

            /* Limits and scaling */
            real_Num m_maxThrust;              ///< Maximum thrust capability
            real_Num m_thrustMultiplier;       ///< Global thrust scaling factor
            real_Num m_voMultiplier;           ///< Inflow scaling factor
            real_Num m_groundEffectMultiplier; ///< Ground effect scaling

            /* Identification and ordering */
            s32 m_debugId; ///< Debug id for logging/visualisation
            s32 m_index;   ///< Index within owning vehicle

            bool m_bUseDebugPos; ///< If true, use debug position override
        };
    } // namespace vehicle
} // namespace workphone

#endif // MultiRotorProp_h__
