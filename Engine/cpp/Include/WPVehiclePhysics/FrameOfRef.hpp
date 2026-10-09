#ifndef FrameOfRef_h__
#define FrameOfRef_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <Workphone/Math/Vector3.hpp>

/**
 * @file VecMath.hpp
 * @brief Lightweight vector math helpers and a small frame-of-reference type used
 * by the vehicle physics code. All functions operate on physics_Vec and
 * physics_Num (typedefs provided by WPAeroPrerequisites).
 */

namespace workphone::vehicle
{
    /**
         * @brief Simple orthonormal frame-of-reference container.
         *
         * Holds three basis vectors (m_xAxis, m_yAxis, m_zAxis) representing a local
         * coordinate frame. The class is intentionally lightweight and
         * trivially-copyable so it can be used in tight loops within the
         * physics simulation.
         */
    class FrameOfRef
    {
    public:
        FrameOfRef();

        /**
             * @brief Copy constructor delegates to operator=
             */
        FrameOfRef(const FrameOfRef &other);

        ~FrameOfRef();

        /**
             * @brief Copy assignment.
             * @param other Frame to copy from.
             * @return Reference to this frame after assignment.
             */
        FrameOfRef &operator=(const FrameOfRef &other);

        /// Basis vector pointing along the local X axis
        physics_Vec m_xAxis = physics_Vec::zero();
        /// Basis vector pointing along the local Y axis
        physics_Vec m_yAxis = physics_Vec::zero();
        /// Basis vector pointing along the local Z axis
        physics_Vec m_zAxis = physics_Vec::zero();
    }; // FrameOfRef
}

#endif // FrameOfRef_h__
