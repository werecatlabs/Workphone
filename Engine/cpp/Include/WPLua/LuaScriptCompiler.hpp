#ifndef WPLuaScriptCompiler_h__
#define WPLuaScriptCompiler_h__

#include <WPLua/WPLuaPrerequisites.hpp>
#include <Workphone/Interface/Resource/IResourceCompiler.hpp>

namespace workphone
{
    /// Compiles syntax-checked, portable Lua source. No VM or live instance is persisted.
    /// Optional file.lua.deps lists data:// resource IDs, one per line; # starts a comment.
    class WPLua_API LuaScriptCompiler final : public resource::IResourceCompiler
    {
    public:
        static constexpr u64 version = 1;
        static constexpr size_t maxSourceBytes = 8 * 1024 * 1024;
        String name() const override;
        Array<resource::CompilerOutput> outputs() const override;
        bool getDependencies( const resource::CompileContext &context,
                              resource::DependencySet &dependencies, String &error ) const override;
        resource::CompilationStatus compile( const resource::CompileContext &context,
                                              std::ostream &output,
                                              Array<String> &messages ) const override;
    };
}
#endif
