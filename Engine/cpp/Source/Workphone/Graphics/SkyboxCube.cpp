#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/SkyboxCube.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/State/States/SkyStateData.hpp>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, SkyboxCube, Sky<ISkyboxCube> );

    bool SkyboxCube::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        return false;
    }

    bool SkyboxCube::handleStateChanged( SmartPtr<IState> &state )
    {
        if( state && state->getOwnerPtr() == this )
        {
            if( isLoaded() )
            {
                auto stateData = state->getData();
                if( stateData->isDerived<SkyStateData>() )
                {
                    auto skyStateData = workphone::static_pointer_cast<SkyStateData>( stateData );

                    auto applicationManager = core::IApplicationManager::instancePtr();
                    WP_ASSERT( applicationManager );

                    auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
                    if( graphicsSystem )
                    {
                        auto smgr = graphicsSystem->getGraphicsScenePtr();
                        WP_ASSERT( smgr );

                        auto distance = getDistance();

                        if( auto skyboxMaterial = getMaterial() )
                        {
                            auto cubeMap = skyboxMaterial->getCubeTexture();
                            if( cubeMap )
                            {
                                smgr->setSkyBox( skyStateData->visible, skyboxMaterial, distance );
                                return true;
                            }
                        }
                    }
                }
            }
        }

        return false;
    }

    SkyboxCube::SkyboxCube() = default;

    SkyboxCube::~SkyboxCube() = default;

}  // namespace workphone::render
