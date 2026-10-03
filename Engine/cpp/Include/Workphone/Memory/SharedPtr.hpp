#ifndef SharedPtr_h__
#define SharedPtr_h__

#include <Workphone/WorkphoneConfig.hpp>
#include <atomic>

namespace workphone
{

    /**
     * @brief Internal abstract control block for SharedPtr.
     *
     * Maintains the strong reference count with atomic operations. A concrete
     * derived block (SharedPtrControlBlockImpl<T>) stores the original typed pointer
     * and provides the correct type-specific deletion through destroyObject().
     *
     * Thread safety: the reference count is maintained using acquire/release memory
     * ordering on the decrement path so that all writes to the managed object are
     * visible to whichever thread ultimately destroys it.
     */
    class SharedPtrControlBlock
    {
    public:
        SharedPtrControlBlock() = default;
        virtual ~SharedPtrControlBlock() = default;

        /** @brief Deletes the managed object. Called exactly once when the strong count reaches zero. */
        virtual void destroyObject() = 0;

        /** @brief Increments the strong reference count. */
        void addRef() noexcept
        {
            m_count.fetch_add( 1, std::memory_order_relaxed );
        }

        /**
         * @brief Decrements the strong reference count.
         * @return The reference count after decrement.
         */
        long release() noexcept
        {
            return m_count.fetch_sub( 1, std::memory_order_acq_rel ) - 1;
        }

        /** @brief Returns the current strong reference count. */
        long count() const noexcept
        {
            return m_count.load( std::memory_order_relaxed );
        }

    private:
        std::atomic<long> m_count{ 1 };
    };

    /**
     * @brief Concrete control block that owns and destroys a pointer of type T.
     *
     * Stores the original T* (which may be a derived type of the SharedPtr's T)
     * so that the correct destructor is always invoked.
     *
     * @tparam T Type of the originally constructed object.
     */
    template <class T>
    class SharedPtrControlBlockImpl final : public SharedPtrControlBlock
    {
    public:
        explicit SharedPtrControlBlockImpl( T *ptr ) : m_ptr( ptr )
        {
        }

        void destroyObject() override
        {
            delete m_ptr;
        }

    private:
        T *m_ptr;
    };

    /**
     * @brief Non-intrusive reference-counted shared pointer.
     *
     * SharedPtr<T> provides shared ownership of a dynamically-allocated object without
     * requiring T to derive from any particular base class. Multiple SharedPtr instances
     * may co-own the same object; the managed object is deleted when the last owning
     * SharedPtr is destroyed or reset.
     *
     * Reference counting is performed through an internal control block
     * (SharedPtrControlBlock / SharedPtrControlBlockImpl<T>) allocated on the heap. No
     * boost or standard-library shared_ptr machinery is used; the implementation is
     * entirely self-contained.
     *
     * @tparam T Type of the managed object.
     */
    template <class T>
    class SharedPtr
    {
    public:
        using this_type = SharedPtr<T>;  ///< Type of this shared pointer.
        using element_type = T;          ///< Type of the managed object.
        using value_type = T;            ///< Value type of the managed object.
        using pointer = T *;             ///< Pointer to the managed object.

        /**
         * @brief Default-constructs an empty (null) SharedPtr.
         */
        SharedPtr() = default;

        /**
         * @brief Constructs an empty SharedPtr from nullptr.
         */
        SharedPtr( std::nullptr_t ) noexcept;

        /**
         * @brief Constructs a SharedPtr taking ownership of a raw pointer.
         *
         * A new control block is allocated. The pointed-to object is deleted when the
         * last SharedPtr owning it is destroyed or reset.
         *
         * @param ptr Raw pointer to take ownership of (may be nullptr).
         */
        explicit SharedPtr( T *ptr );

