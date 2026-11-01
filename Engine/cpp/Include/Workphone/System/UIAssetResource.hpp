#pragma once
#include <Workphone/Interface/Resource/IResourceCompiler.hpp>
#include <Workphone/System/CompiledResource.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <map>
namespace workphone {class StreamBudgetLease;}

namespace workphone::ui
{
    struct UIStyleRecord {std::shared_ptr<StreamBudgetLease> admission;String name;ColourF colour=ColourF::White;f32 textSize=16;};
    struct UILocalizationData
    {
        std::shared_ptr<StreamBudgetLease> admission;
        String fallback;
        std::map<String,std::map<String,String>> locales;
    };
    /** Bounded immutable style and locale data used by the existing UI controls. */
    class WPCore_API UIAssetResourceCompiler final : public resource::IResourceCompiler
    {
    public:
        String name() const override;
        Array<resource::CompilerOutput> outputs() const override;
        resource::CompilationStatus compile(const resource::CompileContext &,std::ostream &,Array<String> &) const override;
    };
    WPCore_API bool decodeUIStyles(const resource::RuntimeResource &,Array<UIStyleRecord> &,String &error);
    WPCore_API bool decodeUILocalization(const resource::RuntimeResource &,UILocalizationData &,String &error);
    WPCore_API bool resolveUILocalization(const UILocalizationData &,const String &locale,const String &key,String &text,String &error);
}
