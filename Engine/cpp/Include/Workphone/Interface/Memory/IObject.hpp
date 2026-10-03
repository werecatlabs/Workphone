#ifndef __WP_IObject_h__
#define __WP_IObject_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/System/RttiClass.hpp>
#include <Workphone/Memory/AtomicRawPtr.hpp>
#include <Workphone/Memory/BaseObjectData.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Core/Handle.hpp>

namespace workphone
{

    /**
     * An interface for an object that can be managed by the system.
     * This interface is a base class for all managed objects in the system.
     */
    class WPCore_API IObject
    {
    public:
        /**
         * Default constructor for the class.
         */
        IObject();

        IObject( u32 typeId );

        /**
         * Virtual destructor for the class, to ensure that destructors of derived classes are called
         * correctly. The destructor is marked as `override` to ensure that it overrides the base class
         * destructor.
         */
        virtual ~IObject();

        /**
         * Called before the object is updated.
         * Implementations of this function can perform any necessary pre-update processing.
         */
        virtual void preUpdate();

        /**
         * Called to update the object.
         * Implementations of this function can perform any necessary updating of the object.
         */
        virtual void update();

        /**
         * Called after the object is updated.
         * Implementations of this function can perform any necessary post-update processing.
         */
        virtual void postUpdate();

        /** Gets the object name. */
        const c8 *getNamePtr() const;

        /** Gets the object id. */
        hash_type getId() const;

        /** Sets the object id. */
        void setId( hash_type id );

        /** Gets the object name. */
        virtual String getName() const;

        /** Sets the object name. */
        virtual void setName( const String &name );

        /**
         * @brief Sets the object flags.
         * @param flags The flags to set.
         */
        void setObjectFlags( u8 flags );

        /**
         * @brief Sets the object flag.
         * @param flag The flag to set.
         * @param value The value to set the flag to.
         */
        void setObjectFlag( u8 flag, bool value );

        /**
         * @brief Gets the object flags.
         * @return The object flags.
         */
        bool getObjectFlag( u8 flag ) const;

        /**
         * Returns a pointer to the `Handle` object associated with this object.
         * @return A pointer to the `Handle` object.
         */
        Handle *getHandle();

        /**
         * Returns a pointer to the `Handle` object associated with this object.
         * @return A pointer to the `Handle` object.
         */
        const Handle *getHandle() const;

        /**
         * Checks if the object is valid.
         * @return Returns a boolean indicating if the object is valid.
         */
        virtual bool isValid() const;

        /**
         * Gets the creator data associated with this object.
         * @return A pointer to the creator data.
         */
        virtual void *getCreatorData() const;

        /**
         * Sets the creator data associated with this object.
         * @param data A pointer to the creator data.
         */
        virtual void setCreatorData( void *data );

        /**
         * Gets the factory data associated with this object.
         * @return An integer representing the factory data.
         */
        virtual hash_type getFactoryData() const;

        /**
         * Sets the factory data associated with this object.
         * @param data An integer representing the factory data.
         */
        virtual void setFactoryData( hash_type data );

        /**
         * Gets the object data as a string.
         * @return The object data as a `String` object.
         */
        virtual String toString() const;

        /**
         * Gets the user data associated with this object.
         * @return A pointer to the user data.
         */
        virtual void *getUserData() const;

        /**
         * Sets the user data associated with this object.
         * @param data A pointer to the user data.
         */
        virtual void setUserData( void *data );

        /**
         * Gets the user data attached to this object with the specified ID.
         * @param id The ID of the user data.
         * @return A pointer to the user data.
         */
        virtual void *getUserData( hash_type id ) const;

        /**
         * Sets the user data attached to this object with the specified ID.
         * @param id The ID of the user data.
         * @param userData A pointer to the user data.
         */
        virtual void setUserData( hash_type id, void *userData );

        /**
         * @brief Check if the current object is derived from a specified type.
         *
         * This function checks if the current object is derived from the specified type.
         * It uses the TypeManager to retrieve the type information of the current object
         * and then calls the TypeManager's isDerived() function to perform the derivation check.
         *
         * @param type The type ID to check against.
         * @return True if the current object is derived from the specified type, false otherwise.
         */
        virtual bool derived( u32 type ) const;

        /**
         * @brief Check if the current object is of exactly the specified type.
         *
         * This function checks if the current object is of exactly the specified type.
         * It uses the TypeManager to retrieve the type information of the current object
         * and then calls the TypeManager's isExactly() function to perform the type check.
         *
         * @param type The type ID to check against.
         * @return True if the current object is of exactly the specified type, false otherwise.
         */
        virtual bool exactly( u32 type ) const;

