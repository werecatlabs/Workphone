#ifndef CWind_h__
#define CWind_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <WPVehiclePhysics/CAircraftAttachment.hpp>
#include <Workphone/Interface/Vehicle/IAerodymanicsWind.hpp>

namespace workphone
{
    namespace vehicle
    {
        /**
         * @class CAerodymanicsWind
         * @brief Wind aerodynamics model for aircraft with turbulence simulation.
         *
         * This class implements a comprehensive wind model for aircraft aerodynamics simulation,
         * including wind speed, direction, and dynamic turbulence effects. It uses a logarithmic
         * wind profile based on surface roughness and applies frequency-based turbulence components
         * to simulate realistic atmospheric effects.
         *
         * The wind model supports two coordinate frame conventions:
         * - Conventional frame (Z vertical): Used by the aerodynamics
         * - Saracen frame (Y vertical): Used by helicopter simulations
         *
         * @note Wind speeds are based on 2-meter mean wind speed measurements and are adjusted
         *       for altitude using a logarithmic profile derived from the surface roughness length.
         *
         * @see IAerodymanicsWind, CAircraftAttachment
         */
        class WPVehiclePhysics_API CAerodymanicsWind : public CAircraftAttachment<IAerodymanicsWind>
        {
        public:
            /** @brief Constructs a wind aerodynamics model with default parameters. */
            CAerodymanicsWind();

            /** @brief Destroys the wind aerodynamics model. */
            ~CAerodymanicsWind() override;

            /**
             * @brief Configures the wind model with environmental parameters.
             *
             * Sets all wind and weather parameters for the simulation, including wind speed,
             * direction, turbulence characteristics, and surface properties.
             *
             * @param speed         Mean wind speed at 2 meters altitude (m/s).
             * @param direction     Wind direction in degrees (0-360).
             * @param turb          User-adjustable turbulence factor (0.0-1.0 or greater).
             * @param gndHt         Ground/reference height (meters).
             * @param dirOff        Direction offset for field wind information adjustment (degrees).
             * @param rough         Surface roughness length used in logarithmic wind profile (meters).
             *
             * @note This method must be called to initialize the wind model before simulation.
             */
            void setWind( real_Num speed, real_Num direction, real_Num turb, real_Num gndHt,
                          real_Num dirOff, real_Num rough ) override;

            /**
             * @brief Calculates and updates the dynamic turbulence factor for the given time step.
             *
             * Computes frequency-based turbulence components using a Fourier-like decomposition.
             * This method should be called regularly to maintain smooth turbulence variations over time.
             *
             * @param Time Elapsed time since the last turbulence calculation (seconds).
             *
             * @see m_omega, m_atude
             */
            void calcTurbulence( real_Num Time );

            /**
             * @brief Retrieves wind vector at a specified altitude in Conventional coordinate frame.
             *
             * Computes the wind vector at a given altitude and time using the logarithmic wind
             * profile with applied turbulence effects. The result is in the Conventional frame
             * where Z is vertical.
             *
             * @param height Altitude at which to calculate wind (meters).
             * @param time   Simulation time for turbulence phase calculation (seconds).
             *
             * @return Wind vector in Conventional frame (X, Y, Z) (m/s).
             *
             * @see getWindY
             */
            Vector3<real_Num> getWind( real_Num height, real_Num time ) override;

            /**
             * @brief Retrieves wind vector at a specified altitude in coordinate frame.
             *
             * Computes the wind vector at a given altitude and time using the logarithmic wind
             * profile with applied turbulence effects. The result is in the frame
             * where Y is vertical (used for helicopter simulations).
             *
             * @param height Altitude at which to calculate wind (meters).
             * @param time   Simulation time for turbulence phase calculation (seconds).
             *
             * @return Wind vector in frame (X, Y, Z) (m/s).
             *
             * @see getWind
             */
            Vector3<real_Num> getWindY( real_Num height, real_Num time );

