#ifndef RotorHead_h__
#define RotorHead_h__

#include "WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp"
#include "WPVehiclePhysics/Rotor.hpp"
#include "WPVehiclePhysics/Linkage.hpp"
#include "VecMath.hpp"
#include <string>
#include <array>
#include <memory>

namespace workphone::vehicle
{
    /**
         * @brief Represents the rotor head assembly which connects rotors to the
         *        airframe and transmits forces/torques through the hub.
         *
         * The rotor head encapsulates the shaft frame of reference, the main
         * rotor and optional flybar rotors, linkage geometry and the
         * instantaneous forces and torques applied to the airframe. It also
         * stores configuration constants for teetering components and a simple
         * inflow buffer used by aerodynamic computations.
         */
    class TRotorHead
    {
    public:
        /// Default constructor
        TRotorHead();
        /// Non-copyable: disable copy construction to avoid accidental sharing
        TRotorHead(const TRotorHead &other) = delete;
        /// Destructor
        ~TRotorHead();

        /// Frame of reference for the rotor shaft, defined by the head
            /// orientation in the model
        FrameOfRef m_shaftFrame;

        /// Main rotor data (blades, sectors, dynamics)
        TRotor m_mainRotor;

        /// Flybar (stabiliser bar) rotor data, if present
        TRotor m_flyBar;

        /// Linkage geometry and kinematic data connecting rotor to swashplate
        TLinkage m_linkage;

        /// Torques transmitted to the body from the rotor head (teeter + shaft)
        physics_Vec m_torques = physics_Vec::zero();
        /// Linear forces acting through the hub into the airframe
        physics_Vec m_forces = physics_Vec::zero();

        /// Forward inclination (rake) of the shaft in body coordinates (radians)
        real_dNum m_shaftRake = 0.0;
        /// Roll/tilt of the shaft in body coordinates (radians)
        real_dNum m_shaftTilt = 0.0;
        /// Force constant of teeter rubbers (N·m per radian)
        real_dNum m_teeterForceConstant = 0.0;
        /// Damping constant of teeter rubbers (N·m·s per radian)
        real_dNum m_teeterDampingConstant = 0.0;
        /// True if the model includes a flybar
        bool m_hasFlyBar = false;

        /**
             * @brief Repository for inflow vectors indexed by element (1..20).
             *
             * The array has 21 entries with index 0 unused to allow algorithms
             * to use 1-based indexing for blade element rows.
             */
        std::array<physics_Vec, 21> m_inflow;
    }; // TRotorHead
}

#endif // RotorHead_h__
