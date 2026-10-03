#ifndef __SubComponent_h__
#define __SubComponent_h__

#include <Workphone/Interface/Scene/ISubComponent.hpp>
#include <Workphone/System/Resource.hpp>

namespace workphone
{
    namespace scene
    {
        /** Base class for a sub component object. */
        class WPCore_API SubComponent : public Resource<ISubComponent>
        {
        public:
            /** Default constructor. */
            SubComponent();

            /** Virtual destructor. */
            ~SubComponent() override;

            /** @copydoc Resource<ISubComponent>::getParentComponent */
            SmartPtr<IComponent> getParentComponent() const override;

            /** @copydoc Resource<ISubComponent>::setParentComponent */
            void setParentComponent( SmartPtr<IComponent> parent ) override;

            /** @copydoc Resource<ISubComponent>::setParentComponent */
            SmartPtr<ISubComponent> getParent() const override;

            /** @copydoc Resource<ISubComponent>::setParentComponent */
            void setParent( SmartPtr<ISubComponent> parent ) override;

            /** @copydoc Resource<ISubComponent>::addChildByType */
            void addChildByType( u32 componentType ) override;

            /** @copydoc Resource<ISubComponent>::addChild */
            void addChild( SmartPtr<ISubComponent> child ) override;

            /** @copydoc Resource<ISubComponent>::removeChild */
            void removeChild( SmartPtr<ISubComponent> child ) override;

            /** @copydoc Resource<ISubComponent>::getChildren */
            Array<SmartPtr<ISubComponent>> getChildren() const override;

            /** @copydoc Resource<ISubComponent>::toData */
            SmartPtr<ISharedObject> toData() const override;

            /** @copydoc Resource<ISubComponent>::fromData */
            void fromData( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Resource<ISubComponent>::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc Resource<ISubComponent>::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            SmartPtr<IComponent> m_parentComponent;

            SmartPtr<ISubComponent> m_parent;

            Array<SmartPtr<ISubComponent>> m_children;

            /**
             * The ID extension of the component.
             */
            static u32 m_idExt;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // SubComponent_h__
