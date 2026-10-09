#ifndef VecMathH
#define VecMathH

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
         * @brief Returns the magnitude (length) of a vector.
         * @param V Input vector
         * @return Euclidean length of V
         */
    physics_Num VMag(const physics_Vec &V);

    /**
         * @brief Returns a unit vector in the direction of V.
         * If V is (near) zero a stable default unit vector is returned to
         * avoid divide-by-zero.
         * @param V Input vector
         * @return Normalized vector (length == 1) pointing in V's direction
         */
    physics_Vec VUnit(const physics_Vec &V);

    /**
         * @brief Component-wise sum of two vectors.
         */
    physics_Vec VSum(const physics_Vec &V, const physics_Vec &W);

    /**
         * @brief Component-wise sum of three vectors. Useful for some
         * coordinate rotation calculations.
         */
    physics_Vec V3Sum(const physics_Vec &U, const physics_Vec &V, const physics_Vec &W);

    /**
         * @brief Component-wise difference: V - W.
         */
    physics_Vec VDif(const physics_Vec &V, const physics_Vec &W);

    /**
         * @brief Cross product V x W.
         */
    physics_Vec VCross(const physics_Vec &V, const physics_Vec &W);

    /**
         * @brief Dot product V . W.
         */
    physics_Num VDot(const physics_Vec &V, const physics_Vec &W);

    /**
         * @brief Scales vector V by scalar S.
         */
    physics_Vec VScale(const physics_Vec &V, physics_Num S);

    /**
         * @brief Returns -V (negates each component).
         */
    physics_Vec VNeg(const physics_Vec &V);

    /**
         * @brief Rotate vector V about axis K by angle Ang (radians).
         * @note Implementation provided elsewhere.
         */
    physics_Vec VRotate(const physics_Vec &V, const physics_Vec &K, physics_Num Ang);

    /**
         * @brief Rotates a frame-of-reference by the simultaneous small
         * rotation defined in Rotn (angular components about frame axes).
         * @param Frame Frame to rotate
         * @param Rotn Small rotation vector (radians)
         * @return Rotated frame
         */
    FrameOfRef RotateFrame(const FrameOfRef &Frame, const physics_Vec &Rotn);

    /**
         * @brief Expresses a ground-frame vector VecInGF in the supplied local
         * frame coordinates.
         * @param VecInGF Vector expressed in the ground/global frame
         * @param Frame Target local frame
         * @return Vector expressed in Frame coordinates
         */
    physics_Vec VecToFrame(const physics_Vec &VecInGF, const FrameOfRef &Frame);

    /**
         * @brief Converts a vector given in a local frame to ground/global
         * frame components.
         */
    physics_Vec VecFromFrame(const physics_Vec &V, const FrameOfRef &Frame);

    /**
         * @brief Convert a conventional aerodynamic vector to the internal
         * "Saracen" coordinate convention used by this simulator.
         * @note Mapping: Saracen.x = -conv.y, Saracen.y = -conv.z, Saracen.z = conv.x
         */
    physics_Vec VecToSaracen(const physics_Vec &CV);

    /**
         * @brief Convert from Saracen coordinates back to the conventional system.
         */
    physics_Vec VecFromSaracen(const physics_Vec &SV);

    // returns a vector = V x W
    physics_Vec VCross(const physics_Vec &V, const physics_Vec &W); // VCross

    // returns the dot product of two vectors
    physics_Num VDot(const physics_Vec &V, const physics_Vec &W); // VDot

    // returns vector V but scaled by factor S
    physics_Vec VScale(const physics_Vec &V, physics_Num S); // VScale

    // simply creates a vector pointing in the -V direction
    physics_Vec VNeg(const physics_Vec &V);

    // returns the sum of the vectors passed
    physics_Vec VSum(const physics_Vec &V, const physics_Vec &W); // VSum

    // sums three vectors as needed by coordinate rotation calc etc
    physics_Vec V3Sum(const physics_Vec &U, const physics_Vec &V, const physics_Vec &W); // V3Sum

    // returns the magnitude of a vector
    physics_Num VMag(const physics_Vec &V); // VMag

    // returns the difference of two vectors
    physics_Vec VDif(const physics_Vec &V, const physics_Vec &W); // VDif

    // note: Saracen.x =  - conv.y
    //       Saracen.y =  - conv.z
    //       Saracen.z =    conv.x
    physics_Vec VecToSaracen(const physics_Vec &CV);

    // Note: Conv.x = Saracen.z
    //       Conv.y = -Saracen.x
    //       Conv.z = -Saracen.y
    physics_Vec VecFromSaracen(const physics_Vec &SV);

    // returns a unit vector in direction ov V
    physics_Vec VUnit(const physics_Vec &V); // VNormalize

    // a simple rodregues rotation used for setting up the chord lines
    // rotation is of vector V about axis K and by angle Ang
    // Accurately VRot = V*Cos(Ang) +  (K x V)*Sin(Ang) +K*(K.V)*(1-Cos(Ang))
    physics_Vec VRotate(const physics_Vec &V, const physics_Vec &K, physics_Num Ang);

    // calculates the components of ground frame vector in the specified frame of reference
    physics_Vec VecToFrame(const physics_Vec &VecInGF, const FrameOfRef &Frame); // VecToFrame

    // calculates the ground frame components of a vector in a specified frame of reference
    physics_Vec VecFromFrame(const physics_Vec &V, const FrameOfRef &Frame); // VecFromFrame
}

#endif //  VecMathH
