#ifndef WP_EDITOR_MCP_PLAYTEST_H
#define WP_EDITOR_MCP_PLAYTEST_H

#include <EditorPrerequisites.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <boost/json/object.hpp>
#include <string>

namespace workphone::editor
{
    class EditorApplication;

    SmartPtr<IInputDeviceManager> createMcpInputDevice( SmartPtr<Properties> settings );

    boost::json::object mcpPlaytest( const std::string &name, const boost::json::object &args,
                                     EditorApplication &app );
}

#endif
