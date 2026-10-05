#ifndef WPPHYSICSSCENE2_HPP
#define WPPHYSICSSCENE2_HPP

#include <WPPhysics/WPPhysicsPrerequisites.hpp>
#include <Workphone/Interface/Physics/IPhysicsScene2.hpp>

extern "C" {
#include <WorkphonePhysics/workphone_physics_2d.h>
}

namespace workphone::physics
{
    class WPPhysicsScene2 : public IPhysicsScene2
    {
    public:
        WPPhysicsScene2();
        ~WPPhysicsScene2() override;

        void *getNativeObject() const;
        void _getObject( void **object ) const override;
        wp_physics_scene *getScene() const;

        void updateRigidBodies() override;
        void updateParticles() override;
        void addRigidBody( SmartPtr<IRigidBody2> body ) override;
        void removeRigidBody( SmartPtr<IRigidBody2> body ) override;
        void addParticle( SmartPtr<IPhysicsParticle2> particle ) override;
        void removeParticle( SmartPtr<IPhysicsParticle2> particle ) override;
        void setSize( const Vector2<real_Num> &size ) override;
        Vector2<real_Num> getSize() const override;
        void setGravity( const Vector2<real_Num> &gravity ) override;
        Vector2<real_Num> getGravity() const override;
        void simulate( real_Num elapsedTime );
        void setFixedTimeStep( real_Num fixedTimeStep );
        real_Num getFixedTimeStep() const;

    private:
        wp_physics_scene *m_scene = nullptr;
        Array<SmartPtr<IRigidBody2>> m_bodies;
        Array<SmartPtr<IPhysicsParticle2>> m_particles;
        real_Num m_fixedTimeStep = static_cast<real_Num>( 1.0 / 60.0 );
    };
}  // namespace workphone::physics

#endif
