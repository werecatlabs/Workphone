#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include "WPGraphicsOgre/Addons/GodRaysListener.hpp"
#include <Ogre.h>

namespace workphone
{
    namespace render
    {

        GodRaysListener::GodRaysListener() : SunScreenSpacePosition( Ogre::Vector3::ZERO )
        {
        }

        void GodRaysListener::notifyMaterialSetup( Ogre::uint32 pass_id, Ogre::MaterialPtr &mat )
        {
            if( pass_id == 1 )
                params1 = mat->getTechnique( 0 )->getPass( 0 )->getVertexProgramParameters();
            else if( pass_id == 2 )
                params2 = mat->getTechnique( 0 )->getPass( 0 )->getVertexProgramParameters();
            if( pass_id == 3 )
                params3 = mat->getTechnique( 0 )->getPass( 0 )->getVertexProgramParameters();
        }

        void GodRaysListener::notifyMaterialRender( Ogre::uint32 pass_id, Ogre::MaterialPtr &mat )
        {
            if( pass_id == 1 )
                params1->setNamedConstant( "lightPosition", SunScreenSpacePosition );
            else if( pass_id == 2 )
                params2->setNamedConstant( "lightPosition", SunScreenSpacePosition );
            if( pass_id == 3 )
                params3->setNamedConstant( "lightPosition", SunScreenSpacePosition );
        }

    }  // namespace render
}  // namespace workphone
