#ifndef PhysicsConstraint_h__
#define PhysicsConstraint_h__

#include <Workphone/Interface/Physics/IPhysicsConstraint.hpp>
#include <Workphone/Physics/Physics3SharedObject.hpp>
#include <Workphone/Core/Properties.hpp>

namespace workphone
{
    namespace physics
    {

        template <class T>
        class PhysicsConstraint : public Physics3SharedObject<T>
        {
        public:
            PhysicsConstraint();
            ~PhysicsConstraint() override;

            /** @copydoc Physics3SharedObject<T>::getChildObjects */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /** @copydoc Physics3SharedObject<T>::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc Physics3SharedObject<T>::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            WP_CLASS_REGISTER_TEMPLATE_DECL( PhysicsConstraint, T );
        };

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone::physics, PhysicsConstraint, T,
                                            Physics3SharedObject<T> );

        template <class T>
        PhysicsConstraint<T>::PhysicsConstraint()
        {
        }

        template <class T>
        PhysicsConstraint<T>::~PhysicsConstraint()
        {
        }

        template <class T>
        Array<SmartPtr<ISharedObject>> PhysicsConstraint<T>::getChildObjects() const
        {
            return Array<SmartPtr<ISharedObject>>();
        }

        template <class T>
        SmartPtr<Properties> PhysicsConstraint<T>::getProperties() const
        {
            auto properties = workphone::make_ptr<Properties>();
            return properties;
        }

        template <class T>
        void PhysicsConstraint<T>::setProperties( SmartPtr<Properties> properties )
        {
        }

    }  // namespace physics
}  // namespace workphone

#endif  // PhysicsConstraint_h__
