#ifndef IConsole_h__
#define IConsole_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /** Interface for an interactive console. */
    class IConsole : public ISharedObject
    {
    public:
        /** Virtual destructor. */
        ~IConsole() override = default;
    };
}  // namespace workphone

#endif  // IConsole_h__
