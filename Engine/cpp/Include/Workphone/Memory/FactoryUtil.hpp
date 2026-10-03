#ifndef __FactoryUtil_h__
#define __FactoryUtil_h__

#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/System/FactoryTemplate.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone
{

    /**
     * @class FactoryUtil
     * @brief FactoryUtil is a utility class providing common factory functions
     * that my be used in an application.
     */
    class FactoryUtil
    {
    public:
        /**
         * @brief Adds a factory for the specified type.
         * @tparam T The type for which to add a factory.
         */
        template <class T>
        static void addFactory();

        /**
         * @brief Adds a factory for the specified type.
         * @tparam T The type for which to add a factory.
         * @param factoryManager The factory manager to which to add the factory.
         */
        template <class T>
        static void addFactory( SmartPtr<IFactoryManager> factoryManager );

        /**
         * @brief Adds a factory for the specified type.
         * @tparam T The type for which to add a factory.
         * @param name The name of the type.
         */
        template <class T>
        static void addFactoryByName( const String &name );

        /**
         * @brief Adds a factory for the specified type.
         * @tparam T The type for which to add a factory.
         * @param name The name of the type.
         * @param poolSize The size of the object pool.
         */
        template <class T>
        static void addFactoryByName( const String &name, u32 poolSize );

        /**
         * @brief Removes a factory for the specified type.
         * @tparam T The type for which to remove a factory.
         */
        template <class T>
        static void removeFactory();

        /**
         * @brief Removes a factory for the specified type.
         * @tparam T The type for which to remove a factory.
         * @param factoryManager The factory manager from which to remove the factory.
         */
        template <class T>
        static void removeFactory( SmartPtr<IFactoryManager> factoryManager );
    };

    template <class T>
    void FactoryUtil::addFactory()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        if( !factoryManager )
        {
            WP_LOG_ERROR( "Factory manager not found." );
            return;
        }

        WP_ASSERT( factoryManager->hasFactoryById( T::typeInfo() ) == false );

        auto typeManager = TypeManager::instance();
        WP_ASSERT( typeManager );

        auto typeInfo = T::typeInfo();
        auto name = typeManager->getName( typeInfo );
        auto hash = typeManager->getHash( typeInfo );

        auto pFactory = workphone::make_ptr<FactoryTemplate<T>>();
        pFactory->load( nullptr );
        pFactory->setObjectTypeName( name );
        pFactory->setObjectTypeHash( hash );

        factoryManager->addFactory( pFactory );
    }

    template <class T>
    void FactoryUtil::addFactory( SmartPtr<IFactoryManager> factoryManager )
    {
        if( !factoryManager )
        {
            WP_LOG_ERROR( "Factory manager not found." );
            return;
        }

        WP_ASSERT( factoryManager->hasFactoryById( T::typeInfo() ) == false );

        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto typeManager = TypeManager::instance();

        auto typeInfo = T::typeInfo();
        auto name = typeManager->getName( typeInfo );
        auto hash = typeManager->getHash( typeInfo );

        auto pFactory = workphone::make_ptr<FactoryTemplate<T>>();
        pFactory->load( nullptr );
        pFactory->setObjectTypeName( name );
        pFactory->setObjectTypeHash( hash );

        factoryManager->addFactory( pFactory );
    }

    template <class T>
    void FactoryUtil::addFactoryByName( const String &name )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        if( !factoryManager )
        {
            WP_LOG_ERROR( "Factory manager not found." );
            return;
        }

        WP_ASSERT( factoryManager->hasFactoryByName( name ) == false );

        auto hash = StringUtil::getHash64( name );

        auto pFactory = workphone::make_ptr<FactoryTemplate<T>>();
        pFactory->load( nullptr );
        pFactory->setObjectTypeName( name );
        pFactory->setObjectTypeHash( hash );

        factoryManager->addFactory( pFactory );
    }

    template <class T>
    void FactoryUtil::addFactoryByName( const String &name, u32 poolSize )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        if( !factoryManager )
        {
            WP_LOG_ERROR( "Factory manager not found." );
            return;
        }

        WP_ASSERT( factoryManager->hasFactoryByName( name ) == false );

        auto hash = StringUtil::getHash64( name );

        auto pFactory = workphone::make_ptr<FactoryTemplate<T>>();

        pFactory->load( nullptr );
        pFactory->setObjectTypeName( name );
        pFactory->setObjectTypeHash( hash );

        factoryManager->addFactory( pFactory );
        factoryManager->setPoolSize( hash, poolSize );
    }

    template <class T>
    void FactoryUtil::removeFactory()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        if( !factoryManager )
        {
            WP_LOG_ERROR( "Factory manager not found." );
            return;
        }

        auto typeManager = TypeManager::instance();
        WP_ASSERT( typeManager );

        auto typeInfo = T::typeInfo();
        if( auto factory = factoryManager->getFactoryById( typeInfo ) )
        {
            factoryManager->removeFactory( factory );
        }
    }

    template <class T>
    void FactoryUtil::removeFactory( SmartPtr<IFactoryManager> factoryManager )
    {
        if( !factoryManager )
        {
            WP_LOG_ERROR( "Factory manager not found." );
            return;
        }

        auto typeInfo = T::typeInfo();
        if( auto factory = factoryManager->getFactoryById( typeInfo ) )
        {
            factoryManager->removeFactory( factory );
        }
    }

}  // namespace workphone

#endif  // __FactoryUtil_h__
