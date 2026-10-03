#ifndef CPhysicsShape_h__
#define CPhysicsShape_h__

#include <Workphone/Interface/Physics/IPhysicsShape3.hpp>
#include <Workphone/Interface/Physics/IPhysicsBody3.hpp>
#include <Workphone/Interface/Physics/IPhysicsMaterial3.hpp>
#include <Workphone/Physics/PhysicsShape.hpp>
#include <Workphone/Memory/AtomicSmartPtr.hpp>

namespace workphone
{
    namespace physics
    {

        template <class T>
        class PhysicsShape3 : public PhysicsShape<T>
        {
        public:
            PhysicsShape3();
            ~PhysicsShape3() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            bool isValid() const override;

            bool isAttached() const override;

            SmartPtr<IPhysicsMaterial3> getMaterial() const;

            void setMaterial( SmartPtr<IPhysicsMaterial3> material );

            void setLocalPose( const Transform3<real_Num> &pose );

            Transform3<real_Num> getLocalPose() const;

            void setSimulationFilterData( const FilterData &data );

            FilterData getSimulationFilterData() const;

            void setActor( SmartPtr<IPhysicsBody3> body );

            SmartPtr<IPhysicsBody3> getActor() const;

            void _getObject( void **ppObject ) const;

            bool hasShapeData() const;

            SmartPtr<IPhysicsShape3> clone();

            WP_CLASS_REGISTER_TEMPLATE_DECL( PhysicsShape3, T );

        protected:
            AtomicWeakPtr<IPhysicsBody3> m_body;
            AtomicWeakPtr<IPhysicsMaterial3> m_material;
            Transform3<real_Num> m_localPose;
            FilterData m_simulationFilterData;
        };

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone::physics, PhysicsShape3, T, T );

        template <class T>
        PhysicsShape3<T>::PhysicsShape3() = default;

        template <class T>
        PhysicsShape3<T>::~PhysicsShape3() = default;

        template <class T>
        void PhysicsShape3<T>::load( SmartPtr<ISharedObject> data )
        {
            PhysicsShape<T>::load( data );
        }

        template <class T>
        void PhysicsShape3<T>::unload( SmartPtr<ISharedObject> data )
        {
            PhysicsShape<T>::unload( data );
        }

        template <class T>
        bool PhysicsShape3<T>::isValid() const
        {
            auto valid = PhysicsShape<T>::isValid();
            return valid;
        }

        template <class T>
        bool PhysicsShape3<T>::isAttached() const
        {
            return PhysicsShape3<T>::getActor() != nullptr;
        }

        template <class T>
        SmartPtr<IPhysicsMaterial3> PhysicsShape3<T>::getMaterial() const
        {
            auto p = m_material.load();
            return p.lock();
        }

        template <class T>
        void PhysicsShape3<T>::setMaterial( SmartPtr<IPhysicsMaterial3> material )
        {
            m_material = material;
        }

        template <class T>
        void PhysicsShape3<T>::setLocalPose( const Transform3<real_Num> &pose )
        {
            if( auto stateContext = PhysicsShape3<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<ShapeStateData>() )
                {
                    state->localPose = pose;
                }
            }
        }

        template <class T>
        Transform3<real_Num> PhysicsShape3<T>::getLocalPose() const
        {
            if( auto stateContext = PhysicsShape3<T>::getStateContext() )
            {
                if( auto state = stateContext->template getStateData<ShapeStateData>() )
                {
                    return state->localPose;
                }
            }

            return Transform3<real_Num>();
        }

        template <class T>
        void PhysicsShape3<T>::setSimulationFilterData( const FilterData &data )
        {
            m_simulationFilterData = data;
        }

        template <class T>
        FilterData PhysicsShape3<T>::getSimulationFilterData() const
        {
            return m_simulationFilterData;
        }

        template <class T>
        void PhysicsShape3<T>::setActor( SmartPtr<IPhysicsBody3> body )
        {
            m_body = body;
        }

        template <class T>
        SmartPtr<IPhysicsBody3> PhysicsShape3<T>::getActor() const
        {
            auto p = m_body.load();
            return p.lock();
        }

        template <class T>
        void PhysicsShape3<T>::_getObject( void **ppObject ) const
        {
            *ppObject = nullptr;
        }

        template <class T>
        bool PhysicsShape3<T>::hasShapeData() const
        {
            return false;
        }

        template <class T>
        SmartPtr<IPhysicsShape3> PhysicsShape3<T>::clone()
        {
            return nullptr;
        }

    }  // namespace physics
}  // namespace workphone

#endif  // CPhysicsShape_h__