            /**
             * @brief Outputs wind components at a specified altitude as individual values.
             *
             * Calculates and returns wind vector components in frame as individual
             * scalar values. This is an alternative interface to passWind results internally
             * for compatibility with different calling conventions.
             *
             * @param height Wind vector output format using three separate parameters.
             * @param windX  [out] X-axis wind component (m/s).
             * @param windY  [out] Y-axis wind component (m/s).
             * @param windZ  [out] Z-axis wind component (m/s).
             *
             * @see getWindY
             */
            void passWind( real_Num height, real_Num &windX, real_Num &windY, real_Num &windZ );

            /**
             * @brief Retrieves the current air density at sea level.
             *
             * @return Air density (kg/m³).
             */
            real_Num getAirDensity() const override;

            /**
             * @brief Sets the air density for aerodynamic calculations.
             *
             * @param airDensity Air density value (kg/m³).
             */
            void setAirDensity( real_Num airDensity ) override;

            /**
             * @brief Sets the operational state of the wind model.
             *
             * @param state New operational state (e.g., active, inactive, paused).
             *
             * @see State
             */
            void setState( State state ) override;

        protected:
            /**
             * @brief Initializes weather parameters from configuration.
             *
             * Loads and validates weather-related parameters, initializing the wind model
             * for simulation use.
             */
            void initWeather();

            /** @brief Von Kármán's constant used in logarithmic wind profile equation (˜0.41). */
            static const real_dNum KvonKarmen;

            /** @brief Mean wind speed at 2 meters altitude as provided by configuration (m/s). */
            real_Num m_meanWindSpeed;

            /** @brief Wind direction in degrees as provided by configuration (0-360). */
            real_Num m_meanWindDirection;

            /** @brief User-adjustable turbulence scaling factor (dimensionless). */
            real_Num m_turbulence;

            /** @brief Reference ground/sea level height (meters). */
            real_Num m_groundHeight;

            /** @brief Direction offset applied to field wind information (degrees). */
            real_Num m_directionOffset;

            /** @brief Surface roughness length (z0) for logarithmic wind profile (meters). */
            real_Num m_roughness;

            /** @brief Fraction of mean wind component in Conventional frame X-axis. */
            real_Num m_convXFactor;

            /** @brief Fraction of mean wind component in Conventional frame Y-axis. */
            real_Num m_convYFactor;

            /** @brief Cross-wind turbulence component fraction for Conventional frame X-axis. */
            real_Num m_convCrossX;

            /** @brief Cross-wind turbulence component fraction for Conventional frame Y-axis. */
            real_Num m_convCrossY;

            /** @brief Friction velocity (u*) derived from logarithmic profile equation (m/s). */
            real_Num m_uStar;

            /** @brief Phase time accumulator 1 for turbulence frequency component 1 (seconds). */
            real_Num m_phaseTime1;

            /** @brief Phase time accumulator 2 for turbulence frequency component 2 (seconds). */
            real_Num m_phaseTime2;

            /**
             * @brief Last simulation time turbulence was calculated to avoid excessive recalculation.
             *
             * Used as an optimization flag to prevent redundant turbulence calculations
             * within the same time step (seconds).
             */
            real_Num m_turbTime;

            /** @brief Instantaneous turbulence multiplier applied to mean wind (dimensionless). */
            real_Num m_kTurb;

            /** @brief Instantaneous turbulence multiplier for cross-wind component (dimensionless). */
            real_Num m_kCrossWind;

            /** @brief Current model height for wind calculation (meters). */
            real_Num m_modelHeight;

            /** @brief Last wind vector result in frame from passWind call (m/s). */
            Vector3<real_Num> m_sockWind;

            /** @brief Height of the last wind vector retrieved by passWind (meters). */
            real_Num m_sockHeight;

            /**
             * @brief Angular frequency components for Fourier-like turbulence decomposition.
             *
             * Array holds 15 frequency components (indices 1-15) used to generate
             * realistic turbulence via superposition (rad/s).
             *
             * @note Index 0 is unused; valid range is 1-15.
             */
            real_Num m_omega[16 /* range 1..15*/];

            /**
             * @brief Amplitude of each turbulence frequency component.
             *
             * Array holds 15 amplitude values (indices 1-15) corresponding to each
             * frequency in m_omega (m/s).
             *
             * @note Index 0 is unused; valid range is 1-15.
             */
            real_Num m_atude[16 /* range 1..15*/];
        };
    } // namespace vehicle
} // namespace workphone

#endif // CWind_h__
