#ifndef ScriptFile_h__
#define ScriptFile_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /** ScriptFile implementation.
     */
    class WPCore_API ScriptFile : public ISharedObject
    {
    public:
        /** Constructor. */
        ScriptFile();

        /** Destructor. */
        ~ScriptFile() override;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // ScriptFile_h__
