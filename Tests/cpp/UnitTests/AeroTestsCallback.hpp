//
// Created by Zane Desir on 07/11/2021.
//

#ifndef WP_DUMMYAIRCRAFTCALLBACK_H
#define WP_DUMMYAIRCRAFTCALLBACK_H

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Vehicle/IAircraftCallback.hpp>

namespace workphone
{
    class DummyAircraftCallback : public vehicle::IAircraftCallback
    {
    public:
        DummyAircraftCallback();
        ~DummyAircraftCallback() override;

        void addForce( s32 bodyId, const Vector3<real_Num> &force ) override
        {
        }

        String getData( const String &filePath ) override
        {
            // auto fileSystem = workphone::make_ptr<CFileSystem>();
            // return fileSystem->readAllText(filePath);
            return "";
        }

        void addForce( s32 bodyId, const Vector3<real_Num> &force,
                       const Vector3<real_Num> &pos ) override
        {
        }

        void addTorque( s32 bodyId, const Vector3<real_Num> &torque ) override
        {
        }

        void addLocalForce( s32 bodyId, const Vector3<real_Num> &force,
                            const Vector3<real_Num> &pos ) override
        {
        }

        void addLocalTorque( s32 bodyId, const Vector3<real_Num> &torque ) override
        {
        }

        Vector3<real_Num> getAngularVelocity() const override;

        Vector3<real_Num> getLinearVelocity() const override;

        Vector3<real_Num> getLocalAngularVelocity() const override;

        Vector3<real_Num> getLocalLinearVelocity() const override;

        Vector3<real_Num> getScale() const override;

        Vector3<real_Num> getPosition() const override;

        Quaternion<real_Num> getOrientation() const override;

        void *getCallbackFunction() const override;

        void setCallbackFunction( void *val ) override
        {
        }

        void *getCallbackDataFunction() const override;

        void setCallbackDataFunction( void *val ) override
        {
        }

        std::array<f32, 8> getInputData() const override;

        std::array<float, 11> getControlAngles() const override;

        Vector3<real_Num> getPointVelocity( const Vector3<real_Num> &p ) override
        {
            return Vector3<real_Num>::zero();
        }

        Vector3<real_Num> position;
        Quaternion<real_Num> orientation;

        bool castLocalRay( const Ray3<real_Num> &ray, SmartPtr<physics::IRaycastHit> &data ) override
        {
            return false;
        }

        bool castWorldRay( const Ray3<real_Num> &ray, SmartPtr<physics::IRaycastHit> &data ) override
        {
            return false;
        }

        void displayLocalVector( s32 Bdy, const Vector3<real_Num> &V, const Vector3<real_Num> &Org,
                                 s32 colour ) const override;

        void displayVector( s32 Bdy, s32 id, const Vector3<real_Num> &V, const Vector3<real_Num> &Org,
                            s32 colour ) const override;

        void displayLocalVector( s32 Bdy, s32 id, const Vector3<real_Num> &V,
                                 const Vector3<real_Num> &Org, s32 colour ) const override;
    };
}  // namespace workphone

#endif  // WP_DUMMYAIRCRAFTCALLBACK_H
