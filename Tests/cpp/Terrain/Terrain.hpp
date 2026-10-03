#ifndef Terrain_h__
#define Terrain_h__

#include <Workphone/Application.hpp>

namespace Ogre
{
    class Terra;
    class Light;
}  // namespace Ogre

namespace workphone
{

    class Terrain : public core::Application
    {
    public:
        Terrain();
        ~Terrain() override;

        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;

        void update();

        void createScene() override;

    protected:
        void createPlugins() override;

        Ogre::Terra *mTerra = nullptr;
        Ogre::Light *mSunLight = nullptr;
    };

}  // namespace fb

#endif  // Terrain_h__
