#ifndef TransformReferenceGuard_h__
#define TransformReferenceGuard_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Atomics/AtomicFloat.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Atomics/AtomicObject.hpp>
#include <Workphone/Atomics/AtomicValue.hpp>

namespace workphone
{
    namespace scene
    {

        template <typename T>
        struct TransformReferenceGuard
        {
            explicit TransformReferenceGuard( SmartPtr<T> object ) : m_object( object )
            {
                if( m_object )
                {
                    m_object->addTransformReference();
                }
            }

            ~TransformReferenceGuard()
            {
                if( m_object )
                {
                    m_object->removeTransformReference();
                    m_object = nullptr;
                }
            }

            SmartPtr<T> m_object;
        };

    }  // namespace scene
}  // namespace workphone

#endif  // TransformReferenceGuard_h__
