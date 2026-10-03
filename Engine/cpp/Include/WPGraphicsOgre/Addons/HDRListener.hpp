#ifndef _HDRLISTENER_H
#define _HDRLISTENER_H

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include "Ogre.h"

namespace workphone
{
    namespace render
    {

        class HDRListener : public Ogre::CompositorInstance::Listener
        {
        public:
            HDRListener();
            ~HDRListener();

            void notifyViewportSize( int width, int height );
            void notifyCompositor( Ogre::CompositorInstance *instance );
            virtual void notifyMaterialSetup( Ogre::uint32 pass_id, Ogre::MaterialPtr &mat );
            virtual void notifyMaterialRender( Ogre::uint32 pass_id, Ogre::MaterialPtr &mat );

        protected:
            int mVpWidth, mVpHeight;
            int mBloomSize;
            // Array params - have to pack in groups of 4 since this is how Cg generates them
            // also prevents dependent texture read problems if ops don't require swizzle
            float mBloomTexWeights[15][4];
            float mBloomTexOffsetsHorz[15][4];
            float mBloomTexOffsetsVert[15][4];
        };

    }  // namespace render
}  // namespace workphone

#endif