        /**
         * Checks if the class represented by this `IObject` instance is derived from a specified class
         * `B`. This function uses type information to perform the check.
         * @return Returns `true` if the class is derived from `B`, `false` otherwise.
         */
#if WP_CPP_STANDARD >= WP_CPP_2020
        template <class B>
            requires requires { B::typeInfo(); }
        bool isDerived() const;
#else
        template <class B>
        bool isDerived() const;
#endif

        /**
         * Checks if the class represented by this `IObject` instance is exactly of type `B`.
         * This function uses type information to perform the check.
         * @return Returns `true` if the class is of type `B`, `false` otherwise.
         */
#if WP_CPP_STANDARD >= WP_CPP_2020
        template <class B>
            requires requires { B::typeInfo(); }
        bool isExactly() const;
#else
        template <class B>
        bool isExactly() const;
#endif

#if WP_USE_CUSTOM_NEW_DELETE
        void *operator new( size_Num sz );
        void *operator new( size_Num sz, void *ptr );
        void *operator new[]( size_Num sz );
        void operator delete( void *ptr );
        void operator delete( void *ptr, void * );
        void operator delete[]( void *ptr );

        void *operator new( size_Num sz, const c8 *file, s32 line, const c8 *func );
        void *operator new( size_Num sz, void *ptr, const c8 *file, s32 line, const c8 *func );
        void *operator new( size_Num sz, std::align_val_t alignment, s32 line, const c8 *file,
                            s32 func );
        void *operator new[]( size_Num sz, const c8 *file, s32 line, const c8 *func );
        void operator delete( void *reportedAddress, const c8 *file, s32 line, const c8 *func ) throw();
        void operator delete( void *ptr, void *, const c8 *file, s32 line, const c8 *func );
        void operator delete[]( void *reportedAddress, const c8 *file, s32 line,
                                const c8 *func ) throw();
#endif

#if !WP_FINAL
        String getDebugStr() const;

        void setDebugStr( const String &debugStr );
#endif

        WP_OBJECT_CLASS_REGISTER_DECL;

    protected:
        mutable AtomicRawPtr<BaseObjectData> m_objectData;
        mutable atomic_u32 iTypeInfo = 0;

        u32 m_poolTypeId = 0;

        /// Object flags (bitfield for alive, garbage collected, pool element, etc.).
        atomic_u8 m_objectFlags = OBJECT_FLAG_ALIVE | OBJECT_FLAG_GARBAGE_COLLECTED;
    };

    WPForceInline Handle *IObject::getHandle()
    {
        return &m_objectData->m_handle;
    }

    WPForceInline const Handle *IObject::getHandle() const
    {
        return &m_objectData->m_handle;
    }

    WPForceInline bool IObject::getObjectFlag( u8 flag ) const
    {
        return ( m_objectFlags & flag ) != 0;
    }

#if WP_CPP_STANDARD >= WP_CPP_2020
    template <class B>
    requires { B::typeInfo(); } bool IObject::isDerived() const
    {
        auto typeInfo = getTypeInfo();
        if( typeInfo != 0 )
        {
            auto typeManager = TypeManager::instance();
            WP_ASSERT( typeManager );

            auto otherTypeInfo = B::typeInfo();
            return typeManager->isDerived( typeInfo, otherTypeInfo );
        }

        return false;
    }
#else
    template <class B>
    bool IObject::isDerived() const
    {
        auto typeInfo = getTypeInfo();
        if( typeInfo != 0 )
        {
            auto typeManager = TypeManager::instance();
            WP_ASSERT( typeManager );

            auto otherTypeInfo = B::typeInfo();
            return typeManager->isDerived( typeInfo, otherTypeInfo );
        }

        return false;
    }
#endif

#if WP_CPP_STANDARD >= WP_CPP_2020
    template <class B>
        requires requires { B::typeInfo(); }
    bool IObject::isExactly() const
    {
        auto typeInfo = getTypeInfo();
        if( typeInfo != 0 )
        {
            auto typeManager = TypeManager::instance();
            WP_ASSERT( typeManager );

            auto otherTypeInfo = B::typeInfo();
            return typeManager->isExactly( typeInfo, otherTypeInfo );
        }

        return false;
    }
#else
    template <class B>
    bool IObject::isExactly() const
    {
        auto typeInfo = getTypeInfo();
        if( typeInfo != 0 )
        {
            auto typeManager = TypeManager::instance();
            WP_ASSERT( typeManager );

            auto otherTypeInfo = B::typeInfo();
            return typeManager->isExactly( typeInfo, otherTypeInfo );
        }

        return false;
    }
#endif

}  // namespace workphone

#endif  // __WP_IObject_h__
