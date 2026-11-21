#ifndef WP_EDITOR_MCP_AUTHORING_H
#define WP_EDITOR_MCP_AUTHORING_H

#include <boost/json/object.hpp>
#include <filesystem>
#include <string>

namespace workphone::editor
{
    // Called on the Editor's owner thread between iterations.
    boost::json::object mcpAuthor( const std::string &name, const boost::json::object &args,
                                   const std::filesystem::path &workspace );
}  // namespace workphone::editor

#endif
