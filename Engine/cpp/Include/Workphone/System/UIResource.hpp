#pragma once
#include <Workphone/Interface/Resource/IResourceCompiler.hpp>
#include <Workphone/System/CompiledResource.hpp>
#include <Workphone/Interface/UI/IUIManager.hpp>
#include <Workphone/Interface/UI/IUIElement.hpp>
#include <thread>

namespace workphone { class StreamBudgetLease; }

namespace workphone::ui
{
    /** Experimental .uires format: bounded typed window/container/text/button
     * hierarchy. Unsupported arbitrary properties, scripts, fonts and image
     * references are rejected rather than silently omitted. */
    class WPCore_API UIResourceCompiler final : public resource::IResourceCompiler
    {
    public:
        String name() const override;
        Array<resource::CompilerOutput> outputs() const override;
        resource::CompilationStatus compile(const resource::CompileContext &,std::ostream &,Array<String> &) const override;
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
        bool publish(SmartPtr<IUIElement> parent,String &error);
    private:
        friend WPCore_API std::shared_ptr<UIResourceInstance> stageUIResource(
            SmartPtr<IUIManager>,std::shared_ptr<const resource::RuntimeResource>,String &);
        UIResourceInstance()=default;
        std::shared_ptr<StreamBudgetLease> m_metadataBudget;
        SmartPtr<IUIManager> m_manager;
        Array<SmartPtr<IUIElement>> m_elements;
        Array<u8> m_visible;
        std::shared_ptr<const resource::RuntimeResource> m_resource;
        std::thread::id m_owner;
        bool m_published=false;
    };

    /** Validates the entire sealed payload before creating hidden manager-owned
     * controls. Backend type absence and malformed data leave the parent intact. */
    WPCore_API std::shared_ptr<UIResourceInstance> stageUIResource(
        SmartPtr<IUIManager>,std::shared_ptr<const resource::RuntimeResource>,String &error);
}
