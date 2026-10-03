#ifndef IScriptGenerator_h__
#define IScriptGenerator_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /** Script generator interface. */
    class WPCore_API IScriptGenerator : public ISharedObject
    {
    public:
        /** Destructor. */
        ~IScriptGenerator() override;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IScriptGenerator_h__