        /**
         * @brief Constructs a SharedPtr taking ownership of a compatible raw pointer.
         *
         * The control block stores the original Y* so that the correct type-specific
         * destructor is invoked even when T is a base class of Y.
         *
         * @tparam Y Type that is implicitly convertible to T (typically a derived class).
         * @param ptr Raw pointer to take ownership of.
         */
        template <class Y>
        explicit SharedPtr( Y *ptr );

        /**
         * @brief Copy-constructs from another SharedPtr<T>.
         *
         * Both shared pointers co-own the same managed object after construction.
         *
         * @param other Source shared pointer.
         */
        SharedPtr( const SharedPtr &other );

        /**
         * @brief Copy-constructs from a compatible SharedPtr<Y>.
         *
         * @tparam Y Type whose pointer is implicitly convertible to T*.
         * @param other Source shared pointer.
         */
        template <class Y>
        SharedPtr( const SharedPtr<Y> &other );

        /**
         * @brief Move-constructs from another SharedPtr<T>.
         *
         * After construction, `other` is left empty. No reference-count change is performed.
         *
         * @param other Source shared pointer to move from.
         */
        SharedPtr( SharedPtr &&other ) noexcept;

        /**
         * @brief Move-constructs from a compatible SharedPtr<Y>.
         *
         * @tparam Y Type whose pointer is implicitly convertible to T*.
         * @param other Source shared pointer to move from.
         */
        template <class Y>
        SharedPtr( SharedPtr<Y> &&other ) noexcept;

        /**
         * @brief Destructor. Releases ownership; deletes the managed object when the count reaches zero.
         */
        ~SharedPtr();

        /**
         * @brief Copy-assigns from another SharedPtr<T>.
         *
         * @param other Source shared pointer.
         * @return Reference to this shared pointer.
         */
        SharedPtr &operator=( const SharedPtr &other );

        /**
         * @brief Move-assigns from another SharedPtr<T>.
         *
         * After the call, `other` is left empty.
         *
         * @param other Source shared pointer to move from.
         * @return Reference to this shared pointer.
         */
        SharedPtr &operator=( SharedPtr &&other ) noexcept;

        /**
         * @brief Copy-assigns from a compatible SharedPtr<Y>.
         *
         * @tparam Y Type whose pointer is implicitly convertible to T*.
         * @param other Source shared pointer.
         * @return Reference to this shared pointer.
         */
        template <class Y>
        SharedPtr &operator=( const SharedPtr<Y> &other );

        /**
         * @brief Assigns nullptr, releasing the managed object.
         *
         * @return Reference to this shared pointer.
         */
        SharedPtr &operator=( std::nullptr_t ) noexcept;

        /**
         * @brief Dereferences the stored pointer.
         *
         * @return Reference to the managed object. Behaviour is undefined if the pointer is null.
         */
        T &operator*() const;

        /**
         * @brief Accesses members of the managed object.
         *
         * @return Raw pointer to the managed object.
         */
        T *operator->() const;

        /**
         * @brief Checks whether this SharedPtr is empty (null).
         *
         * @return true if the stored pointer is null, false otherwise.
         */
        bool operator!() const;

        /**
         * @brief Implicit conversion to bool.
         *
         * @return true if the stored pointer is not null, false otherwise.
         */
        explicit operator bool() const;

        /**
         * @brief Returns the raw pointer to the managed object.
         *
         * @return Pointer to the managed object, or nullptr if empty.
         */
        T *get() const;

        /**
         * @brief Returns the current strong reference count.
         *
         * @return Number of SharedPtr instances co-owning the managed object.
         */
        long use_count() const noexcept;

        /**
         * @brief Checks whether this SharedPtr is the sole owner.
         *
         * @return true if use_count() == 1, false otherwise.
         */
        bool unique() const noexcept;

        /**
         * @brief Releases ownership and resets the stored pointer to null.
         */
        void reset() noexcept;

        /**
         * @brief Releases current ownership and takes ownership of ptr.
         *
         * @tparam Y Type whose pointer is implicitly convertible to T*.
         * @param ptr Raw pointer to take ownership of.
         */
        template <class Y>
        void reset( Y *ptr );

