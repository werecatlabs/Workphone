#ifndef FoilData_h__
#define FoilData_h__

#include "WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp"
#include "VecMath.hpp"
#include <string>
#include <array>
#include <memory>

namespace workphone
{
    namespace vehicle
    {
        /**
         * @brief Simple container for aerofoil polar data at a single angle.
         *
         * TFoilData stores the angle of attack and the corresponding
         * non-dimensional coefficients (lift, drag and moment) used by the
         * aerodynamic lookup tables. The moment coefficient is referenced to
         * the quarter-chord (25% chord) by convention.
         */
        class TFoilData
        {
        public:
            /// Default constructor
            TFoilData();

            /**
             * @brief Angle of attack in degrees or radians depending on calling code.
             *        (Documented units depend on the simulation configuration.)
             */
            f32 m_alpha; ///< Angle of attack (see project conventions for units)

            /**
             * @brief Lift coefficient (Cl) at m_alpha.
             */
            f32 m_cl;

            /**
             * @brief Drag coefficient (Cd) at m_alpha.
             */
            f32 m_cd;

            /**
             * @brief Moment coefficient (Cm) at m_alpha. Moment is taken about
             *        the 25% chord point by convention.
             */
            f32 m_cm;
        }; // TFoilData
    } // namespace vehicle
} // namespace workphone

#endif // FoilData_h__
