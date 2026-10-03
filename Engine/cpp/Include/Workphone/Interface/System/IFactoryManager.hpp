#ifndef IFactoryManager_h__
#define IFactoryManager_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/System/IFactory.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{

    /** A factory to class to create objects.
     */
    class WPCore_API IFactoryManager : public ISharedObject
    {
    public:
        /** Destructor.
         */
        ~IFactoryManager() override;

        /** Allocates data for the factory manager's factories.
         */
        virtual void allocateData() = 0;

        /** Frees the data allocated for the factory manager's factories.
         */
        virtual void freeData() = 0;

        /** Adds factory to the manager.
         * @param factory
         *   The factory to add.
         */
        virtual void addFactory( SmartPtr<IFactory> factory ) = 0;

        /** Removes a factory from the manager.
         * @param factory
         *   The factory to remove.
         * @return
         *   Returns true if remove the factory is successful.
         */
        virtual bool removeFactory( SmartPtr<IFactory> factory ) = 0;

        /** Removes all the factories from the manager.
         */
        virtual void removeAllFactories() = 0;

        /** Gets a factory by name.
         * @param name
         *   The name of the factory to get.
         * @return
         *   Returns the factory if it exists. Otherwise, it will return null.
         */
        virtual SmartPtr<IFactory> getFactoryByName( const String &name ) const = 0;

        /** Gets a factory by name.
         * @param name
         *   The name of the factory to get.
         * @return
         *   Returns the factory if it exists. Otherwise, it will return null.
         */
        virtual SmartPtr<IFactory> getFactoryByHash( hash64 hash ) const = 0;

        /** Gets a factory by id.
         * @param id
         *   The id of the factory to get.
         * @return
         *   Returns the factory if it exists. Otherwise, it will return null.
         */
        virtual SmartPtr<IFactory> getFactoryById( u32 id ) const = 0;

        /**
         * @brief Finds a factory by name.
         * @param id The name of the factory to find.
         * @return Returns the factory if it exists. Otherwise, it will return null.
         */
        virtual SmartPtr<IFactory> findFactoryById( u32 id ) const = 0;

        /** Checks if the manager has a factory using the name.
         * @param name
         *   The name of the object to check.
         * @return
         *   Returns true if the factory exists.
         */
        virtual bool hasFactoryByName( const String &name ) const = 0;

        /** Checks if the manager has a factory using the type.
         * @param type
         *   The type of the object to check.
         * @return
         *   Returns true if the factory exists.
         */
        virtual bool hasFactoryByType( const String &type ) const = 0;

        /** Checks if the manager has a factory using the id.
         * @param typeId
         *   The type id of the object to check.
         * @return
         *   Returns true if the factory exists.
         */
        virtual bool hasFactoryById( u32 typeId ) const = 0;

        /** Gets an array of factories.
         * @return
         *   Returns an array of factories.
         */
        virtual Array<SmartPtr<IFactory>> getFactories() const = 0;

        /** Creates an object of the type passed.
         * @param typeId
         *   The type id of the object to create.
         * @return
         *   Returns the newly created object. Will return null if no object exists.
         */
        virtual SmartPtr<ISharedObject> createById( u32 typeId ) const = 0;

        /** Creates an object of the type passed.
         * @param typeId
         *   The type id of the object to create.
         * @param hint
         *   The hint to use to create the object.
         */
        virtual SmartPtr<ISharedObject> createById( u32 typeId, const String &hint ) const = 0;

        /** Sets the factory pool size.
         * @param typeId
         *   The type id of the object to set the pool size.
         * @param size
         *   The size of the pool.
         */
        virtual void setPoolSize( u32 typeId, size_Num size ) = 0;

        /** Compare tags of two factories.
         * @param factory1
         *   The first factory to compare.
         * @param factory2
         *   The second factory to compare.
         * @return
         *   Returns true if the tags are the same.
         */
        virtual bool compareTags( const SmartPtr<IFactory> &factory1,
                                  const SmartPtr<IFactory> &factory2 ) = 0;

        /** Check if a factory has a specific tag.
         * @param factory
         *   The factory to check.
         * @param tag
         *   The tag to check.
         * @return
         *   Returns true if the factory has the tag.
         */
        virtual bool hasTag( const SmartPtr<IFactory> &factory, const String &tag ) = 0;

        /** Add a tag to a factory.
         * @param factory
         *   The factory to add the tag.
         * @param tag
         *   The tag to add.
         */
        virtual void addTag( SmartPtr<IFactory> &factory, const String &tag ) = 0;

        /** Remove a tag from a factory.
         * @param factory
         *   The factory to remove the tag.
         * @param tag
         *   The tag to remove.
         */
        virtual void removeTag( SmartPtr<IFactory> &factory, const String &tag ) = 0;

        /** Get all factories that have a specific tag.
         * @param tag
         *   The tag to get the factories.
         * @return
         *   Returns an array of factories that have the tag.
         */
        virtual Array<SmartPtr<IFactory>> getFactoriesWithTag( const String &tag ) = 0;

        /** Creates an object of the type passed.
         * @return
         *     Returns the newly created object. Will return null if no object exists.
         */
        template <class T>
        SmartPtr<T> createObjectFromType( const String &type );

        /** Creates an instance by finding a factory with a concrete
         * implementation by the type info of the template class.
         * @tparam T
         *     The type of the object to create.
         * @return
         *     Returns the newly created object. Will return null if no object exists.
         */
        template <class T>
        SmartPtr<T> make_object();

        /** Creates an instance by finding a factory with a concrete
         * implementation by the type info of the template class.
         * @tparam T
         *     The type of the object to create.
         * @return
         *     Returns the newly created object. Will return null if no object exists.
         */
        template <class T>
        SmartPtr<T> make_object( const String &hint );

        /** Creates an instance by finding a factory with a concrete
         * implementation by the type info of the template class.
         * @tparam T
         *     The type of the object to create.
         * @return
         *     Returns the newly created object. Will return null if no object exists.
         */
        template <class T>
        SmartPtr<T> make_object( u32 type );

        /** Creates an instance. Finds a factory with a concrete implementation
         * to create the object. If no factory is found, it will create a new object.
         * @tparam T
         *   The type of the object to create.
         * @return
         *   Returns the newly created object. Will return null if no object exists.
         */
        template <class T>
        SmartPtr<T> make_ptr();

        /** Creates an instance. Finds a factory with a concrete implementation
         * to create the object. If no factory is found, it will create a new object.
         * @tparam T
         *   The type of the object to create.
         * @tparam _Types
         *   The types of the arguments to pass to the constructor.
         * @return
         *   Returns the newly created object. Will return null if no object exists.
         */
        template <class T, class... _Types>
        SmartPtr<T> make_ptr( _Types &&..._Args );

        /** Creates an instance.
         * @tparam T
         *   The type of the object to create.
         * @return
         *   Returns the newly created object. Will return null if no object exists.
         */
        template <class T>
        SharedPtr<T> make_shared();

        /** Creates an instance.
         * @tparam T
         *   The type of the object to create.
         * @tparam _Types
         *   The types of the arguments to pass to the constructor.
         * @return
         *   Returns the newly created object. Will return null if no object exists.
         */
        template <class T, class... _Types>
        SharedPtr<T> make_shared( _Types &&..._Args );

        /** Gets an array of factories by type.
         * @tparam T
         *   The type of the factory.
         * @return
         *   Returns an array of factories by type.
         */
        template <class T>
        Array<SharedPtr<T>> getFactoriesByType() const;

        /** Sets the factory pool size.
         * @tparam T
         *   The type of the object the factory creates.
         * @param size
         *   The size of the pool.
         */
        template <class T>
        void setPoolSizeByType( size_Num size );

        /** Gets an array of factories by the type of object it creates.
         * @tparam T
         *   The type of object the factory creates.
         * @return
         *   Returns an array of factories by the type of object the factory creates.
         */
        template <class T>
        Array<SmartPtr<IFactory>> getFactoriesByObjectType() const;

        WP_CLASS_REGISTER_DECL;
    };

    template <class T>
    SmartPtr<T> IFactoryManager::make_object()
    {
        auto typeinfo = T::typeInfo();
        WP_ASSERT( typeinfo != 0 );
        return IFactoryManager::make_object<T>( typeinfo );
    }

    template <class T>
    SmartPtr<T> IFactoryManager::make_object( const String &hint )
    {
        auto typeinfo = T::typeInfo();
        WP_ASSERT( typeinfo != 0 );

        auto p = createById( typeinfo, hint );

#if !WP_FINAL
        if( p )
        {
            WP_ASSERT( workphone::dynamic_pointer_cast<T>( p ) );
        }
#endif

        return workphone::static_pointer_cast<T>( p );
    }

    template <class T>
    SmartPtr<T> IFactoryManager::make_object( u32 type )
    {
        auto p = createById( type );

#if !WP_FINAL
        if( p )
        {
            WP_ASSERT( workphone::dynamic_pointer_cast<T>( p ) );
        }
#endif

        return workphone::static_pointer_cast<T>( p );
    }

    template <class T>
    SmartPtr<T> IFactoryManager::make_ptr()
    {
        auto typeinfo = T::typeInfo();
        WP_ASSERT( typeinfo != 0 );

        auto typeManager = TypeManager::instance();
        WP_ASSERT( typeManager );

        auto typeHash = typeManager->getHash( typeinfo );
        WP_ASSERT( typeHash != 0 );

        auto factory = getFactoryByHash( typeHash );
        if( factory )
        {
            WP_ASSERT( factory->isObjectDerivedFromByInfo( T::typeInfo() ) );

            auto p = factory->template make_ptr<T>();
            WP_ASSERT( workphone::dynamic_pointer_cast<T>( p ) );
            return p;
        }

        return workphone::make_ptr<T>();
    }

    template <class T, class... _Types>
    SmartPtr<T> IFactoryManager::make_ptr( _Types &&..._Args )
    {
        auto typeinfo = T::typeInfo();
        WP_ASSERT( typeinfo != 0 );

        auto typeManager = TypeManager::instance();
        WP_ASSERT( typeManager );

        auto typeHash = typeManager->getHash( typeinfo );
        WP_ASSERT( typeHash != 0 );

        auto factory = getFactoryByHash( typeHash );
        if( factory )
        {
            WP_ASSERT( factory->isObjectDerivedFromByInfo( T::typeInfo() ) );

            auto pObject = factory->allocateMemory();
            if( !pObject )
            {
                WP_EXCEPTION( "Unable to allocate memory." );
            }

            T *ptr = nullptr;
            try
            {
                ptr = new( pObject ) T( std::forward<_Types>( _Args )... );
            }
            catch( ... )
            {
                factory->freeMemory( pObject );
                throw;
            }

            if( ptr )
            {
                ptr->setSharedObjectListener( factory->getListener() );
                ptr->setPoolElement( false );
                ptr->setCreatorData( factory.get() );

                WP_ASSERT( ptr->getSharedObjectListener() == factory->getListener() );
                WP_ASSERT( !ptr->isPoolElement() );
                WP_ASSERT( ptr->getCreatorData() == factory.get() );
            }

            auto p = SmartPtr<T>( ptr );

#if !WP_FINAL
            WP_ASSERT( workphone::dynamic_pointer_cast<T>( p ) );
#endif

#if WP_GARBAGE_COLLECTION == 1
            if( ptr )
                ptr->removeReference();
#endif

            return p;
        }

        auto ptr = WP_NEW T( std::forward<_Types>( _Args )... );
        auto p = SmartPtr<T>( ptr );

#if WP_GARBAGE_COLLECTION == 1
        ptr->removeReference();
#endif

        return p;
    }

    template <class T>
    SharedPtr<T> IFactoryManager::make_shared()
    {
        return workphone::make_shared<T>();
    }

    template <class T, class... _Types>
    SharedPtr<T> IFactoryManager::make_shared( _Types &&..._Args )
    {
        return workphone::make_shared<T>( std::forward<_Types>( _Args )... );
    }

    template <class T>
    SmartPtr<T> IFactoryManager::createObjectFromType( const String &type )
    {
        if( auto factory = getFactoryByName( type ) )
        {
            if( factory->isObjectDerivedFromByInfo( T::typeInfo() ) )
            {
                return factory->make_object<T>();
            }
        }

        return nullptr;
    }

    template <class T>
    void IFactoryManager::setPoolSizeByType( size_Num size )
    {
        auto typeInfo = T::typeInfo();
        WP_ASSERT( typeInfo != 0 );
        setPoolSize( typeInfo, size );
    }

    template <class T>
    Array<SharedPtr<T>> IFactoryManager::getFactoriesByType() const
    {
        auto f = getFactories();

        Array<SharedPtr<T>> factories;
        factories.reserve( f.size() );

        for( auto &factory : f )
        {
            if( factory->isDerived<T>() )
            {
                auto derivedFactory = workphone::static_pointer_cast<T>( factory );
                factories.push_back( derivedFactory );
            }
        }

        return factories;
    }

    template <class T>
    Array<SmartPtr<IFactory>> IFactoryManager::getFactoriesByObjectType() const
    {
        const auto factories = getFactories();
        auto objectFactories = Array<SmartPtr<IFactory>>();
        objectFactories.reserve( 32 );

        for( auto factory : factories )
        {
            if( factory->isObjectDerivedFromByInfo( T::typeInfo() ) )
            {
                objectFactories.push_back( factory );
            }
        }

        return objectFactories;
    }

}  // namespace workphone

#endif  // IFactoryManager_h__
