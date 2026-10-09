#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include "WPVehiclePhysics/VecMath.hpp"
#include "WPVehiclePhysics/FrameOfRef.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::vehicle
{
    // Rotates a frame of reference through simultaneous pitch roll and yaw motions specified by the Rotn
    // vec
    FrameOfRef RotateFrame(const FrameOfRef &Frame, const physics_Vec &Rotn)
    {
        FrameOfRef result;
        FrameOfRef F;
        float Kx, Ky; // holds the 'cosine' factor in rotations

        float TotRot, Rx, Ry, Rz;
        int n, Steps;
        int ExLoc;
        ExLoc = 0;
        try
        {
            TotRot = 10.0f * (0.1 + Math<physics_Num>::Abs(Rotn.x) +
                              Math<physics_Num>::Abs(Rotn.y) + Math<physics_Num>::Abs(Rotn.z));
            // calculate total rotation (in 0.1 radians) as a crude way of deciding on the number of
            // steps needed
            if((TotRot <= 0) ||
               (TotRot > 10000)) // with zero rotation or a stupidly big rotation just ignore it!!
            {
                // there is no rotation to do
                if(TotRot > 10000)
                {
                    WP_LOG_ERROR("RotateFrame asked to rotate by " + StringUtil::toString( TotRot ) +
                        "! Fuck that!");
                }

                result = Frame; // pass the unrotated frame out and save time!
            }
            else
            {
                ExLoc = 1;
                Steps = Math<physics_Num>::roundToInt(TotRot); // use 1 step for each 0.1 radians
                if(Steps < 1)
                    Steps = 1; // belt and braces trap

                ExLoc = 2;
                Rx = Rotn.x / static_cast<float>(Steps); // split each rotation into n
                Ry = Rotn.y / static_cast<float>(Steps);
                Rz = Rotn.z / static_cast<float>(Steps);
                ExLoc = 3;
                Kx = 1.0f - (Math<physics_Num>::Sqr(Ry) + Math<physics_Num>::Sqr(Rz)) /
                     2.0f; // set Kx for XAxis rotation  *NOTE revised in Version 11
                Ky = 1.0f - (Math<physics_Num>::Sqr(Rz) + Math<physics_Num>::Sqr(Rx)) /
                     2.0f; // set Ky for YAxis rotation  *NOTE revised in Version 11
                ExLoc = 4;
                F = Frame;
                // copy the unrotated frame to F so that each step works on the output axes of the
                // previous rotation!!
                for(n = 0; n < Steps; n++) // now do the rotation in n steps
                {
                    ExLoc = 5;
                    F.m_xAxis = VUnit(V3Sum(VScale(F.m_xAxis, Kx), VScale(F.m_yAxis, Rz),
                                            VScale(F.m_zAxis, -Ry))); // new XAxis
                    ExLoc = 6;
                    F.m_yAxis = VUnit(V3Sum(VScale(F.m_yAxis, Ky), VScale(F.m_xAxis, -Rz),
                                            VScale(F.m_zAxis, Rx))); // new YAxis
                    ExLoc = 7;
                    F.m_zAxis = VUnit(
                        VCross(F.m_xAxis, F.m_yAxis)); // new Z axis perpendicular to new X and Y
                    ExLoc = 8;
                    F.m_xAxis =
                        VCross(F.m_yAxis,
                               F.m_zAxis); // re-do XAxis as cross of Y & Z to ensure orthoganality!
                } // for n loop
                ExLoc = 9;
                result = F; // pass the finished frame of reference back
            } // else (i.e rot<>0)
        }
        catch(std::exception &Err)
        {
            WP_LOG_EXCEPTION(Err);
        }

        return result;
    }

    physics_Vec VCross(const physics_Vec &V, const physics_Vec &W)
    {
        return physics_Vec(V.y * W.z - V.z * W.y, V.z * W.x - V.x * W.z, V.x * W.y - V.y * W.x);
    }

    physics_Num VDot(const physics_Vec &V, const physics_Vec &W)
    {
        return V.x * W.x + V.y * W.y + V.z * W.z;
    }

    physics_Vec VScale(const physics_Vec &V, physics_Num S)
    {
        return physics_Vec(V.x * S, V.y * S, V.z * S);
    }

    physics_Vec VNeg(const physics_Vec &V)
    {
        return physics_Vec(-V.x, -V.y, -V.z);
    }

    physics_Vec VSum(const physics_Vec &V, const physics_Vec &W)
    {
        return physics_Vec(V.x + W.x, V.y + W.y, V.z + W.z);
    }

    physics_Vec V3Sum(const physics_Vec &U, const physics_Vec &V,
                      const physics_Vec &W)
    {
        return physics_Vec(U.x + V.x + W.x, U.y + V.y + W.y, U.z + V.z + W.z);
    }

    physics_Num VMag(const physics_Vec &V)
    {
        // return square root of the sum of the squares (wow!)
        return Math<physics_Num>::Sqrt(Math<physics_Num>::Sqr(V.x) + Math<physics_Num>::Sqr(V.y) +
                                       Math<physics_Num>::Sqr(V.z));
    }

    physics_Vec VDif(const physics_Vec &V, const physics_Vec &W)
    {
        return physics_Vec(V.x - W.x, V.y - W.y, V.z - W.z);
    }

    physics_Vec VecToSaracen(const physics_Vec &CV)
    {
        return physics_Vec(-CV.y, -CV.z, CV.x);
    }

    physics_Vec VecFromSaracen(const physics_Vec &SV)
    {
        return physics_Vec(SV.z, -SV.x, -SV.y);
    }

    physics_Vec VUnit(const physics_Vec &V)
    {
        // temp. for magnitude
        physics_Num Mag = Math<physics_Num>::Sqr(V.x) + Math<physics_Num>::Sqr(V.y) +
                          Math<physics_Num>::Sqr(V.z);
        if(Mag < std::numeric_limits<physics_Num>::epsilon())
        {
            // if Mag = 0
            return physics_Vec(0.577350,
                               0.577530,
                               0.577350);
            // return a unit vector with a non-zero component in all directions to avoid crashes
        }

        // Mag <>0
        auto l = Math<physics_Num>::Sqrt(Mag);
        return physics_Vec(V.x / l, V.y / l, V.z / l); // return each element/Magnitude
    }

    physics_Vec VRotate(const physics_Vec &V, const physics_Vec &K,
                        physics_Num Ang)
    {
        WP_ASSERT(Math<physics_Num>::isFinite( Ang ));

        return V3Sum(VScale(V, Math<physics_Num>::Cos(Ang)),
                     VScale(VCross(K, V), Math<physics_Num>::Sin(Ang)),
                     VScale(K, VDot(K, V) * (static_cast<physics_Num>(1.0) -
                                             Math<physics_Num>::Cos(Ang))));
    }

    physics_Vec VecToFrame(const physics_Vec &VecInGF, const FrameOfRef &Frame)
    {
        // calculate the flow component in the direction of the rotor's X axis
        // calculate the flow component in the direction of the rotor's Y axis
        // calculate the flow component in the direction of the rotor's Z axis
        return physics_Vec(VDot(VecInGF, Frame.m_xAxis), VDot(VecInGF, Frame.m_yAxis),
                           VDot(VecInGF, Frame.m_zAxis));
    }

    physics_Vec VecFromFrame(const physics_Vec &V, const FrameOfRef &Frame)
    {
        return V3Sum(VScale(Frame.m_xAxis, V.x), VScale(Frame.m_yAxis, V.y),
                     VScale(Frame.m_zAxis, V.z)); //
    }
} // namespace workphone::vehicle
