#include <WPPhysx/WPPhysxPCH.hpp>
#include <WPPhysx/WPPhysxVehicleWheel3.hpp>

namespace workphone::physics
{
    PhysxVehicleWheel3::PhysxVehicleWheel3() = default;

    PhysxVehicleWheel3::~PhysxVehicleWheel3() = default;

    void PhysxVehicleWheel3::initialise( const Vector3F &pos, f32 radius, f32 width,
                                         f32 suspensionRestLength, f32 suspension_Ks, f32 suspension_Kd,
                                         bool powered, bool steering, bool brakes )
    {
    }

    Vector3F PhysxVehicleWheel3::getPosition() const
    {
        return Vector3F::zero();
    }

    QuaternionF PhysxVehicleWheel3::getOrientation() const
    {
        return QuaternionF::identity();
    }

    Vector3F PhysxVehicleWheel3::getVelocity() const
    {
        return Vector3F::zero();
    }

    void PhysxVehicleWheel3::setMaterialId( u32 materialId )
    {
    }

    u32 PhysxVehicleWheel3::getMaterialId() const
    {
        return 0;
    }

    f32 PhysxVehicleWheel3::getAngularVelocity() const
    {
        return 0;
    }

    AABB3F PhysxVehicleWheel3::getLocalAABB() const
    {
        return {};
    }

    AABB3F PhysxVehicleWheel3::getWorldAABB() const
    {
        return {};
    }

    physics_Num PhysxVehicleWheel3::getRadius() const
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    void PhysxVehicleWheel3::setRadius( physics_Num radius )
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    physics_Num PhysxVehicleWheel3::getWidth() const
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    void PhysxVehicleWheel3::setWidth( physics_Num width )
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    physics_Num PhysxVehicleWheel3::getMaxSuspensionTravelCm() const
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    void PhysxVehicleWheel3::setMaxSuspensionTravelCm( physics_Num maxSuspensionTravelCm )
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    physics_Num PhysxVehicleWheel3::getMaxSuspensionForce() const
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    void PhysxVehicleWheel3::setMaxSuspensionForce( physics_Num maxSuspensionForce )
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    physics_Num PhysxVehicleWheel3::getSuspensionStiffness() const
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    void PhysxVehicleWheel3::setSuspensionStiffness( physics_Num suspensionStiffness )
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    physics_Num PhysxVehicleWheel3::getSuspensionDamping() const
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    void PhysxVehicleWheel3::setSuspensionDamping( physics_Num suspensionDamping )
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    physics_Num PhysxVehicleWheel3::getFrictionSlip() const
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    void PhysxVehicleWheel3::setFrictionSlip( physics_Num frictionSlip )
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    physics_Num PhysxVehicleWheel3::getSteering() const
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    void PhysxVehicleWheel3::setSteering( physics_Num steering )
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    physics_Num PhysxVehicleWheel3::getEngineForce() const
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    void PhysxVehicleWheel3::setEngineForce( physics_Num engineForce )
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    physics_Num PhysxVehicleWheel3::getBrake() const
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    void PhysxVehicleWheel3::setBrake( physics_Num brake )
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    bool PhysxVehicleWheel3::isInContact() const
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }
} // namespace workphone::physics

// end namespace fb
