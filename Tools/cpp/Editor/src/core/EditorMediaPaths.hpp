#pragma once

#include <Workphone/Core/Path.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>

namespace workphone::editor
{
    // Editor media belongs to the installation, independently of the selected
    // project. Resolve it before handing it to project-relative filesystem APIs.
    inline String mountEditorMedia( SmartPtr<IFileSystem> fileSystem,
                                    const String &startupDirectory,
                                    const String &configuredMediaPath )
    {
        const auto mediaRoot = Path::getAbsolutePath( startupDirectory, configuredMediaPath );
        fileSystem->addFolder( mediaRoot + "/Scripts", true );
        fileSystem->addFolder( mediaRoot, false );
        return mediaRoot;
    }
}
