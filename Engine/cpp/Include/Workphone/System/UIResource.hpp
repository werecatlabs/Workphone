#pragma once
#include <Workphone/Interface/Resource/IResourceCompiler.hpp>
#include <Workphone/System/CompiledResource.hpp>
#include <Workphone/Interface/UI/IUIManager.hpp>
#include <Workphone/Interface/UI/IUIElement.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Graphics/IFont.hpp>
#include <thread>

namespace workphone { class StreamBudgetLease;class Properties; }
namespace workphone::resource {class ResourcePayloadMount;}

namespace workphone::ui
{
    struct UIResourceInstallOptions
    {
        std::shared_ptr<resource::ResourcePayloadMount> mount;
        String target="pc",locale;
        bool packagedBuild=true;
    };
    /** Bounded typed window/container/text/button/image hierarchy with captured
     * font, texture, style and localization dependencies. */
    class WPCore_API UIResourceCompiler final : public resource::IResourceCompiler
    {
    public:
        String name() const override;
        Array<resource::CompilerOutput> outputs() const override;
        bool getDependencies(const resource::CompileContext &,resource::DependencySet &,String &) const override;
        resource::CompilationStatus compile(const resource::CompileContext &,std::ostream &,Array<String> &) const override;
        /** Validates the authored schema and returns only durable UUID reference
         * metadata for the existing catalog graph; never imports or loads assets. */
        static SmartPtr<Properties> sourceReferenceProperties(const Array<u8> &,String &error);
    };

    /** Created, published and destroyed on the UI owner thread before manager
     * unload. Holds the pinned
     * resource and manager registrations; destruction detaches/removes its tree.
     * Staging creates hidden existing backend controls, never a parallel UI tree.
     * root()/child references are inspection handles; callers must release them
     * before instance destruction and manager unload. Destruction removes their
     * registrations and unloads the controls. A retained SourceCpu participation
     * estimate covers this instance's controls/labels/registration metadata;
     * backend physical storage remains owned by existing backend allocators. */
    class WPCore_API UIResourceInstance final
    {
    public:
        ~UIResourceInstance();
        UIResourceInstance(const UIResourceInstance &)=delete;
        UIResourceInstance &operator=(const UIResourceInstance &)=delete;
        SmartPtr<IUIElement> root() const;
        /** Parent must be a registered control in the captured, loaded manager.
         * Foreign/unregistered parents and unloaded managers are rejected.
         * All staged controls must still be registered. Visible external windows
         * cannot share a staged window label (experimental native-ID restriction).
         * Callers still fence project/request generations before publication. */
        bool publish(SmartPtr<IUIElement> parent,String &error,
            const UIResourceInstance *replacing=nullptr);
        bool setActive(bool visible,bool enabled,String &error);
        /** Replaces the complete captured tree transactionally without loose dictionary IO. */
        bool setLocale(const String &locale,String &error);
    private:
        friend WPCore_API std::shared_ptr<UIResourceInstance> stageUIResource(
            SmartPtr<IUIManager>,std::shared_ptr<const resource::RuntimeResource>,String &,const UIResourceInstallOptions &);
        UIResourceInstance()=default;
        std::shared_ptr<StreamBudgetLease> m_metadataBudget;
        SmartPtr<IUIManager> m_manager;
        Array<SmartPtr<render::IFont>> m_fonts;
        Array<SmartPtr<render::ITexture>> m_textures;
        Array<SmartPtr<IUIElement>> m_elements;
        Array<u8> m_visible,m_enabled;
        std::shared_ptr<const resource::RuntimeResource> m_resource;
        UIResourceInstallOptions m_options;
        String m_locale;
        std::thread::id m_owner;
        bool m_published=false;
        bool m_activeVisible=true,m_activeEnabled=true;
    };

    /** Validates the entire sealed payload before creating hidden manager-owned
     * controls. Backend type absence and malformed data leave the parent intact. */
    WPCore_API std::shared_ptr<UIResourceInstance> stageUIResource(
        SmartPtr<IUIManager>,std::shared_ptr<const resource::RuntimeResource>,String &error,
        const UIResourceInstallOptions &options={});
}
