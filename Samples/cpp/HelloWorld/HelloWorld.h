#ifndef __HelloWorld_H
#define __HelloWorld_H

#include <Workphone/Application.hpp>

namespace workphone
{

    class HelloWorld : public core::Application
    {
    public:
        HelloWorld();
        ~HelloWorld() override;

        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;
        void update() override;

#ifdef WP_ENABLE_TRACE
        s32 addReference();
        bool removeReference();
#endif

    protected:
        void createScene() override;

        void createPlugins() override;

        time_interval m_fpsSampleStart = 0.0;
        u32 m_fpsFrameCount = 0;
    };

}  // namespace workphone

#endif  // __HelloWorld_H
