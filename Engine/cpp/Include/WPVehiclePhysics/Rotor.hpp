#ifndef Rotor_h__
#define Rotor_h__

#include "WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp"
#include "WPVehiclePhysics/FrameOfRef.hpp"
#include "WPVehiclePhysics/FoilLookup.hpp"
#include "WPVehiclePhysics/RotorSector.hpp"
#include <Workphone/Core/StringTypes.hpp>
#include "VecMath.hpp"
#include <string>
#include <array>
#include <memory>

namespace workphone::vehicle
{
    /**
         * @brief Models a rotor assembly including geometry, dynamics and
         *        aerodynamic lookup data.
         *
         * TRotor stores per-rotor properties (blade geometry, inertial
         * properties, operational state) and results (forces, moments,
         * per-sector data). Many members are public for efficient, POD-style
         * access within performance-critical simulation loops.
         */
    class TRotor
    {
    public:
        /// Default constructor
        TRotor();

        /// Non-copyable: disable copy construction to avoid accidental sharing
        TRotor(const TRotor &other) = delete;

        /// Destructor
        ~TRotor();

        /**
             * @brief Access the aerofoil lookup table used by this rotor.
             * @return Modifiable reference to the TFoilTable.
             */
        TFoilTable &getAFoilLookup();

        /**
             * @brief Const access to the aerofoil lookup table.
             * @return Const reference to the TFoilTable.
             */
        const TFoilTable &getAFoilLookup() const;

        /**
             * @brief Replace the aerofoil lookup table for this rotor.
             * @param foilLookup Table to use for CL/CD lookups.
             */
        void setAFoilLookup(const TFoilTable &foilLookup);

        const String &getSection() const;
        void setSection(const String &sectionName);

        /**
             * @brief Get the current rotor angular velocity (rad/s).
             */
        real_dNum getOmega() const;

        /**
             * @brief Set the rotor angular velocity (rad/s).
             */
        void setOmega(const real_dNum &angularVelocity);

        /**
             * @brief Returns true if rotor rotates clockwise (viewed from above).
             */
        bool isCw() const;

        /**
             * @brief Set the rotor rotation direction flag.
             */
        void setCw(bool isClockwise);

        /* Frame vectors that define the rotor axes. */
        FrameOfRef m_frame;

        /* Resultant force (lift+drag) acting at the hub centre. */
        physics_Vec m_hubForce = physics_Vec::zero();

        /* Moments acting on the rotor disc represented as a vector. */
        physics_Vec m_forceMoments = physics_Vec::zero();

        /* Precession rate about each axis (rad/s). */
        physics_Vec m_precessionRate = physics_Vec::zero();

        /* Position vector of the rotor hub in the body frame. */
        physics_Vec m_hubPosition = physics_Vec::zero();

        /* Local airflow at the hub centre expressed in rotor frame. */
        physics_Vec m_hubFlow = physics_Vec::zero();

        /* Number of blades fitted to the rotor. */
        s32 m_blades = 0;

        /* Geometry: radial positions and chord lengths. */
        real_dNum m_minRad = 0.0; ///< Inner (cuff) radius
        real_dNum m_maxRad = 0.0; ///< Outer (tip) radius
        real_dNum m_cuffChord = 0.0; ///< Chord at cuff
        real_dNum m_tipChord = 0.0; ///< Chord at tip

        /* Blade twist (cuff angle relative to tip). */
        real_dNum m_twist = 0.0;

        /* Inertial properties of individual blades. */
        real_dNum m_bladeWeight = 0.0; ///< Mass or weight of single blade
        real_dNum m_radOfGyr = 0.0; ///< Radius of gyration about hub

        /* Rotor orientation and control inputs. */
        real_dNum m_cone = 0.0; ///< Coning angle (rad, + = upwards)
        real_dNum m_collective = 0.0; ///< Collective pitch (rad)
        real_dNum m_ailCyclic = 0.0; ///< Aileron cyclic input (rad)
        real_dNum m_eleCyclic = 0.0; ///< Elevator cyclic input (rad)

        /* Dynamics and state. */
        real_dNum m_momentOfInertia = 0.0; ///< Moment of inertia about hub
        real_dNum m_angularMomentum = 0.0; ///< Angular momentum about hub

        /* Head deflections relative to mainshaft (body sign convention). */
        real_dNum m_eleAngle = 0.0; ///< Longitudinal head tilt (+ nose down)
        real_dNum m_ailAngle = 0.0; ///< Lateral head tilt (+ roll right)

        /* Limits and operational parameters. */
        real_dNum m_totalAngleLimit = 0.0; ///< Cyclic+collective angle limit (rad)
        real_dNum m_omega = 0.0; ///< Rotation rate (rad/s)
        bool m_isCw = true; ///< Rotation direction flag

        /* Ground effect and per-sector data (1-based indices: 1..4). */
        std::array<physics_Num, 5> m_groundEffect; ///< Per-sector ground effect values
        std::array<physics_Num, 5> m_groundDistance; ///< Per-sector ground distances
        std::array<TRotorSector, 5> m_sector; ///< Per-sector rotor data

        /* Aerodynamic lookup and identification. */
        String m_section; ///< Name of aerofoil section (for CL/CD lookup)
        TFoilTable m_aFoilLookup; ///< Aerofoil lookup table used for this rotor
    }; // TRotor

    inline const TFoilTable &TRotor::getAFoilLookup() const
    {
        return m_aFoilLookup;
    }

    inline TFoilTable &TRotor::getAFoilLookup()
    {
        return m_aFoilLookup;
    }

    inline void TRotor::setAFoilLookup(const TFoilTable &foilLookup)
    {
        m_aFoilLookup = foilLookup;
    }

    inline const String &TRotor::getSection() const
    {
        return m_section;
    }

    inline void TRotor::setSection(const String &sectionName)
    {
        m_section = sectionName;
    }

    inline real_dNum TRotor::getOmega() const
    {
        WP_ASSERT(Math<real_dNum>::isFinite( m_omega ));
        return m_omega;
    }

    inline void TRotor::setOmega(const real_dNum &angularVelocity)
    {
        WP_ASSERT(Math<real_dNum>::isFinite( angularVelocity ));
        WP_ASSERT(Math<real_dNum>::isFinite( m_omega ));
        m_omega = angularVelocity;
    }

    inline bool TRotor::isCw() const
    {
        return false;
        // return m_isCw;
    }

    inline void TRotor::setCw(bool isClockwise)
    {
        m_isCw = isClockwise;
    }
}

#endif // Rotor_h__
