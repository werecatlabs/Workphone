#ifndef DisableScreenSave_h__
#define DisableScreenSave_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * @brief Disable screen saver
     *
     * @note
     *  This class is used to disable screen saver.
     *  It is used in the following way:
     *  @code
     *  CDisableScreenSave disableScreenSave;
     *  // do something
     *  @endcode
     *  When the object is destroyed, the screen saver is enabled again.
     */
    class WPCore_API DisableScreenSave : public ISharedObject
    {
    public:
        /**
         * @brief Constructor
         */
        DisableScreenSave();

        /**
         * @brief Destructor
         */
        ~DisableScreenSave() override;

        WP_CLASS_REGISTER_DECL;

    protected:
        s32 *m_pValue = nullptr;
    };

}  // namespace workphone

#endif  // DisableScreenSave_h__
