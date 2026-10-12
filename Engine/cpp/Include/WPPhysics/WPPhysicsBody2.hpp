#ifndef WPPHYSICSBODY2_HPP
#define WPPHYSICSBODY2_HPP

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Physics/IPhysicsBody2D.hpp>
#include <Workphone/Interface/Physics/IPhysicsConstraint2.hpp>
#include <algorithm>

namespace workphone::physics
{
    template <class T>
    class WPPhysicsBody2 : public T
    {
    public:
        WPPhysicsBody2()
        {
        }

        ~WPPhysicsBody2() override
        {
        }

        virtual bool getEnableGravity() const
        {
            return m_enableGravity;
        }

        virtual void setEnableGravity( bool enableGravity )
        {
            m_enableGravity = enableGravity;
        }

        void setCollisionMask( u32 mask )
        {
            m_collisionMask = mask;
        }

        u32 getCollisionMask() const
        {
            return m_collisionMask;
        }

        virtual Array<SmartPtr<IPhysicsConstraint2>> getConstraints() const
        {
            return m_constraints;
        }

        virtual void removeConstraints()
        {
            m_constraints.clear();
        }

        virtual void removeConstraint( SmartPtr<IPhysicsConstraint2> constraint )
        {
            m_constraints.erase( std::remove( m_constraints.begin(), m_constraints.end(), constraint ),
                                 m_constraints.end() );
        }

        virtual void addConstraint( SmartPtr<IPhysicsConstraint2> constraint )
        {
            if( constraint && std::find( m_constraints.begin(), m_constraints.end(), constraint ) ==
                                  m_constraints.end() )
            {
                m_constraints.push_back( constraint );
            }
        }

    protected:
        atomic_u32 m_collisionMask{ 0xFFFFFFFFu };
        bool m_enableGravity = true;
        Array<SmartPtr<IPhysicsConstraint2>> m_constraints;
    };
}  // namespace workphone::physics

#endif  // WPPHYSICSBODY2_HPP
