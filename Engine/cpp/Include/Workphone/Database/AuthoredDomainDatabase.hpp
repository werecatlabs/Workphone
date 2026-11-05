#pragma once

#include <Workphone/Interface/Database/IDatabase.hpp>
#include <Workphone/Memory/SmartPtr.hpp>

namespace workphone
{
    /** Delete a numeric configured actor, its descendants and their owned attributes.
     * The caller supplies the existing writable standalone domain connection. */
    WPCore_API bool removeConfiguredActorSubtree( SmartPtr<IDatabase> database, s64 nodeId,
                                                   String &error );
}
