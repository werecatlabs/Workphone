#ifndef WP_WORLD_STREAMING_COMPONENT_HPP
#define WP_WORLD_STREAMING_COMPONENT_HPP
#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Scene/SceneWorldStreamer.hpp>

namespace workphone::scene
{
    /** Attach to a persistent player/interest actor. Editor camera demand is an
     * explicit preview option; runtime defaults to the supplied observer/owner. */
    class WPCore_API WorldStreaming : public Component
    {
    public:
        WorldStreaming();
        ~WorldStreaming() override;
        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;
        void preUpdate() override;
        SmartPtr<Properties> getProperties() const override;
        void setProperties( SmartPtr<Properties> properties ) override;
        SceneWorldStreamStatistics statistics() const;
        WP_CLASS_REGISTER_DECL;

    private:
        String m_manifestPath, m_openPath, m_error;
        SceneWorldStreamOptions m_options;
        WeakPtr<IGameActor> m_observer;
        bool m_previewEditorCamera = false, m_followActiveCamera = false, m_instanceWorld = false;
        bool m_reopen = false, m_updating = false, m_unloading = false;
        std::shared_ptr<SceneWorldStreamer> m_streamer;
    };
}  // namespace workphone::scene
#endif
