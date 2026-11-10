#pragma once

#include <ui/EditorWindow.hpp>
#include <Workphone/Jobs/JobCreatePackage.hpp>
#include <Workphone/Interface/UI/IUIButton.hpp>
#include <Workphone/Interface/UI/IUITextEntry.hpp>
#include <mutex>
#include <thread>

namespace workphone::editor
{
    class GamePackageWindow : public EditorWindow
    {
    public:
        ~GamePackageWindow() override;
        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;
        void setWindowVisible( bool visible ) override;
        void update();

        WP_CLASS_REGISTER_DECL;

    private:
        enum Action { Browse = 1, Build, Cancel, OpenFolder, Run };

        class Listener : public IEventListener
        {
        public:
            Parameter handleEvent( EventType, hash_type, const Array<Parameter> &,
                                   SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject>,
                                   SmartPtr<IEvent> ) override;
            AtomicWeakPtr<GamePackageWindow> owner;
            WP_CLASS_REGISTER_DECL;
        };

        void handleAction( Action action );
        void refresh();
        void startBuild();
        void browse();

        std::mutex m_mutex;
        std::thread m_worker;
        SmartPtr<Listener> m_listener;
        SmartPtr<JobCreatePackage> m_job;
        SmartPtr<ui::IUITextEntry> m_destination;
        SmartPtr<ui::IUIText> m_status;
        SmartPtr<ui::IUIButton> m_browse, m_build, m_cancel, m_open, m_run;
        String m_projectFile;
        String m_message;
        bool m_cancelling = false;
        bool m_usedDefaultDestination = false;
    };
}
