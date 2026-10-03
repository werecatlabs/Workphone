#ifndef EditCommandsUtil_h__
#define EditCommandsUtil_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/Core/Properties.hpp>

namespace workphone::editor
{
    /**
     * @brief Prepares serialized actor data for duplication/pasting.
     *
     * Clears UUIDs so newly instantiated actors get fresh identifiers and appends
     * " (Copy)" to actor names to distinguish them from the originals. The operation
     * is applied recursively to all nested child actor data.
     */
    void prepareActorDataForDuplicate( SmartPtr<Properties> actorData );
}  // namespace workphone::editor

#endif  // EditCommandsUtil_h__
