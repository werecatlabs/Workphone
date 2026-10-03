#ifndef PointerUtil_h__
#define PointerUtil_h__

#include <Workphone/WorkphoneConfig.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Memory/RawPtr.hpp>
#include <Workphone/Memory/SafePtr.hpp>
#include <Workphone/Memory/SharedPtr.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <exception>
#include <iostream>
#include <utility>

namespace workphone
{

    /**
     * @brief Creates a SmartPtr for a new instance of T using the default constructor.
     *
     * @tparam T The type of the object to create.
     * @return SmartPtr<T> A smart pointer managing the newly created object.
     */
    template <class T>
    SmartPtr<T> make_ptr()
    {
        auto ptr = WP_NEW T();
        auto p = SmartPtr<T>( ptr );

#if WP_GARBAGE_COLLECTION == 1
        ptr->removeReference();
#endif

        return p;
    }

    /**
     * @brief Creates a SmartPtr for a new instance of T, forwarding arguments to the constructor.
     *
     * @tparam T The type of the object to create.
     * @tparam _Types Variadic types for constructor arguments.
     * @param _Args Arguments to be forwarded to the T constructor.
     * @return SmartPtr<T> A smart pointer managing the newly created object.
     */
    template <class T, class... _Types>
    SmartPtr<T> make_ptr( _Types &&..._Args )
    {
        auto ptr = WP_NEW T( std::forward<_Types>( _Args )... );
        auto p = SmartPtr<T>( ptr );

#if WP_GARBAGE_COLLECTION == 1
        ptr->removeReference();
#endif

        return p;
    }

    /**
     * @brief Creates a SharedPtr for a new instance of T using the default constructor.
     *
     * @tparam T The type of the object to create.
     * @return SharedPtr<T> A shared pointer managing the newly created object.
     */
    template <class T>
    SharedPtr<T> make_shared()
    {
        return SharedPtr<T>( WP_NEW T() );
    }

    /**
     * @brief Creates a SharedPtr for a new instance of T, forwarding arguments to the constructor.
     *
     * @tparam T The type of the object to create.
     * @tparam _Types Variadic types for constructor arguments.
     * @param _Args Arguments to be forwarded to the T constructor.
     * @return SharedPtr<T> A shared pointer managing the newly created object.
     */
    template <class T, class... _Types>
    SharedPtr<T> make_shared( _Types &&..._Args )
    {
        return SharedPtr<T>( new T( std::forward<_Types>( _Args )... ) );
    }

    /**
     * @brief Creates a SharedPtr to an Array of T, optionally initializing it with data from another
     * array.
     *
     * @tparam T The type of elements in the array.
     * @param size The desired size of the new array.
     * @param data An optional SharedPtr to an existing array to copy data from.
     * @param defaultValue The value to use for elements that are not copied from the data array.
     * @return SharedPtr<Array<T>> A shared pointer to the newly created array.
     */
    template <class T>
    SharedPtr<Array<T>> make_shared_array( size_t size, SharedPtr<Array<T>> data, const T &defaultValue )
    {
        auto p = workphone::make_shared<Array<T>>();
        auto &newArray = *p;
        newArray.resize( size, defaultValue );

        if( data )
        {
            auto &dataArray = *data;

            for( size_t i = 0; i < size && i < dataArray.size(); ++i )
            {
                newArray[i] = dataArray[i];
            }
        }

        return p;
    }

    /**
     * @brief Performs a reinterpret_cast of a SmartPtr from one type to another.
     * @warning This is a dangerous operation and should be used with caution.
     *
     * @tparam T Target type.
     * @tparam B Base/Source type.
     * @param obj The smart pointer to cast.
     * @return SmartPtr<T>& Reference to the casted smart pointer.
     */
    template <class T, class B>
    SmartPtr<T> &reinterpret_pointer_cast( SmartPtr<B> &obj )
    {
        return *reinterpret_cast<SmartPtr<T> *>( &obj );
    }

    /**
     * @brief Performs a reinterpret_cast of a const SmartPtr from one type to another.
     * @warning This is a dangerous operation and should be used with caution.
     *
     * @tparam T Target type.
     * @tparam B Base/Source type.
     * @param obj The const smart pointer to cast.
     * @return const SmartPtr<T>& Reference to the casted const smart pointer.
     */
    template <class T, class B>
    const SmartPtr<T> &reinterpret_pointer_cast( const SmartPtr<B> &obj )
    {
        return *reinterpret_cast<const SmartPtr<T> *>( &obj );
    }

    /**
     * @brief Performs a static cast of a SmartPtr to a derived type.
     *
     * @tparam T Target type.
     * @tparam B Base type.
     * @param obj The smart pointer to cast.
     * @return SmartPtr<T> A new smart pointer of the target type.
     */
    template <class T, class B>
    SmartPtr<T> static_pointer_cast( const SmartPtr<B> &obj )
    {
        auto ptr = (T *)obj.get();
        return SmartPtr<T>( ptr );
    }

