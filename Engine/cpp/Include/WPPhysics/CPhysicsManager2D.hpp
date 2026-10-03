#ifndef __PhysicsManager2d__H
#define __PhysicsManager2d__H

#include "WPPhysics/WPPhysicsPrerequisites.hpp"
#include "WPPhysics/CPhysicsManager2.hpp"
#include <Workphone/Interface/Physics/IPhysicsShape2.hpp>
#include "WPPhysics/CRigidBody2.hpp"
#include "WPPhysics/Particle2.hpp"
#include <Workphone/Interface/Physics/IPhysicsManager2D.hpp>
#include <Workphone/Interface/Physics/IPhysicsMaterial2.hpp>
#include <Workphone/System/Job.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * Legacy compatibility facade. New code should use CPhysicsManager2; this class
         * forwards the historical timed API to the production native-backed manager.
         */
        class CPhysicsManager2D : public CPhysicsManager2
        {
        public:
            CPhysicsManager2D();

            ~CPhysicsManager2D() override;

            void update( const s32 &task, const time_interval &t, const time_interval &dt );

            void updateRigidBodies( const s32 &task, const time_interval &t, const time_interval &dt );

            void updateParticles( const s32 &task, const time_interval &t, const time_interval &dt );

            SmartPtr<IPhysicsScene2> addWorld( u32 id ) override;

            void removeWorld( SmartPtr<IPhysicsScene2> world ) override;

            SmartPtr<IPhysicsScene2> findWorld( u32 id ) const override;

            Array<SmartPtr<IPhysicsScene2>> getWorlds() const override;

            SmartPtr<IPhysicsShape2> createCollisionShape( u8 type );

            SmartPtr<IPhysicsShape2> createCollisionShape( const Properties &properties );

            SmartPtr<IRigidBody2> createRigidBody() override;

            SmartPtr<IRigidBody2> createRigidBody( SmartPtr<IPhysicsShape2> collisionShape ) override;

            SmartPtr<IPhysicsParticle2> createParticle(
                u8 particleType, SmartPtr<IPhysicsShape2> collisionShape ) override;

            SmartPtr<IPhysicsSoftBody2> createSoftBody() override;

            void clear() override;

            bool destroyCollisionShape( IPhysicsShape2 *collisionShape );

            bool destroyPhysicsBody( CRigidBody2 *body );

            SmartPtr<IRigidBody2> getRigidBody( u32 id ) const;

            SmartPtr<IPhysicsParticle2> getParticle( u32 id ) const override;

            void OnChangeFlags( IPhysicsBody2D *body ) override;

            bool isColliding( IPhysicsBody2D *bodyA, IPhysicsBody2D *bodyB ) const override;

            u32 getNumActiveRigidBodies() const;

            u32 getNumActiveColRecords() const;

        private:
            class ProcessWorld : public Job
            {
            public:
                ProcessWorld( SmartPtr<IPhysicsScene2> world, const s32 &task, const time_interval &t,
                              const time_interval &dt, int state );

                ~ProcessWorld() override;

                void execute() override;

                SmartPtr<IPhysicsScene2> m_world;
                s32                      m_task;
                time_interval            m_t;
                time_interval            m_dt;
                int                      m_state;
            };
        };

    } // end namespace physics
} // namespace workphone

#endif
