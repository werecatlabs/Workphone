#ifndef IAerodymanicsWind_h__
#define IAerodymanicsWind_h__

#include <Workphone/Interface/Vehicle/IVehicleComponent.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{
    namespace vehicle
    {

        /**
         * @file IAerodymanicsWind.hpp
         * @brief Abstract interface for wind models used by vehicle aerodynamics.
         *
         * @class IAerodymanicsWind
         * @ingroup Vehicle
         *
         * @details
         * `IAerodymanicsWind` exposes the minimal contract required by aerodynamic
         * subsystems to obtain wind information. Implementations may represent
         * simple constant winds, height-dependent profiles (logarithmic, power-law),
         * time-varying flows, turbulence models or stochastic gust generators.
         *
         * Implementers should ensure returned vectors are expressed in the engine's
         * world-space coordinate system and use the engine's standard units (typically
         * meters and seconds).
         *
         * @remarks
         * - The interface does not mandate how parameters (speed, direction, turbulence,
         *   etc.) are internally applied — concrete classes document their interpretation.
         * - Thread-safety and temporal interpolation behavior are implementation-defined.
         */
        class WPCore_API IAerodymanicsWind : public IVehicleComponent
        {
        public:
            /**
             * @brief Virtual destructor.
             *
             * Ensures proper cleanup of derived wind-model objects when deleted
             * through this interface.
             */
            ~IAerodymanicsWind() override;

            /**
             * @brief Set or update the wind model parameters.
             *
             * Provide the canonical parameters that describe the mean flow and
             * boundary-layer/turbulence characteristics used by the wind model.
             * Concrete implementations decide how these parameters influence the
             * computed wind (for example, as a reference value for a height profile,
             * as a mean in a stochastic model, or as inputs to an aerodynamic lookup).
             *
             * @param speed     Mean wind speed at the reference height (e.g. m/s).
             * @param direction Wind direction using the engine's angular convention
             *                  (e.g. radians). The interpretation (wind-from vs wind-to)
             *                  is implementation-specific — check the concrete class.
             * @param turb      Turbulence intensity (normalized, typical range 0.0–1.0).
             *                  Larger values indicate stronger stochastic fluctuations.
             * @param gndHt     Reference ground elevation used by height-dependent profiles
             *                  (meters). Passed value should match the height basis used
             *                  by callers when querying `getWind`.
             * @param dirOff    Directional shear or offset term (radians) used to model
             *                  change of wind direction with height.
             * @param rough     Surface roughness length (meters) for boundary-layer models.
             */
            virtual void setWind( real_Num speed, real_Num direction, real_Num turb, real_Num gndHt,
                                  real_Num dirOff, real_Num rough ) = 0;

            /**
             * @brief Evaluate instantaneous wind velocity at a given height and time.
             *
             * Combine mean flow, shear, turbulence and any time-varying components
             * to produce the instantaneous wind velocity vector in world-space.
             *
             * @note Implementations should accept the supplied `height` relative to the
             *       same reference ground elevation provided to `setWind`.
             *
             * @param height Height above the model's reference ground (meters).
             * @param time   Simulation or real time used to evaluate time-dependent effects (seconds).
             * @return A `Vector3<real_Num>` containing the wind velocity (e.g. m/s) in world-space.
             */
            virtual Vector3<real_Num> getWind( real_Num height, real_Num time ) = 0;

            /**
             * @brief Get the current air density used by the wind/aerodynamics model.
             *
             * Air density affects aerodynamic force calculations; implementations
             * may return a constant value or a height/time dependent value if they
             * model atmospheric stratification.
             *
             * @return Air density in kg/m^3 (or the project's chosen density unit).
             */
            virtual real_Num getAirDensity() const = 0;

            /**
             * @brief Set the air density value used by the model.
             *
             * Use this to override the density used for aerodynamic computations.
             *
             * @param airDensity Air density in kg/m^3 (or the project's chosen density unit).
             */
            virtual void setAirDensity( real_Num airDensity ) = 0;

            /** Macro used for runtime class registration within the engine. */
            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace vehicle
}  // namespace workphone

#endif  // IAerodymanicsWind_h__