    /**
     * @brief Casts the underlying raw pointer of a SmartPtr to a specific type.
     *
     * @tparam T Target type.
     * @tparam B Base type.
     * @param obj The smart pointer to cast.
     * @return T* The casted raw pointer.
     */
    template <class T, class B>
    T *static_ptr_cast( const SmartPtr<B> &obj )
    {
        return (T *)obj.get();
    }

    /**
     * @brief Performs a static cast from a SafePtr to a SmartPtr.
     *
     * @tparam T Target type.
     * @tparam B Base type.
     * @param obj The safe pointer to cast.
     * @return SmartPtr<T> A new smart pointer of the target type.
     */
    template <class T, class B>
    SmartPtr<T> static_pointer_cast( SafePtr<B> obj )
    {
        auto ptr = static_cast<T *>( obj.get() );
        return SmartPtr<T>( ptr );
    }

    /**
     * @brief Casts a SmartPtr to a SafePtr of the target type.
     *
     * @tparam T Target type.
     * @tparam B Base type.
     * @param obj The smart pointer to cast.
     * @return SafePtr<T> A new safe pointer of the target type.
     */
    template <class T, class B>
    SafePtr<T> safe_pointer_cast( SmartPtr<B> obj )
    {
        auto ptr = static_cast<T *>( obj.get() );
        return SafePtr<T>( ptr );
    }

    /**
     * @brief Performs a dynamic cast of a SmartPtr to a derived type.
     *
     * @tparam T Target type.
     * @tparam B Base type.
     * @param obj The smart pointer to cast.
     * @return SmartPtr<T> A smart pointer of the target type if the cast is successful, otherwise an
     * empty SmartPtr.
     */
    template <class T, class B>
    SmartPtr<T> dynamic_pointer_cast( SmartPtr<B> obj )
    {
        try
        {
            auto ptr = dynamic_cast<T *>( obj.get() );
            if( ptr )
            {
                return SmartPtr<T>( ptr );
            }
        }
        catch( std::exception &e )
        {
            std::cout << e.what() << std::endl;
        }

        return SmartPtr<T>();
    }

    /**
     * @brief Performs a dynamic cast of a RawPtr to a SmartPtr.
     *
     * @tparam T Target type.
     * @tparam B Base type.
     * @param obj The raw pointer to cast.
     * @return SmartPtr<T> A smart pointer of the target type if the cast is successful, otherwise an
     * empty SmartPtr.
     */
    template <class T, class B>
    SmartPtr<T> dynamic_pointer_cast( RawPtr<B> &obj )
    {
        try
        {
            auto ptr = dynamic_cast<T *>( obj.get() );
            if( ptr )
            {
                return SmartPtr<T>( ptr );
            }
        }
        catch( std::exception &e )
        {
            std::cout << e.what() << std::endl;
        }

        return SmartPtr<T>();
    }

    /**
     * @brief Converts a SmartPtr to a RawPtr.
     *
     * @tparam T The type of the pointer.
     * @param other The smart pointer to convert.
     * @return RawPtr<T> A raw pointer pointing to the same object.
     */
    template <class T>
    RawPtr<T> ptr_to_raw( SmartPtr<T> &other )
    {
        return RawPtr<T>( other.get() );
    }

    /**
     * @brief Converts a RawPtr to a SmartPtr.
     *
     * @tparam T The type of the pointer.
     * @param other The raw pointer to convert.
     * @return SmartPtr<T> A smart pointer managing the object.
     */
    template <class T>
    SmartPtr<T> ptr_to_smart( RawPtr<T> &other )
    {
        return SmartPtr<T>( other.get() );
    }

    /**
     * @brief Safely deletes the object pointed to by a RawPtr and sets the pointer to nullptr.
     *
     * @tparam T The type of the pointer.
     * @param ptr The raw pointer to delete.
     */
    template <class T>
    void WP_SAFE_DELETE( RawPtr<T> &ptr )
    {
        T *p = ptr.get();
        if( p )
        {
            delete p;
            p = nullptr;
        }

        ptr = nullptr;
    }

    /**
     * @brief Safely deletes the array pointed to by a RawPtr and sets the pointer to nullptr.
     *
     * @tparam T The type of the pointer.
     * @param ptr The raw pointer to delete.
     */
    template <class T>
    void WP_SAFE_DELETE_ARRAY( RawPtr<T> &ptr )
    {
        T *p = ptr.get();
        if( p )
        {
            delete[] p;
            p = nullptr;
        }

        ptr = nullptr;
    }

}  // namespace workphone

#endif  // PointerUtil_h__