        /**
         * @brief Swaps the contents of this SharedPtr with another.
         *
         * @param other SharedPtr to swap with.
         */
        void swap( SharedPtr &other ) noexcept;

        /** @brief Equality comparison between two SharedPtr instances. */
        bool operator==( const SharedPtr &other ) const;

        /** @brief Inequality comparison between two SharedPtr instances. */
        bool operator!=( const SharedPtr &other ) const;

        /** @brief Equality comparison with a raw pointer. */
        bool operator==( T *other ) const;

        /** @brief Inequality comparison with a raw pointer. */
        bool operator!=( T *other ) const;

        /** @brief Equality comparison with nullptr. */
        bool operator==( std::nullptr_t ) const noexcept;

        /** @brief Inequality comparison with nullptr. */
        bool operator!=( std::nullptr_t ) const noexcept;

        /**
         * @brief Less-than comparison for use in ordered containers.
         *
         * @param other SharedPtr to compare against.
         * @return true if this pointer is less than `other`.
         */
        bool operator<( const SharedPtr &other ) const;

    private:
        template <class Y>
        friend class SharedPtr;

        /** @brief Decrements the reference count; destroys the object and control block when it reaches
         * zero. */
        void releaseRef() noexcept;

        T *m_ptr = nullptr;                       ///< Raw pointer to the managed object.
        SharedPtrControlBlock *m_ctrl = nullptr;  ///< Pointer to the reference-counting control block.
    };

    // --- Out-of-class template definitions ---

    template <class T>
    SharedPtr<T>::SharedPtr( std::nullptr_t ) noexcept
    {
    }

    template <class T>
    SharedPtr<T>::SharedPtr( T *ptr ) :
        m_ptr( ptr ),
        m_ctrl( ptr ? new SharedPtrControlBlockImpl<T>( ptr ) : nullptr )
    {
    }

    template <class T>
    template <class Y>
    SharedPtr<T>::SharedPtr( Y *ptr ) :
        m_ptr( static_cast<T *>( ptr ) ),
        m_ctrl( ptr ? new SharedPtrControlBlockImpl<Y>( ptr ) : nullptr )
    {
    }

    template <class T>
    SharedPtr<T>::SharedPtr( const SharedPtr &other ) : m_ptr( other.m_ptr ), m_ctrl( other.m_ctrl )
    {
        if( m_ctrl )
        {
            m_ctrl->addRef();
        }
    }

    template <class T>
    template <class Y>
    SharedPtr<T>::SharedPtr( const SharedPtr<Y> &other ) :
        m_ptr( static_cast<T *>( other.m_ptr ) ),
        m_ctrl( other.m_ctrl )
    {
        if( m_ctrl )
        {
            m_ctrl->addRef();
        }
    }

    template <class T>
    SharedPtr<T>::SharedPtr( SharedPtr &&other ) noexcept : m_ptr( other.m_ptr ), m_ctrl( other.m_ctrl )
    {
        other.m_ptr = nullptr;
        other.m_ctrl = nullptr;
    }

    template <class T>
    template <class Y>
    SharedPtr<T>::SharedPtr( SharedPtr<Y> &&other ) noexcept :
        m_ptr( static_cast<T *>( other.m_ptr ) ),
        m_ctrl( other.m_ctrl )
    {
        other.m_ptr = nullptr;
        other.m_ctrl = nullptr;
    }

    template <class T>
    SharedPtr<T>::~SharedPtr()
    {
        releaseRef();
    }

    template <class T>
    SharedPtr<T> &SharedPtr<T>::operator=( const SharedPtr &other )
    {
        if( this != &other )
        {
            releaseRef();
            m_ptr = other.m_ptr;
            m_ctrl = other.m_ctrl;
            if( m_ctrl )
            {
                m_ctrl->addRef();
            }
        }
        return *this;
    }

