#ifndef __Game_h__
#define __Game_h__

#include <Workphone/Application.hpp>

namespace workphone
{

    class Game : public core::Application
    {
    public:
        Game();
        ~Game() override;

        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;

    protected:
        void createScene() override;
        void createPlugins() override;
    };
}  // namespace workphone

#endif  // __Game_h__
