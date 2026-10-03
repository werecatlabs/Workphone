#ifndef Prototype_h__
#define Prototype_h__

#include <Workphone/Interface/IPrototype.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    namespace core
    {

        /** Template class to implement the IPrototype interface. */
        template <class T>
        class Prototype : public T
        {
        public:
            /** Constructor. */
            Prototype();

            Prototype( u32 poolTypeInfo );

            /** Destructor. */
            ~Prototype() override;

            /** @copydoc IPrototype::getParentPrototype */
            SmartPtr<IPrototype> getParentPrototype() const override;

            /** @copydoc IPrototype::setParentPrototype */
            void setParentPrototype( SmartPtr<IPrototype> prototype ) override;

            /** Gets the data as a properties object.
            @return The data as a properties object.
            */
            SmartPtr<Properties> getProperties() const override;

            /** Sets the data as a properties object.
            @param properties The properties object.
            */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** @copydoc IPrototype::clone */
            virtual SmartPtr<IPrototype> clone();

            WP_CLASS_REGISTER_TEMPLATE_DECL( Prototype, T );

        protected:
            // The parent prototype.
            SmartPtr<IPrototype> m_parentPrototype;
        };

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone::core, Prototype, T, T );

        template <class T>
        Prototype<T>::Prototype() : T( Prototype<T>::typeInfo() )
        {
        }

        template <class T>
        Prototype<T>::Prototype( u32 poolTypeInfo ) : T( poolTypeInfo )
        {
        }

        template <class T>
        Prototype<T>::~Prototype() = default;

        template <class T>
        SmartPtr<IPrototype> Prototype<T>::getParentPrototype() const
        {
            return m_parentPrototype;
        }

        template <class T>
        void Prototype<T>::setParentPrototype( SmartPtr<IPrototype> prototype )
        {
            m_parentPrototype = prototype;
        }

        template <class T>
        SmartPtr<Properties> Prototype<T>::getProperties() const
        {
            auto properties = ISharedObject::getProperties();
            return properties;
        }

        template <class T>
        void Prototype<T>::setProperties( SmartPtr<Properties> properties )
        {
            bool wasLoaded = Prototype<T>::isLoaded();
            bool loaded = wasLoaded;

            properties->getPropertyValue( ISharedObject::loadedStr, loaded );

            if( wasLoaded != loaded )
            {
                if( wasLoaded )
                {
                    Prototype<T>::unload( nullptr );
                }
                else
                {
                    Prototype<T>::load( nullptr );
                }
            }
        }

        template <class T>
        SmartPtr<IPrototype> Prototype<T>::clone()
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto type = this->getTypeInfo();

            SmartPtr<IPrototype> instance = factoryManager->createById( type );

            if( auto properties = this->getProperties() )
            {
                instance->setProperties( properties );
            }

            return instance;
        }

    }  // namespace core
}  // namespace workphone

#endif  // Prototype_h__
