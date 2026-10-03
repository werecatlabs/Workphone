#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/FactoryManager.hpp>
#include <Workphone/System/Factory.hpp>
#include <Workphone/Core/Exception.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Interface/System/IState.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, FactoryManager, IFactoryManager );

    FactoryManager::FactoryManager()
    {
        static const auto name = String( "FactoryManager" );
        setName( name );
    }

    FactoryManager::~FactoryManager() = default;

    void FactoryManager::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        m_factories.reserve( 2048 );
        setLoadingState( LoadingState::Loaded );
    }

    void FactoryManager::unload( SmartPtr<ISharedObject> data )
    {
        if( !isLoaded() )
        {
            return;
        }

        setLoadingState( LoadingState::Unloading );

        auto factories = m_factories.snapshot();

        for( auto &factory : factories )
        {
            if( factory )
            {
                auto objects = factory->getInstanceObjects();
                for( auto &object : objects )
                {
                    if( object && object->isAlive() && object->getReferences() > 0 )
                    {
                        object->unload( nullptr );
                    }
                }
            }
        }

        for( auto &factory : factories )
        {
            if( factory )
            {
                factory->unload( nullptr );
            }
        }

        removeAllFactories();

        setLoadingState( LoadingState::Unloaded );
    }

    void FactoryManager::allocateData()
    {
        for( auto &factory : m_factories )
        {
            factory->allocatePoolData();
        }
    }

    void FactoryManager::freeData()
    {
        for( auto &factory : m_factories )
        {
            factory->freePoolData();
        }
    }

    s32 FactoryManager::getFactoryUnloadPriority( SmartPtr<IFactory> factory )
    {
        if( factory->isObjectDerivedFrom<IStateContext>() )
        {
            return 2000;
        }
        if( factory->isObjectDerivedFrom<IState>() )
        {
            return 1000;
        }

        return 0;
    }

    void FactoryManager::addFactory( SmartPtr<IFactory> factory )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto typeManager = TypeManager::instance();
            auto typeId = factory->getTypeInfo();
            auto typeName = typeManager->getName( typeId );

            if( !factory->isLoaded() )
            {
                factory->load( nullptr );
            }

            factory->setName( typeName );

            auto objectType = factory->getObjectTypeName();

            //WP_ASSERT( hasFactory( objectType ) == false );

#if !WP_FINAL
            if( hasFactoryByType( objectType ) )
            {
                auto existingFactory = getFactoryByName( objectType );
                WP_LOG_ERROR( "FactoryManager::addFactory: Factory for type " + String( objectType ) +
                              " already exists! " + existingFactory->getObjectTypeName() );
            }
