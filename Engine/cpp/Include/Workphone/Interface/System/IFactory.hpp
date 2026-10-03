#ifndef _FBObjectFactory_H_
#define _FBObjectFactory_H_

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Memory/PointerUtil.hpp>

namespace workphone
{

    /**
     * @class IFactory
     * @brief Interface for a factory class to create and manage objects and their memory.
     *
     * This interface provides methods for object creation, memory management, type information,
     * and tagging. It supports both raw and smart pointer object creation, array allocation,
     * and listener management for shared objects. Factories implementing this interface can be
     * used to efficiently manage object lifecycles and memory pools in a type-safe manner.
     */
    class WPCore_API IFactory : public ISharedObject
    {
    public:
        /**
         * @brief Virtual destructor.
         */
        ~IFactory() override;

        /**
         * @brief Checks if the object is derived from the given type info.
         * @param typeInfo The type information hash to check against.
         * @return True if the object is derived from the given type info, false otherwise.
         */
        virtual bool isObjectDerivedFromByInfo( u32 typeInfo ) const = 0;

        /**
         * @brief Gets the type name of the object this factory creates.
         * @return The type name as a C-style string.
         */
        virtual const c8 *getTypeNamePtr() const = 0;

        /**
         * @brief Gets the type name of the object this factory creates.
         * @return The type name as a C-style string.
         */
        virtual String getTypeName() const = 0;

        /**
         * @brief Sets the type name of the object this factory creates.
         * @param typeName The type name as a String.
         */
        virtual void setTypeName( const String &typeName ) = 0;

        /**
         * @brief Gets the grow size for the memory pool.
         * @return The grow size (number of objects to allocate when growing the pool).
         */
        virtual u32 getGrowSize() const = 0;

        /**
         * @brief Sets the grow size for the memory pool.
         * @param size The grow size (number of objects to allocate when growing the pool).
         */
        virtual void setGrowSize( u32 size ) = 0;

        /**
         * @brief Allocates memory for the pool data.
         */
        virtual void allocatePoolData() = 0;

        /**
         * @brief Frees memory for the pool data.
         */
        virtual void freePoolData() = 0;

        /**
         * @brief Allocates memory for a single object.
         * @return Pointer to the allocated memory.
         */
        virtual void *allocateMemory() = 0;

        /**
         * @brief Frees the memory for a single object.
         * @param ptr Pointer to the memory to free.
         */
        virtual void freeMemory( void *ptr ) = 0;

        /**
         * @brief Creates an object instance.
         * @return Pointer to the created object.
         */
        virtual void *createObject() = 0;

        /**
         * @brief Frees an object created by the pool.
         * @param object Pointer to the object to free.
         */
        virtual void freeObject( void *object ) = 0;

        /**
         * @brief Creates an array of objects.
         * @param numElements Number of elements in the array.
         * @return Pointer to the created array.
         */
        virtual void *createArray( u32 numElements ) = 0;

        /**
         * @brief Gets the type name of the object this factory creates (alternative accessor).
         * @return The type name as a C-style string.
         */
        virtual const c8 *getObjectTypeNamePtr() const = 0;

        /**
         * @brief Gets the type name of the object this factory creates (alternative accessor).
         * @return The type name as a C-style string.
         */
        virtual String getObjectTypeName() const = 0;

        /**
         * @brief Sets the type name of the object this factory creates.
         * @param type The type name as a String.
         */
        virtual void setObjectTypeName( const String &type ) = 0;

        /**
         * @brief Gets the type hash of the object this factory creates.
         * @return The type hash (unique identifier for the type).
         */
        virtual hash_type getObjectTypeHash() const = 0;

        /**
         * @brief Sets the type hash of the object this factory creates.
         * @param id The type hash (unique identifier for the type).
         */
        virtual void setObjectTypeHash( hash_type hash ) = 0;

        /**
         * @brief Gets the unique id of the type of object this factory creates.
         * @return The unique type id.
         */
        virtual u32 getObjectTypeId() const = 0;

        /**
         * @brief Sets the unique id of the type of object this factory creates.
         * @param objectTypeId The unique type id.
         */
        virtual void setObjectTypeId( u32 objectTypeId ) = 0;

        /**
         * @brief Gets the size of the object this factory creates.
         * @return The size of the object in bytes.
         */
        virtual u32 getObjectSize() const = 0;

        /**
         * @brief Sets the size of the object this factory creates.
         * @param objectSize The size of the object in bytes.
         */
        virtual void setObjectSize( u32 objectSize ) = 0;

        /**
         * @brief Gets the amount of memory used by the factory.
         * @return The amount of memory used (in bytes).
         */
        virtual s32 getMemoryUsed() const = 0;

