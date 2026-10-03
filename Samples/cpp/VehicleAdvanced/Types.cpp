#include "Types.h"
#include <Workphone/Core/StringUtil.hpp>

namespace workphone
{
    const u32 Types::INPUT_UP = StringUtil::getHashMakeLower( "up" );
    const u32 Types::INPUT_DOWN = StringUtil::getHashMakeLower( "down" );
    const u32 Types::INPUT_RIGHT = StringUtil::getHashMakeLower( "right" );
    const u32 Types::INPUT_LEFT = StringUtil::getHashMakeLower( "left" );
} // namespace fb
