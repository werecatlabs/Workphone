#pragma once
#include <Workphone/WorkphonePrerequisites.hpp>

namespace workphone::scene
{
    /// Bake persistent, repeating island surfaces with the registered texture forge.
    /// Publishes the four material slots as one undoable terrain edit.
    WPCore_API bool createLandscapeMaterials( TerrainSystem *terrain,
                                               SmartPtr<ICommandManager> commands, String &error );
}
