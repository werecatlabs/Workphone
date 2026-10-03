#ifndef ISubComponent_h__
#define ISubComponent_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/System/IResource.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @file ISubComponent.h
         * @brief Interface for a sub component.
         *
         * This class defines an interface for a sub component of a larger component in an ECS system.
         * Sub components can be thought of as optional pieces of functionality that can be attached
         * to a component, and may be implemented as separate classes derived from this interface.
         * Sub components should provide their own implementation for the methods defined here.
         *
         * @author Zane Desir
         * @version 1.0
         */
        class WPCore_API ISubComponent : public IResource
        {
        public:
            ISubComponent();

            ISubComponent( u32 poolTypeId );

            /** Virtual destructor. */
            ~ISubComponent() override;

            /**
             * Get the parent component of this component.
             * @return A shared pointer to the parent component.
             */
            virtual SmartPtr<IComponent> getParentComponent() const = 0;

            /**
             * Set the parent component of this component.
             * @param parentComponent A shared pointer to the parent component.
             */
            virtual void setParentComponent( SmartPtr<IComponent> parentComponent ) = 0;

            /**
             * Get the parent entity of this component.
             * @return A shared pointer to the parent entity.
             */
            virtual SmartPtr<ISubComponent> getParent() const = 0;

            /**
             * Set the parent entity of this component.
             * @param parent A shared pointer to the parent entity.
             */
            virtual void setParent( SmartPtr<ISubComponent> parent ) = 0;

            /**
             * Get the type of the component.
             * @return The type of the component.
             */
            virtual void addChildByType( u32 componentType ) = 0;

            /**
             * Get the type of the component.
             * @return The type of the component.
             */
            virtual void addChild( SmartPtr<ISubComponent> child ) = 0;

            /**
             * Get the type of the component.
             * @return The type of the component.
             */
            virtual void removeChild( SmartPtr<ISubComponent> child ) = 0;

            /**
             * Get the type of the component.
             * @return The type of the component.
             */
            virtual Array<SmartPtr<ISubComponent>> getChildren() const = 0;

            // 'c' style linked list.
            AtomicRawPtr<ISubComponent> m_next;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace scene
}  // namespace workphone

#endif  // ISubComponent_h__