    template <class T>
    SharedPtr<T> &SharedPtr<T>::operator=( SharedPtr &&other ) noexcept
    {
        if( this != &other )
        {
            releaseRef();
            m_ptr = other.m_ptr;
            m_ctrl = other.m_ctrl;
            other.m_ptr = nullptr;
            other.m_ctrl = nullptr;
        }
        return *this;
    }

    template <class T>
    template <class Y>
    SharedPtr<T> &SharedPtr<T>::operator=( const SharedPtr<Y> &other )
    {
        releaseRef();
        m_ptr = static_cast<T *>( other.m_ptr );
        m_ctrl = other.m_ctrl;
        if( m_ctrl )
        {
            m_ctrl->addRef();
        }
        return *this;
    }

    template <class T>
    SharedPtr<T> &SharedPtr<T>::operator=( std::nullptr_t ) noexcept
    {
        releaseRef();
        return *this;
    }

    template <class T>
    T &SharedPtr<T>::operator*() const
    {
        return *m_ptr;
    }

    template <class T>
    T *SharedPtr<T>::operator->() const
    {
        return m_ptr;
    }

    template <class T>
    bool SharedPtr<T>::operator!() const
    {
        return m_ptr == nullptr;
    }

    template <class T>
    SharedPtr<T>::operator bool() const
    {
        return m_ptr != nullptr;
    }

    template <class T>
    T *SharedPtr<T>::get() const
    {
        return m_ptr;
    }

    template <class T>
    long SharedPtr<T>::use_count() const noexcept
    {
        return m_ctrl ? m_ctrl->count() : 0;
    }

    template <class T>
    bool SharedPtr<T>::unique() const noexcept
    {
        return use_count() == 1;
    }

    template <class T>
    void SharedPtr<T>::reset() noexcept
    {
        releaseRef();
    }

    template <class T>
    template <class Y>
    void SharedPtr<T>::reset( Y *ptr )
    {
        releaseRef();
        if( ptr )
        {
            m_ptr = static_cast<T *>( ptr );
            m_ctrl = new SharedPtrControlBlockImpl<Y>( ptr );
        }
    }

    template <class T>
    void SharedPtr<T>::swap( SharedPtr &other ) noexcept
    {
        T *tmpPtr = m_ptr;
        SharedPtrControlBlock *tmpCtrl = m_ctrl;
        m_ptr = other.m_ptr;
        m_ctrl = other.m_ctrl;
        other.m_ptr = tmpPtr;
        other.m_ctrl = tmpCtrl;
    }

    template <class T>
    bool SharedPtr<T>::operator==( const SharedPtr &other ) const
    {
        return m_ptr == other.m_ptr;
    }

    template <class T>
    bool SharedPtr<T>::operator!=( const SharedPtr &other ) const
    {
        return m_ptr != other.m_ptr;
    }

    template <class T>
    bool SharedPtr<T>::operator==( T *other ) const
    {
        return m_ptr == other;
    }

    template <class T>
    bool SharedPtr<T>::operator!=( T *other ) const
    {
        return m_ptr != other;
    }

    template <class T>
    bool SharedPtr<T>::operator==( std::nullptr_t ) const noexcept
    {
        return m_ptr == nullptr;
    }

    template <class T>
    bool SharedPtr<T>::operator!=( std::nullptr_t ) const noexcept
    {
        return m_ptr != nullptr;
    }

    template <class T>
    bool SharedPtr<T>::operator<( const SharedPtr &other ) const
    {
        return m_ptr < other.m_ptr;
    }

    template <class T>
    void SharedPtr<T>::releaseRef() noexcept
    {
        if( m_ctrl )
        {
            if( m_ctrl->release() == 0 )
            {
                m_ctrl->destroyObject();
                delete m_ctrl;
            }
            m_ptr = nullptr;
            m_ctrl = nullptr;
        }
    }

}  // namespace workphone

#endif  // SharedPtr_h__
