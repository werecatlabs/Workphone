#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IFont.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IFont, IResource );

    IFont::IFont( u32 poolTypeId ) : IResource( poolTypeId )
    {
    }

    IFont::IFont() : IResource( IFont::typeInfo() )
    {
    }

    const String IFont::fontTypeStr = String( "font_type" );
    const String IFont::fontSourceStr = String( "font_source" );
    const String IFont::fontSizeStr = String( "font_size" );
    const String IFont::fontResolutionStr = String( "font_resolution" );

    IFont::~IFont() = default;
    bool IFont::getRuntimeMetrics(BakedFontInfo &,u32 &) const{return false;}
    u32 IFont::getRuntimeGlyphCount() const{return 0;}
    bool IFont::getRuntimeGlyphAt(u32,FontGlyphInfo &) const{return false;}
    SmartPtr<ITexture> IFont::getRuntimeAtlas() const{return nullptr;}
    std::shared_ptr<const void> IFont::retainRuntimeFont() const{return {};}
    std::shared_ptr<StreamBudgetLease> IFont::reserveRuntimeFontStorage(u64,String &error) const
    {error="Captured bitmap-font storage is unavailable";return {};}
    bool IFont::getRuntimeWhitePixelUV(Vector2F &) const{return false;}

}  // namespace workphone::render
