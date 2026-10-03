#ifndef _IPrintMessage_H
#define _IPrintMessage_H

#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    struct IPrintMessage
    {
        virtual void PrintMessage( String text ) = 0;
    };
}  // namespace workphone

#endif  // _IPrintMessage_H
