#ifndef _OGRECONSOLE_H_
#define _OGRECONSOLE_H_

#include <Workphone/Interface/System/IConsole.hpp>
#include <OgreOverlaySystem.h>

namespace workphone
{

    class OgreConsole : public IConsole
    {
    public:
        OgreConsole();
        ~OgreConsole();

        void initialise();

        void setVisible( bool visible );

    private:
        Ogre::Overlay *m_pOverlay;
    };

}  // namespace workphone

#endif
