#ifndef StateData_h__
#define StateData_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Thread/RecursiveSpinMutex.hpp>

namespace workphone
{

    /** A class for state data. */
    class WPCore_API StateData : public ISharedObject
    {
    public:
        /** Constructor. */
        StateData();

        StateData( u32 poolTypeId );

        /** Copy constructor. */
        StateData( const StateData &other );

        /** Destructor. */
        ~StateData() override;

        /** @copydoc ISharedObject::lock */
        void lock() override;

        /** @copydoc ISharedObject::unlock */
        void unlock() override;

        /** @copydoc ISharedObject::lock_shared */
        void lock_shared() override;

        /** @copydoc ISharedObject::unlock_shared */
        void unlock_shared() override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /// The state mutex.
        mutable RecursiveSpinMutex m_mutex;
    };

}  // namespace workphone

#endif  // StateData_h__
