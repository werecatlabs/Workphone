#ifndef Physics_h__
#define Physics_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Application.hpp>
#include <atomic>

namespace workphone
{

    class Physics : public core::Application
    {
    public:
        Physics();
        ~Physics() override;

        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;
        void update() override;
        void setSmokeTest( bool enabled ) { m_smokeTest = enabled; }
        bool smokeTestPassed() const { return m_smokeTestPassed; }

    protected:
        void createScene() override;

        void createPlugins() override;

        SmartPtr<scene::IGameActor> m_boxGround;
        SmartPtr<scene::IGameActor> m_lightActor;

        SmartPtr<scene::IGameActor> m_cameraActor;
        Array<SmartPtr<scene::IGameActor>> m_boxes;
        std::atomic<u32> m_physicsUpdates{ 0 };
        f64 m_nextDebugUpdate = 0.0;
        f64 m_nextDebugLog = 0.0;
        bool m_smokeTest = false;
        bool m_smokeTestPassed = false;
    };
}  // end namespace fb

#endif  // Physics_h__
