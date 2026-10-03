// ============================================================================
// WPProceduralPipeline.cpp - AAA procedural pipeline implementation
// ============================================================================
#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/WPProceduralPipeline.hpp"

namespace workphone
{
    namespace procedural
    {
        WPProceduralPipeline::WPProceduralPipeline( u32 seed ) :
            mSeed( seed ),
            mNoise( seed ),
            mTextures( seed ),
            mBuildings( seed ),
            mInitialized( false )
        {
        }

        WPProceduralPipeline::~WPProceduralPipeline()
        {
            if( mInitialized )
                shutdown();
        }

        void WPProceduralPipeline::initialize()
        {
            SkyState state;
            state.timeOfDay = 14.0f;
            state.latitude = 32.0f;
            state.dayOfYear = 172.0f;
            mSky.setState( state );

            mInitialized = true;
        }

        void WPProceduralPipeline::shutdown()
        {
            mInitialized = false;
        }

        void WPProceduralPipeline::update( real_Num /*dt*/ )
        {
            mSky.update();
        }

        const workphone::procedural::WPTextureForge &WPProceduralPipeline::textures() const
        {
            return mTextures;
        }

        workphone::procedural::WPTextureForge &WPProceduralPipeline::textures()
        {
            return mTextures;
        }

        const workphone::procedural::WPBuildingGenerator &WPProceduralPipeline::buildings() const
        {
            return mBuildings;
        }

        workphone::procedural::WPBuildingGenerator &WPProceduralPipeline::buildings()
        {
            return mBuildings;
        }

        const workphone::procedural::WPSkyAtmosphere &WPProceduralPipeline::sky() const
        {
            return mSky;
        }

        workphone::procedural::WPSkyAtmosphere &WPProceduralPipeline::sky()
        {
            return mSky;
        }

        const workphone::procedural::WPNoise &WPProceduralPipeline::noise() const
        {
            return mNoise;
        }

        workphone::procedural::WPNoise &WPProceduralPipeline::noise()
        {
            return mNoise;
        }

        u32 WPProceduralPipeline::getSeed() const
        {
            return mSeed;
        }

        void WPProceduralPipeline::setSeed( u32 seed )
        {
            mSeed = seed;
            mNoise = WPNoise( seed );
            mTextures = WPTextureForge( seed );
            mBuildings = WPBuildingGenerator( seed );
        }
    }  // namespace procedural
}  // namespace workphone
