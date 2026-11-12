#pragma once

#include <ui/EditorWindow.hpp>
#include <Workphone/AI/MeshyClient.hpp>
#include <Workphone/Database/AssetCatalogBrowser.hpp>
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

namespace workphone::editor
{
    class ImageToMeshWindow : public EditorWindow
    {
    public:
        ~ImageToMeshWindow() override;
        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;
        void update();

        WP_CLASS_REGISTER_DECL;

    private:
        enum Action { None, Browse, Generate, Cancel };
        enum class Provider : u32 { Meshy, Trellis2 };
        struct ResultState
        {
            mesh_generation::Result result;
            std::atomic_bool ready{ false };
            std::atomic_bool cancel{ false };
            std::mutex progressMutex;
            mesh_generation::Progress progress;
        };
        class Listener : public IEventListener
        {
        public:
            Parameter handleEvent( EventType, hash_type, const Array<Parameter> &,
                                   SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject>,
                                   SmartPtr<IEvent> ) override;
            AtomicWeakPtr<ImageToMeshWindow> owner;
            WP_CLASS_REGISTER_DECL;
        };

        void browseImage();
        void startGeneration();
        void finishGeneration();
        void discardPending( const String &message );
        void refreshProvider();
        void refreshControls();
        bool sessionCurrent() const;
        static const char *providerName( Provider provider );

        std::mutex m_mutex;
        std::thread m_worker;
        std::shared_ptr<ResultState> m_result;
        std::atomic<int> m_action{ None };
        SmartPtr<Listener> m_listener;
        SmartPtr<ui::IUIText> m_hint;
        SmartPtr<ui::IUITextEntry> m_imagePath, m_name, m_status;
        SmartPtr<ui::IUIDropdown> m_provider, m_polycount, m_resolution;
        SmartPtr<ui::IUILabelTogglePair> m_textured;
        SmartPtr<ui::IUIButton> m_browse, m_generate, m_cancel;
        AssetCatalogSession m_session;
        std::chrono::steady_clock::time_point m_nextSessionCheck{};
        String m_projectPath, m_projectRoot, m_displayProjectPath, m_basename, m_message, m_taskId;
        Provider m_selectedProvider = Provider::Meshy, m_jobProvider = Provider::Meshy;
        bool m_meshyTextured = true;
        bool m_discard = false;
    };
}
