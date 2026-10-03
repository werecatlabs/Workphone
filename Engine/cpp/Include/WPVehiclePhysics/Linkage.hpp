#ifndef Linkage_h__
#define Linkage_h__

#include "WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp"
#include "WPVehiclePhysics/FrameOfRef.hpp"
#include "VecMath.hpp"
#include <string>
#include <array>
#include <memory>

namespace workphone
{
    namespace vehicle
    {
        /**
         * @brief Represents the control linkage between the swashplate and the
         *        rotor head (and flybar where present).
         *
         * TLinkage stores the swashplate attitude and the mixing/dimensional
         * coefficients used to convert pilot/flight-control inputs into blade
         * pitch commands. Values are held in simple POD members to keep the
         * class lightweight for use in simulation loops.
         */
        class TLinkage
        {
        public:
            /// Default constructor
            TLinkage();

            /// Attitude of the swashplate expressed in the ground (body) frame
            FrameOfRef m_swashFrame;

            /**
             * @brief Mixing coefficients used to compute blade pitch inputs from
             *        swashplate and flybar motion.
             *
             * m_swashToMainMix: contribution from swash to main blade cyclic
             * m_swashToFBMix: contribution from swash to flybar cyclic
             * m_fbToMainMix: contribution from flybar to main blade cyclic
             */
            real_dNum m_swashToMainMix = 0.0; ///< Swash -> main cyclic coupling
            real_dNum m_swashToFBMix = 0.0;   ///< Swash -> flybar cyclic coupling
            real_dNum m_fbToMainMix = 0.0;    ///< Flybar -> main cyclic mixing

            /**
             * @brief Swashplate travel limits used to convert control outputs
             *        (e.g., transmitter commands) to actual swash deflections.
             */
            real_dNum m_maxSwashEle = 0.0; ///< Max swash elevator deflection (mm or model units)
            real_dNum m_maxSwashAil = 0.0; ///< Max swash aileron deflection (mm or model units)

            /**
             * @brief Conversion factor from vertical swash displacement (mm)
             *        to main rotor collective pitch (radians per mm).
             */
            real_dNum m_collectivePerMM = 0.0;

            /**
             * @brief Current swashplate angular deflections relative to the
             *        mainshaft. Positive sign convention shown in comments.
             */
            real_dNum m_swashEleAngle = 0.0; ///< Elevator angular deflection (+ = nose down command)
            real_dNum m_swashAilAngle = 0.0; ///< Aileron angular deflection (+ = right roll command)
        }; // TLinkage

    } // namespace vehicle
} // namespace workphone

#endif // Linkage_h__
