#ifndef WPPHYSICSMANAGER2_HPP
#define WPPHYSICSMANAGER2_HPP

#include <WPPhysics/WPPhysicsPrerequisites.hpp>
#include <Workphone/Interface/Physics/IPhysicsManager2D.hpp>

namespace workphone::physics
{
    class WPPhysicsManager2 : public IPhysicsManager2D
    {
    public:
        WPPhysicsManager2();
        ~WPPhysicsManager2() override;

        void *getNativeObject() const override;

        void updateRigidBodies() override;
        void updateParticles() override;
        SmartPtr<IPhysicsScene2> addWorld( u32 id ) override;
        void removeWorld( SmartPtr<IPhysicsScene2> world ) override;
        SmartPtr<IPhysicsScene2> findWorld( u32 id ) const override;
        Array<SmartPtr<IPhysicsScene2>> getWorlds() const override;
        SmartPtr<IPhysicsShape2> createCollisionShapeByType( hash32 type ) override;
        SmartPtr<IRigidBody2> createRigidBody() override;
        SmartPtr<IRigidBody2> createRigidBody( SmartPtr<IPhysicsShape2> collisionShape ) override;
        SmartPtr<IPhysicsParticle2> createParticle( u8 particleType,
                                                    SmartPtr<IPhysicsShape2> collisionShape ) override;
        SmartPtr<IPhysicsSoftBody2> createSoftBody() override;
        void clear() override;
        SmartPtr<IPhysicsParticle2> getParticle( u32 id ) const override;
        SmartPtr<IRigidBody2> getRigidBody( u32 id ) const;
        bool removeRigidBody( IRigidBody2 *body );
        bool removeCollisionShape( IPhysicsShape2 *shape );
        u32 getRigidBodyCount() const;
        void OnChangeFlags( IPhysicsBody2D *body ) override;
        bool isColliding( IPhysicsBody2D *bodyA, IPhysicsBody2D *bodyB ) const override;

    private:
        Array<SmartPtr<IPhysicsScene2>> m_worlds;
        Array<u32> m_worldIds;
        Array<SmartPtr<IPhysicsShape2>> m_shapes;
        Array<SmartPtr<IRigidBody2>> m_bodies;
        Array<SmartPtr<IPhysicsParticle2>> m_particles;
        Array<SmartPtr<IPhysicsSoftBody2>> m_softBodies;
    };
}  // namespace workphone::physics

#endif