#endif

            if( !hasFactoryByType( objectType ) )
            {
                m_factories.push_back( factory );

                std::sort( m_factories.begin(), m_factories.end(),
                           []( SmartPtr<IFactory> a, SmartPtr<IFactory> b ) {
                               return a < b;  // compare by memory address
                           } );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    bool FactoryManager::removeFactory( SmartPtr<IFactory> factory )
    {
        m_factories.erase( std::remove( m_factories.begin(), m_factories.end(), factory ),
                           m_factories.end() );

        if( factory )
        {
            factory->unload( nullptr );
            return true;
        }

        return false;
    }

    void FactoryManager::removeAllFactories()
    {
        m_factories.clear();
    }

    SmartPtr<IFactory> FactoryManager::getFactoryByName( const String &name ) const
    {
        for( auto &factory : m_factories )
        {
            if( factory )
            {
                auto objectType = factory->getObjectTypeName();
                if( StringUtil::isEqual( objectType, name ) )
                {
                    return factory;
                }
            }
        }

        auto splitNames = StringUtil::split( name, "::" );
        std::reverse( splitNames.begin(), splitNames.end() );

        for( auto &factory : m_factories )
        {
            if( factory )
            {
                auto objectType = factory->getObjectTypeName();

                auto splitFactroyNames = StringUtil::split( objectType, "::" );
                std::reverse( splitFactroyNames.begin(), splitFactroyNames.end() );

                if( StringUtil::isEqual( splitFactroyNames.front(), splitNames.front() ) )
                {
                    return factory;
                }
            }
        }

        return nullptr;
    }

    SmartPtr<IFactory> FactoryManager::getFactoryByHash( hash64 hash ) const
    {
        for( auto &factory : m_factories )
        {
            if( factory )
            {
                auto objectTypeHash = factory->getObjectTypeHash();
                if( objectTypeHash == hash )
                {
                    return factory;
                }
            }
        }

        return nullptr;
    }

    SmartPtr<IFactory> FactoryManager::getFactoryById( u32 id ) const
    {
        WP_ASSERT( id != 0 );
        WP_ASSERT( isLoaded() );

        for( auto &factory : m_factories )
        {
            if( factory )
            {
                auto objectTypeId = factory->getObjectTypeId();
                if( objectTypeId == id )
                {
                    return factory;
                }
            }
        }

        return nullptr;
    }

    SmartPtr<IFactory> FactoryManager::findFactoryById( u32 id ) const
    {
        WP_ASSERT( id != 0 );
        WP_ASSERT( isLoaded() );

        for( auto &factory : m_factories )
        {
            if( factory )
            {
                auto objectTypeId = factory->getObjectTypeId();
                if( objectTypeId == id )
                {
                    return factory;
                }
            }
        }

        auto typeManager = TypeManager::instance();
        auto derivedTypes = typeManager->getDerivedTypes( id );
        std::reverse( derivedTypes.begin(), derivedTypes.end() );

        for( auto &derivedType : derivedTypes )
        {
            if( auto factory = getFactoryById( derivedType ) )
            {
                return factory;
            }
        }

        return nullptr;
    }

    bool FactoryManager::hasFactoryByName( const String &name ) const
    {
        WP_ASSERT( !StringUtil::isNullOrEmpty( name ) );
        WP_ASSERT( isLoaded() );

        for( auto &factory : m_factories )
        {
            if( factory )
            {
                auto factoryName = factory->getObjectTypeName();
                if( StringUtil::isEqual( factoryName, name ) )
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool FactoryManager::hasFactoryByType( const String &type ) const
    {
        WP_ASSERT( !StringUtil::isNullOrEmpty( type ) );
        WP_ASSERT( isLoaded() );

        for( auto &factory : m_factories )
        {
            if( factory )
            {
                auto factoryType = factory->getObjectTypeName();
                if( StringUtil::isEqual( factoryType, type ) )
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool FactoryManager::hasFactoryById( u32 typeId ) const
    {
        WP_ASSERT( typeId != 0 );
        WP_ASSERT( isLoaded() );

        auto typeManager = TypeManager::instance();

        for( auto &factory : m_factories )
        {
            if( factory )
            {
                auto objectTypeHash = factory->getObjectTypeHash();
                auto objectTypeId = typeManager->getIdFromHash( objectTypeHash );
                if( objectTypeId == typeId )
                {
                    return true;
                }
            }
        }

        return false;
    }

    Array<SmartPtr<IFactory>> FactoryManager::getFactories() const
    {
        return m_factories.snapshot();
    }

    SmartPtr<ISharedObject> FactoryManager::createById( u32 typeId ) const
    {
        WP_ASSERT( typeId != 0 );
        WP_ASSERT( isLoaded() );

        auto typeManager = TypeManager::instance();
        WP_ASSERT( typeManager );

        auto factory = getFactoryById( typeId );

        if( !factory )
        {
            auto derivedTypes = typeManager->getDerivedTypes( typeId );
            std::reverse( derivedTypes.begin(), derivedTypes.end() );

            for( auto &derivedType : derivedTypes )
            {
                factory = getFactoryById( derivedType );
                if( factory )
                {
                    break;
                }

                auto factoryHash = typeManager->getHash( derivedType );
                factory = getFactoryByHash( factoryHash );
                if( factory )
                {
                    break;
                }

                auto factoryName = typeManager->getName( derivedType );
                factory = getFactoryByName( factoryName );
                if( factory )
                {
                    break;
                }
            }
        }

        if( !factory )
        {
            auto typeHash = typeManager->getHash( typeId );
            factory = getFactoryByHash( typeHash );
        }

        if( !factory )
        {
            auto typeName = typeManager->getName( typeId );
            factory = getFactoryByName( typeName );
        }

        if( factory )
        {
            auto ptr = static_cast<ISharedObject *>( factory->createObject() );
            auto p = SmartPtr<ISharedObject>( ptr );
            ptr->removeReference();
            return p;
        }

        return nullptr;
    }

    SmartPtr<ISharedObject> FactoryManager::createById( u32 typeId, const String &hint ) const
    {
        WP_ASSERT( typeId != 0 );
        WP_ASSERT( isLoaded() );

        auto typeManager = TypeManager::instance();
        WP_ASSERT( typeManager );

        auto typeHash = typeManager->getHash( static_cast<u32>( typeId ) );
        WP_ASSERT( typeHash != 0 );

        auto factory = getFactoryByHash( typeHash );
        if( !factory )
        {
            auto typeName = typeManager->getName( static_cast<u32>( typeId ) );
            factory = getFactoryByName( typeName );
        }

        if( !factory )
        {
            auto derivedTypes = typeManager->getDerivedTypes( static_cast<u32>( typeId ) );

            auto score = -1e10;
            for( auto derivedType : derivedTypes )
            {
                auto factoryName = typeManager->getName( derivedType );

                auto factoryScore =
                    static_cast<f64>( StringUtil::numCommonSubsequence( factoryName, hint ) );
                if( factoryScore > score )
                {
                    score = factoryScore;

                    auto factoryHash = typeManager->getHash( derivedType );
                    WP_ASSERT( factoryHash != 0 );

                    factory = getFactoryByHash( factoryHash );
                }
            }
        }

        if( factory )
        {
            auto ptr = static_cast<ISharedObject *>( factory->createObject() );
            auto p = SmartPtr<ISharedObject>( ptr );
            ptr->removeReference();
            return p;
        }

        return nullptr;
    }

    void FactoryManager::setPoolSize( u32 typeId, size_t size )
    {
        if( auto factory = getFactoryById( typeId ) )
        {
            factory->setGrowSize( static_cast<u32>( size ) );
            factory->allocatePoolData();
        }
    }

    bool FactoryManager::compareTags( const SmartPtr<IFactory> &factory1,
                                      const SmartPtr<IFactory> &factory2 )
    {
        return factory1->getTags() == factory2->getTags();
    }

    bool FactoryManager::hasTag( const SmartPtr<IFactory> &factory, const String &tag )
    {
        const auto &tags = factory->getTags();
        return std::find( tags.begin(), tags.end(), tag ) != tags.end();
    }

    void FactoryManager::addTag( SmartPtr<IFactory> &factory, const String &tag )
    {
        auto tags = factory->getTags();
        if( std::find( tags.begin(), tags.end(), tag ) == tags.end() )
        {
            tags.push_back( tag );
            factory->setTags( tags );
        }
    }

    void FactoryManager::removeTag( SmartPtr<IFactory> &factory, const String &tag )
    {
        auto tags = factory->getTags();
        auto it = std::remove( tags.begin(), tags.end(), tag );
        if( it != tags.end() )
        {
            tags.erase( it, tags.end() );
            factory->setTags( tags );
        }
    }

    Array<SmartPtr<IFactory>> FactoryManager::getFactoriesWithTag( const String &tag )
    {
        Array<SmartPtr<IFactory>> result;
        result.reserve( 4 );

        for( auto &factory : m_factories )
        {
            if( hasTag( factory, tag ) )
            {
                result.push_back( factory );
            }
        }

        return result;
    }

    void FactoryManager::lock()
    {
        m_mutex.lock();
    }

    bool FactoryManager::try_lock()
    {
        return m_mutex.try_lock();
    }

    void FactoryManager::unlock()
    {
        m_mutex.unlock();
    }

}  // namespace workphone
