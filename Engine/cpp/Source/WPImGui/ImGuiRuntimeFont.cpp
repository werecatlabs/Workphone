#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiRuntimeFont.hpp>
#include <Workphone/Graphics/Font.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cmath>
#include <cstring>

#if defined WP_PLATFORM_WIN32
#    include <d3d11.h>
#    include <wrl/client.h>
#endif

namespace workphone::ui
{
    namespace
    {
        struct FramePins
        {
            StreamBudget admission;
            Array<SmartPtr<ISharedObject>> resources=makeSourceBudgetSnapshot<SmartPtr<ISharedObject>>();
            Array<std::shared_ptr<const void>> snapshots=makeSourceBudgetSnapshot<std::shared_ptr<const void>>();
#if defined WP_PLATFORM_WIN32
            using TextureView = Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>;
            Array<TextureView> dx11Textures = makeSourceBudgetSnapshot<TextureView>();
#endif
        };
        void frameHook(ImGuiContext *,ImGuiContextHook *hook)
        {
            auto *pins=static_cast<FramePins *>(hook->UserData);
            if(hook->Type==ImGuiContextHookType_Shutdown){delete pins;return;}
            pins->resources.clear();pins->snapshots.clear();
#if defined WP_PLATFORM_WIN32
            pins->dx11Textures.clear();
#endif
        }
        FramePins *findPins( ImGuiContext &context ) noexcept
        {
            for( const auto &hook : context.Hooks )
            {
                if( hook.Type == ImGuiContextHookType_NewFramePre && hook.Callback == frameHook )
                {
                    return static_cast<FramePins *>( hook.UserData );
                }
            }
            return nullptr;
        }
        FramePins *pins()
        {
            auto *context=ImGui::GetCurrentContext();if(!context)return nullptr;
            if( auto *existing = findPins( *context ) ) return existing;
            String error;auto admission=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::SourceCpu,4096,error);if(!admission)throw std::runtime_error(error.c_str());
            auto owner=std::make_unique<FramePins>();owner->admission=std::move(admission);
            // Reserve both hook slots before publishing a userdata pointer.
            context->Hooks.reserve(context->Hooks.Size+2);
            ImGuiContextHook frame;frame.Type=ImGuiContextHookType_NewFramePre;frame.Callback=frameHook;frame.UserData=owner.get();ImGui::AddContextHook(context,&frame);
            frame.Type=ImGuiContextHookType_Shutdown;ImGui::AddContextHook(context,&frame);return owner.release();
        }
    }
    struct ImGuiRuntimeFont::Data
    {
        StreamBudget admission;
        SmartPtr<render::IFont> source;
        SmartPtr<render::ITexture> texture;
        std::shared_ptr<const void> snapshot;
        std::unique_ptr<ImFontAtlas> atlas;
        ImFont *font=nullptr;
    };
    ImGuiRuntimeFont::ImGuiRuntimeFont(std::unique_ptr<Data> data):m_data(std::move(data)){}
    ImGuiRuntimeFont::~ImGuiRuntimeFont()=default;
    std::shared_ptr<ImGuiRuntimeFont> ImGuiRuntimeFont::create(SmartPtr<render::IFont> source,String &error)
    {
        error.clear();try {
            if(!source||!source->isLoaded()){error="ImGui runtime font requires a loaded captured font";return {};}
            render::BakedFontInfo metrics;u32 fallback=0;const auto count=source->getRuntimeGlyphCount();
            auto snapshot=source->retainRuntimeFont();auto texture=source->getRuntimeAtlas();
            if(!snapshot||!texture||!texture->isLoaded()||!count||count>4096||!source->getRuntimeMetrics(metrics,fallback)||fallback<32||fallback>IM_UNICODE_CODEPOINT_MAX||!std::isfinite(metrics.height)||metrics.height<1||metrics.height>1024||!std::isfinite(metrics.ascent)||!std::isfinite(metrics.descent))
            {error="Unsupported captured ImGui bitmap-font view";return {};}
            Vector2F white;if(!source->getRuntimeWhitePixelUV(white)||!std::isfinite(white.x)||!std::isfinite(white.y)||white.x<=0||white.y<=0||white.x>=1||white.y>=1){error="Missing bitmap-font opaque white utility texel";return {};}
            u32 maximum=0;bool fallbackFound=false;const auto dimensions=texture->getSize();
            if(dimensions.x<=0||dimensions.y<=0||dimensions.x>1024||dimensions.y>1024){error="Invalid ImGui bitmap-font atlas dimensions";return {};}
            for(u32 i=0;i<count;++i){render::FontGlyphInfo glyph;if(!source->getRuntimeGlyphAt(i,glyph)||glyph.codepoint<32||glyph.codepoint>IM_UNICODE_CODEPOINT_MAX||glyph.codepoint<=maximum||(glyph.codepoint>=0xd800&&glyph.codepoint<=0xdfff)){error="This ImGui build cannot represent the font's sorted Unicode glyph range";return {};}
                for(const auto value:{glyph.xadvance,glyph.x0,glyph.y0,glyph.x1,glyph.y1,glyph.width,glyph.height,glyph.u0,glyph.v0,glyph.u1,glyph.v1})if(!std::isfinite(value)||std::abs(value)>4096){error="Invalid ImGui bitmap glyph metric";return {};}
                if(glyph.xadvance<0||glyph.x1<glyph.x0||glyph.y1<glyph.y0||glyph.width!=glyph.x1-glyph.x0||glyph.height!=glyph.y1-glyph.y0||glyph.u0<0||glyph.v0<0||glyph.u1>1||glyph.v1>1||glyph.u1<glyph.u0||glyph.v1<glyph.v0||
                    std::abs((glyph.u1-glyph.u0)*dimensions.x-glyph.width)>.01f||std::abs((glyph.v1-glyph.v0)*dimensions.y-glyph.height)>.01f){error="Invalid ImGui bitmap glyph bounds/UVs";return {};}
                if(glyph.codepoint==fallback)fallbackFound=glyph.width>0&&glyph.height>0&&glyph.xadvance>0;maximum=glyph.codepoint;}
            if(!fallbackFound){error="ImGui bitmap-font fallback glyph is absent";return {};}
            auto admission=source->reserveRuntimeFontStorage(65536+128ull*count+8ull*(maximum+1),error);if(!admission)return {};
            auto data=std::make_unique<Data>();data->admission=std::move(admission);data->source=std::move(source);data->texture=std::move(texture);data->snapshot=std::move(snapshot);data->atlas=std::make_unique<ImFontAtlas>();
            data->font=IM_NEW(ImFont)();data->atlas->Fonts.push_back(data->font);auto &font=*data->font;font.ContainerAtlas=data->atlas.get();font.FontSize=metrics.height;font.Ascent=metrics.ascent;font.Descent=metrics.descent;font.FallbackChar=ImWchar(fallback);
            const auto size=data->texture->getSize();data->atlas->TexWidth=size.x;data->atlas->TexHeight=size.y;data->atlas->TexPixelsUseColors=false;data->atlas->TexUvWhitePixel=ImVec2(white.x,white.y);
            for(u32 i=0;i<count;++i){render::FontGlyphInfo glyph;if(!data->source->getRuntimeGlyphAt(i,glyph))throw std::runtime_error("Captured font changed during ImGui adapter creation");
                font.AddGlyph(nullptr,ImWchar(glyph.codepoint),glyph.x0,glyph.y0,glyph.x1,glyph.y1,glyph.u0,glyph.v0,glyph.u1,glyph.v1,glyph.xadvance);}
            font.BuildLookupTable();return std::shared_ptr<ImGuiRuntimeFont>(new ImGuiRuntimeFont(std::move(data)));
        }catch(const std::exception &failure){try{error=failure.what();}catch(...){error.clear();}return {};}
    }
    bool ImGuiRuntimeFont::bind() const
    {
        void *view = nullptr;
        m_data->texture->getTextureFinal( &view );
        if( !view ) return false;
        retainImGuiDx11FrameTexture( view );
        m_data->atlas->SetTexID( view );
        return true;
    }
    ImFont *ImGuiRuntimeFont::native() const{return m_data->font;}
    SmartPtr<render::IFont> ImGuiRuntimeFont::source() const{return m_data->source;}
    void retainImGuiFrameResource(SmartPtr<ISharedObject> resource,std::shared_ptr<const void> snapshot)
    {if(auto *frame=pins()){if(resource)frame->resources.push_back(std::move(resource));if(snapshot)frame->snapshots.push_back(std::move(snapshot));}}

    void retainImGuiDx11FrameTexture( void *view )
    {
#if defined WP_PLATFORM_WIN32
        if( !view || !ImGui::GetCurrentContext() ) return;
        const auto *backendName = ImGui::GetIO().BackendRendererName;
        if( !backendName || std::strcmp( backendName, "imgui_impl_claw_dx11" ) != 0 ) return;

        auto &textures = pins()->dx11Textures;
        auto *native = static_cast<ID3D11ShaderResourceView *>( view );
        const auto existing = std::find_if( textures.begin(), textures.end(),
                                          [native]( const auto &texture ) { return texture.Get() == native; } );
        if( existing == textures.end() ) textures.emplace_back( native );
#endif
    }

    void releaseImGuiDx11FrameTextures() noexcept
    {
#if defined WP_PLATFORM_WIN32
        if( auto *context = ImGui::GetCurrentContext() )
        {
            if( auto *frame = findPins( *context ) ) frame->dx11Textures.clear();
        }
#endif
    }

    ImGuiRuntimeFontScope::ImGuiRuntimeFontScope(const std::shared_ptr<ImGuiRuntimeFont> &font,f32 pixelSize)
    {if(!font)return;m_valid=font->bind();if(!m_valid)return;retainImGuiFrameResource(font->source(),font);m_owner=font;m_previousScale=font->native()->Scale;
        auto *draw=ImGui::GetWindowDrawList();m_previousDrawFlags=draw->Flags;
        // PushFont/PopFont update the context's white UV from ContainerAtlas.
        // This private atlas has no baked line-UV table.
        draw->Flags&=~ImDrawListFlags_AntiAliasedLinesUseTex;
        if(std::isfinite(pixelSize)&&pixelSize>0)font->native()->Scale=pixelSize/font->native()->FontSize;ImGui::PushFont(font->native());m_pushed=true;}
    ImGuiRuntimeFontScope::~ImGuiRuntimeFontScope(){if(m_pushed){ImGui::PopFont();m_owner->native()->Scale=m_previousScale;auto *draw=ImGui::GetWindowDrawList();draw->Flags=ImDrawListFlags(m_previousDrawFlags);}}
}
