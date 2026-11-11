#pragma once

#include <ui/EditorWindow.hpp>
#include <Workphone/Interface/Ai/IAiManager.hpp>
#include <Workphone/Database/AssetCatalogBrowser.hpp>
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

namespace workphone::editor
{
    class TextToImageWindow : public EditorWindow
    {
    public:
        ~TextToImageWindow() override;
        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;
        void update();

        WP_CLASS_REGISTER_DECL;

    private:
        enum Action { None, Generate, Cancel };
        struct ResultState
        {
            IAiManager::ImageGenerationResult result;
            std::atomic<bool> ready{ false };
        };
        class Listener : public IEventListener
        {
        public:
            Parameter handleEvent( EventType, hash_type, const Array<Parameter> &,
                                   SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject>,
                                   SmartPtr<IEvent> ) override;
            AtomicWeakPtr<TextToImageWindow> owner;
            WP_CLASS_REGISTER_DECL;
        };

        void startGeneration();
        void finishGeneration();
        void refreshControls();
        bool sessionCurrent() const;

        std::mutex m_mutex;
        std::thread m_worker;
        std::shared_ptr<ResultState> m_result;
        std::atomic<int> m_action{ None };
        SmartPtr<Listener> m_listener;
        SmartPtr<ui::IUITextEntry> m_prompt, m_name, m_status;
        SmartPtr<ui::IUIDropdown> m_size, m_quality;
        SmartPtr<ui::IUIButton> m_generate, m_cancel;
        SmartPtr<ui::IUIImage> m_preview;
        AssetCatalogSession m_session;
        AssetCatalogSession m_previewSession;
        std::chrono::steady_clock::time_point m_nextPreviewCheck{};
        String m_projectPath, m_projectRoot, m_basename, m_message, m_previewProject;
        bool m_discard = false;
    };
}
