#ifndef WaitForSeconds_h__
#define WaitForSeconds_h__

#include <Workphone/Interface/System/ICoroutineData.hpp>

namespace workphone
{

    /** Represents a wait for a specified duration. */
    class WPCore_API WaitForSeconds : public ISharedObject
    {
    public:
        /** Constructor.
         * @param seconds The duration to wait.
         */
        explicit WaitForSeconds( ICoroutineData::PullType &yield, f64 seconds );

        /** Destructor. */
        ~WaitForSeconds() override;

        /** Get the duration to wait. */
        f64 getDuration() const;

        WP_CLASS_REGISTER_DECL;

    private:
        f64 m_duration = 0.0;  // Duration to wait
    };
}  // namespace workphone

#endif  // WaitForSeconds_h__
