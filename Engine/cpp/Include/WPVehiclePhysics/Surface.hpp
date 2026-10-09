#ifndef Surface_h__
#define Surface_h__

#include "WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp"
#include "VecMath.hpp"
#include <string>
#include <array>
#include <memory>

namespace workphone::vehicle
{
    /**
         * @brief Represents a single aerodynamic surface element used for lift/drag
         *        calculations (for example a blade element or control surface panel).
         *
         * The class stores unit direction vectors, local flow and force vectors
         * and scalar aerodynamic properties such as lift, drag, coefficients and
         * geometric properties. Many members are public for plain-old-data style
         * usage in performance-sensitive code.
         */
    class TSurface
    {
    public:
        /// Default constructor
        TSurface();

        /// Non-copyable: disable copy construction to avoid accidental sharing
        TSurface(const TSurface &other) = delete;

        /**
             * @brief Get the computed lift force for this surface element.
             * @return Reference to the lift scalar (units depend on simulation).
             */
        const real_dNum &getLift() const;

        /**
             * @brief Set the computed lift force for this surface element.
             * @param liftValue Lift magnitude to assign.
             */
        void setLift(const real_dNum &liftValue);

        /**
             * @brief Get the local flow speed magnitude for this element.
             * @return Reference to the flow speed scalar.
             */
        const real_dNum &getFlowSpeed() const;

        /**
             * @brief Set the local flow speed magnitude for this element.
             * @param flowSpeedValue Flow speed magnitude to assign.
             */
        void setFlowSpeed(const real_dNum &flowSpeedValue);

        /**
             * @brief Get the local airflow vector (non-unit) for this element.
             * @return Reference to the local airflow vector (m_lFlow).
             */
        const physics_Vec &getLFlow() const;

        /**
             * @brief Set the local airflow vector for this element.
             * @param localFlow New local airflow vector (non-unit).
             */
        void setLFlow(const physics_Vec &localFlow);

        /* Unit direction vectors (all expected to be normalized): */
        physics_Vec m_chordVec =
            physics_Vec::zero(); ///< Unit vector along the chord (zero-lift line)
        physics_Vec m_spanVec = physics_Vec::zero(); ///< Unit vector pointing spanwise
        physics_Vec m_normVec = physics_Vec::zero(); ///< Unit vector normal to span and chord
        physics_Vec m_liftVec = physics_Vec::zero();
        ///< Unit vector in the lift direction (normal
                                                                ///< to local flow and chord)
        physics_Vec m_flowVec = physics_Vec::zero(); ///< Unit vector along the local flow direction

        /* Position and non-unit vectors: */
        physics_Vec m_centre = physics_Vec::zero(); ///< Position vector of element w.r.t. datum

        physics_Vec m_lForce =
            physics_Vec::zero(); ///< Sum of lift and drag forces applied to this surface (non-unit)
        physics_Vec m_lInduced =
            physics_Vec::zero(); ///< Induced flow component local to this element (rotor FoR)
        physics_Vec m_interference =
            physics_Vec::zero(); ///< Flow interference from other aircraft components

        /* Scalars used in aerodynamic calculations: */
        real_dNum m_iFactor = 0.0;
        ///< Inverse factor (1/(2*SweptArea*Ro)) used converting arc lift
                                              ///< to induced velocity
        real_dNum m_alphaG =
            0.0; ///< Geometric angle of attack (w.r.t. tip-path plane), not accounting for flow
        real_dNum m_alphaL = 0.0; ///< Local (effective) angle of attack including flow effects
        real_dNum m_cl = 0.0; ///< Current lift coefficient (Cl)
        real_dNum m_cd = 0.0; ///< Current drag coefficient (Cd)
        real_dNum m_drag =
            0.0; ///< Drag force magnitude (along -LFlow). Note: does not include time-averaging
        real_dNum m_area = 0.0; ///< Surface area of this element
        real_dNum m_chord = 0.0; ///< Chordwise length of the element
        real_dNum m_span = 0.0; ///< Spanwise length of the element
        real_dNum m_rad = 0.0; ///< Radial distance of the element from hub centre (if applicable)

        real_dNum m_lift = 0.0;
        ///< Lift force magnitude (along m_liftVec). Note: does not include
                                           ///< blade/time averaging
        real_dNum m_flowSpeed = 0.0; ///< Local flow speed magnitude (norm of m_lFlow)
        physics_Vec m_lFlow =
            physics_Vec::zero(); ///< Local airflow vector for this surface (non-unit)
    };

    inline const real_dNum &TSurface::getLift() const
    {
        WP_ASSERT(Math<physics_Num>::isFinite( m_lift ));
        return m_lift;
    }

    inline void TSurface::setLift(const real_dNum &liftValue)
    {
        WP_ASSERT(Math<physics_Num>::isFinite( liftValue ));
        m_lift = liftValue;
    }

    inline const real_dNum &TSurface::getFlowSpeed() const
    {
        WP_ASSERT(Math<physics_Num>::isFinite( m_flowSpeed ));
        return m_flowSpeed;
    }

    inline void TSurface::setFlowSpeed(const real_dNum &flowSpeedValue)
    {
        WP_ASSERT(flowSpeedValue < 1000.0 && flowSpeedValue > -1000.0);
        WP_ASSERT(Math<physics_Num>::isFinite( flowSpeedValue ));
        m_flowSpeed = flowSpeedValue;
    }

    inline const physics_Vec &TSurface::getLFlow() const
    {
        WP_ASSERT(Math<physics_Num>::isFinite( VMag( m_lFlow ) ));
        WP_ASSERT(m_lFlow.isFinite());
        return m_lFlow;
    }

    inline void TSurface::setLFlow(const physics_Vec &localFlow)
    {
        WP_ASSERT(Math<physics_Num>::isFinite( VMag( localFlow ) ));
        WP_ASSERT(localFlow.isFinite());
        m_lFlow = localFlow;
    }
}

#endif // Surface_h__