        /**
         * @brief Gets the factory's shared object listener.
         * @return Pointer to the shared object listener.
         */
        virtual ISharedObjectListener *getListener() const = 0;

        /**
         * @brief Gets the instantiated objects managed by the factory.
         * @return Array of smart pointers to instantiated shared objects.
         */
        virtual Array<SmartPtr<ISharedObject>> getInstanceObjects() const = 0;

        /**
         * @brief Gets the tags associated with this factory.
         * @return Array of tags as strings.
         */
        virtual Array<String> getTags() const = 0;

        /**
         * @brief Sets the tags associated with this factory.
         * @param tags Array of tags as strings.
         */
        virtual void setTags( const Array<String> &tags ) = 0;

        /**
         * @brief Checks if the factory has an internal memory pool.
         * @return True if the factory has a memory pool, false otherwise.
         */
        virtual bool hasPool() const = 0;

        /**
         * @brief Gets the factory manager that manages this factory.
         * @return Smart pointer to the factory manager.
         */
        virtual IFactoryManager *getFactoryManagerPtr() const = 0;

        /**
         * @brief Gets the factory manager that manages this factory.
         * @return Smart pointer to the factory manager.
         */
        virtual SmartPtr<IFactoryManager> getFactoryManager() const = 0;

        /**
         * @brief Sets the factory manager that manages this factory.
         * @param factoryManager Smart pointer to the factory manager.
         */
        virtual void setFactoryManager( SmartPtr<IFactoryManager> factoryManager ) = 0;

        /**
         * @brief Creates an instance of the given type as a smart pointer.
         * @tparam T The type to create.
         * @return Smart pointer to the created instance.
         */
        template <class T>
        SmartPtr<T> make_ptr();

        /**
         * @brief Creates an instance of the given type with constructor arguments as a smart pointer.
         * @tparam T The type to create.
         * @tparam _Types The argument types.
         * @param _Args The arguments to forward to the constructor.
         * @return Smart pointer to the created instance.
         */
        template <class T, class... _Types>
        SmartPtr<T> make_ptr( _Types &&..._Args );

        /**
         * @brief Creates a shared pointer instance of the given type.
         * @tparam T The type to create.
         * @return Shared pointer to the created instance.
         */
        template <class T>
        SharedPtr<T> make_shared();

        /**
         * @brief Creates an instance of the given type as a smart pointer (alternative method).
         * @tparam T The type to create.
         * @return Smart pointer to the created instance.
         */
        template <class T>
        SmartPtr<T> make_object();

        /**
         * @brief Checks if the object is derived from the given type.
         * @tparam T The type to check.
         * @return True if the object is derived from the given type, false otherwise.
         */
        template <class T>
        bool isObjectDerivedFrom();

        WP_CLASS_REGISTER_DECL;
    };

    template <class T>
    bool IFactory::isObjectDerivedFrom()
    {
        auto typeInfo = T::typeInfo();
        return isObjectDerivedFromByInfo( typeInfo );
    }

    template <class T>
    SmartPtr<T> IFactory::make_ptr()
    {
        auto ptr = static_cast<T *>( createObject() );
        SmartPtr<T> p( ptr );

#if WP_GARBAGE_COLLECTION == 1
        ptr->removeReference();
#endif

        return p;
    }

    template <class T, class... _Types>
    SmartPtr<T> IFactory::make_ptr( _Types &&..._Args )
    {
        auto ptr = static_cast<T *>( allocateMemory() );
        if( !ptr )
        {
            WP_EXCEPTION( "Unable to allocate." );
        }

        T *object = nullptr;
        try
        {
            object = new( ptr ) T( std::forward<_Types>( _Args )... );
        }
        catch( ... )
        {
            freeMemory( ptr );
            throw;
        }

        object->setSharedObjectListener( getListener() );
        object->setPoolElement( false );
        object->setCreatorData( this );

        WP_ASSERT( object->getSharedObjectListener() == getListener() );
        WP_ASSERT( !object->isPoolElement() );
        WP_ASSERT( object->getCreatorData() == this );

        SmartPtr<T> p( object );

#if WP_GARBAGE_COLLECTION == 1
        object->removeReference();
#endif

        return p;
    }

    template <class T>
    SharedPtr<T> IFactory::make_shared()
    {
        return workphone::make_shared<T>();
    }

    template <class T>
    SmartPtr<T> IFactory::make_object()
    {
        auto object = static_cast<T *>( createObject() );
        WP_ASSERT( object );

        if( !object )
        {
            WP_EXCEPTION( "Unable to allocate." );
        }

        SmartPtr<T> p( object );

#if WP_GARBAGE_COLLECTION == 1
        object->removeReference();
#endif

        return p;
    }

}  // namespace workphone

#endif
