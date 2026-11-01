#pragma once
#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <Workphone/Interface/Graphics/IFont.hpp>
#include <memory>
struct ImFont;
namespace workphone::ui
{
    class WPImGui_API ImGuiRuntimeFont final
    {
    public:
        static std::shared_ptr<ImGuiRuntimeFont> create(SmartPtr<render::IFont>,String &error);
        ~ImGuiRuntimeFont();
        bool bind() const;
        ImFont *native() const;
        SmartPtr<render::IFont> source() const;
    private:
        struct Data;
        explicit ImGuiRuntimeFont(std::unique_ptr<Data>);
        std::unique_ptr<Data> m_data;
    };
    /** Per-context pins survive widget removal/retarget until the next frame or
     * context shutdown. They retain actual old texture/font snapshots. */
    WPImGui_API void retainImGuiFrameResource(SmartPtr<ISharedObject>,std::shared_ptr<const void> snapshot={});
    class WPImGui_API ImGuiRuntimeFontScope
    {
    public:
        explicit ImGuiRuntimeFontScope(const std::shared_ptr<ImGuiRuntimeFont> &,f32 pixelSize=0);
        ~ImGuiRuntimeFontScope();
        bool valid() const{return m_valid;}
    private:
        bool m_pushed=false,m_valid=true;
        f32 m_previousScale=1;
        f32 m_previousWhiteU=0,m_previousWhiteV=0;
        unsigned m_previousDrawFlags=0;
        std::shared_ptr<ImGuiRuntimeFont> m_owner;
    };
}
