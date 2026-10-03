#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Physics/RaycastHit.hpp>
#include <Workphone/Interface/Physics/IPhysicsShape3.hpp>
#include <Workphone/Interface/Physics/IRigidBody3.hpp>

namespace workphone::physics
{

    WP_CLASS_REGISTER_DERIVED( workphone::physics, RaycastHit, IRaycastHit );

    RaycastHit::RaycastHit() = default;
    RaycastHit::~RaycastHit() = default;

    auto RaycastHit::getCollider() const -> SmartPtr<IPhysicsShape3>
    {
        return m_collider;
    }

    void RaycastHit::setCollider( SmartPtr<IPhysicsShape3> shape )
    {
        m_collider = shape;
    }

    auto RaycastHit::getRigidBody() const -> SmartPtr<IRigidBody3>
    {
        return m_rigidbody;
    }

    void RaycastHit::setRigidBody( SmartPtr<IRigidBody3> rigidBody )
    {
        m_rigidbody = rigidBody;
    }

    auto RaycastHit::getBarycentricCoordinate() const -> Vector2<real_Num>
    {
        return m_barycentricCoordinate;
    }

    void RaycastHit::setBarycentricCoordinate( const Vector2<real_Num> &barycentricCoordinate )
    {
        m_barycentricCoordinate = barycentricCoordinate;
    }

    auto RaycastHit::getLightmapCoord() const -> Vector2<real_Num>
    {
        return m_lightmapCoord;
    }

    void RaycastHit::setLightmapCoord( const Vector2<real_Num> &lightmapCoord )
    {
        m_lightmapCoord = lightmapCoord;
    }

    auto RaycastHit::getNormal() const -> Vector3<real_Num>
    {
        return m_normal;
    }

    void RaycastHit::setNormal( const Vector3<real_Num> &normal )
    {
        m_normal = normal;
    }

    auto RaycastHit::getPoint() const -> Vector3<real_Num>
    {
        return m_point;
    }

    void RaycastHit::setPoint( const Vector3<real_Num> &point )
    {
        m_point = point;
    }

    auto RaycastHit::getTextureCoord() const -> Vector2<real_Num>
    {
        return m_textureCoord;
    }

    void RaycastHit::setTextureCoord( const Vector2<real_Num> &textureCoord )
    {
        m_textureCoord = textureCoord;
    }

    auto RaycastHit::getTextureCoord2() const -> Vector2<real_Num>
    {
        return m_textureCoord2;
    }

    void RaycastHit::setTextureCoord2( const Vector2<real_Num> &textureCoord2 )
    {
        m_textureCoord2 = textureCoord2;
    }

    auto RaycastHit::getDistance() const -> real_Num
    {
        return m_distance;
    }

    void RaycastHit::setDistance( real_Num distance )
    {
        m_distance = distance;
    }

    auto RaycastHit::getTriangleIndex() const -> s32
    {
        return m_triangleIndex;
    }

    void RaycastHit::setTriangleIndex( s32 triangleIndex )
    {
        m_triangleIndex = triangleIndex;
    }

    auto RaycastHit::getCollisionMask() const -> u32
    {
        return m_collisionMask;
    }

    void RaycastHit::setCollisionMask( u32 collisionMask )
    {
        m_collisionMask = collisionMask;
    }

    auto RaycastHit::getCheckStatic() const -> bool
    {
        return m_checkStatic;
    }

    void RaycastHit::setCheckStatic( bool checkStatic )
    {
        m_checkStatic = checkStatic;
    }

    auto RaycastHit::getCheckDynamic() const -> bool
    {
        return m_checkDynamic;
    }

    void RaycastHit::setCheckDynamic( bool checkDynamic )
    {
        m_checkDynamic = checkDynamic;
    }
}  // namespace workphone::physics
