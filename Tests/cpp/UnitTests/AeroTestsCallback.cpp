//
// Created by Zane Desir on 07/11/2021.
//

#include "AeroTestsCallback.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    DummyAircraftCallback::DummyAircraftCallback()
    {
    }

    DummyAircraftCallback::~DummyAircraftCallback()
    {
    }

    Vector3<real_Num> DummyAircraftCallback::getAngularVelocity() const
    {
        return Vector3<real_Num>::zero();
    }

    Vector3<real_Num> DummyAircraftCallback::getLinearVelocity() const
    {
        return Vector3<real_Num>::zero();
    }

    Vector3<real_Num> DummyAircraftCallback::getLocalAngularVelocity() const
    {
        return Vector3<real_Num>::zero();
    }

    Vector3<real_Num> DummyAircraftCallback::getLocalLinearVelocity() const
    {
        return Vector3<real_Num>::zero();
    }

    Vector3<real_Num> DummyAircraftCallback::getScale() const
    {
        return Vector3<real_Num>::unit();
    }

    Vector3<real_Num> DummyAircraftCallback::getPosition() const
    {
        return position;
    }

    Quaternion<real_Num> DummyAircraftCallback::getOrientation() const
    {
        return orientation;
    }

    void *DummyAircraftCallback::getCallbackFunction() const
    {
        return nullptr;
    }

    void *DummyAircraftCallback::getCallbackDataFunction() const
    {
        return nullptr;
    }

    std::array<f32, 8> DummyAircraftCallback::getInputData() const
    {
        return std::array<f32, 8>();
    }

    std::array<float, 11> DummyAircraftCallback::getControlAngles() const
    {
        return std::array<f32, 11>();
    }

    void DummyAircraftCallback::displayLocalVector( s32 Bdy, s32 id, const Vector3<real_Num> &V,
                                                    const Vector3<real_Num> &Org, s32 colour ) const
    {
    }

    void DummyAircraftCallback::displayLocalVector( s32 Bdy, const Vector3<real_Num> &V,
                                                    const Vector3<real_Num> &Org, s32 colour ) const
    {
    }

    void DummyAircraftCallback::displayVector( s32 Bdy, s32 id, const Vector3<real_Num> &V,
                                               const Vector3<real_Num> &Org, s32 colour ) const
    {
    }
}  // namespace workphone
