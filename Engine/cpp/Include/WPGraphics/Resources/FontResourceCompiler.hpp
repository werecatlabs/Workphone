#pragma once
#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/Resource/IResourceCompiler.hpp>
#include <Workphone/System/CompiledResource.hpp>
#include <Workphone/Graphics/Font.hpp>

namespace workphone {class StreamBudgetLease;}
namespace workphone::render
{
    struct CookedFontData
    {
        std::shared_ptr<StreamBudgetLease> admission;
        resource::ResourceID atlas;
        u64 atlasSourceHash=0,atlasPayloadHash=0,atlasCompilerVersion=0;
        u32 atlasWidth=0,atlasHeight=0,whiteX=0,whiteY=0,fallback=0;
        f32 height=0,ascent=0,descent=0;
        String target;
        bool packagedBuild=false;
        Array<FontGlyphInfo> glyphs;
    };
    /** Explicit bounded baked bitmap-font recipe. No TTF parsing or host font
     * lookup. The ResourceSystem cooks and pins the one .texres atlas first. */
    class WPGraphics_API FontResourceCompiler final : public resource::IResourceCompiler
    {
    public:
        String name() const override;
        Array<resource::CompilerOutput> outputs() const override;
        bool getDependencies(const resource::CompileContext &,resource::DependencySet &,String &) const override;
        resource::CompilationStatus compile(const resource::CompileContext &,std::ostream &,Array<String> &) const override;
    };
    WPGraphics_API bool decodeCookedFont(const resource::RuntimeResource &,CookedFontData &,String &error);
    WPGraphics_API bool validateCookedFontData(const CookedFontData &,String &error);
}
