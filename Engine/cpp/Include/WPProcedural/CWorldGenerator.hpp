#ifndef CWorldGenerator_h__
#define CWorldGenerator_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Interface/Procedural/IWorldGenerator.hpp>
#include <WPProcedural/CProceduralGenerator.hpp>

namespace workphone
{
    class Properties;

    namespace procedural
    {
        /**
         * @brief Centralised, serialisable configuration for CWorldGenerator.
         *
         * All world-level tunables are grouped together so presets can be swapped,
         * duplicated, saved, and edited by tools without changing generation logic.
         */
        struct WPProcedural_API SWorldGeneratorOptions
        {
            int worldWidth = 2048;
            int worldHeight = 2048;
            int worldDepth = 256;
            int chunkSize = 256;
            int seed = 0;

            float scale = 100.0f;
            float lacunarity = 2.0f;
            float persistence = 0.5f;
            float seaLevel = 0.3f;
            float mountainLevel = 0.7f;
            int octaves = 4;

            bool randomizeSeed = true;
            bool autoUpdate = false;
            bool generateTerrain = true;
            bool generateCities = false;
            bool generateRoads = false;
            bool generateVegetation = false;
        };

        /**
         * @brief Production-ready world generator.
         *
         * Implements IWorldGenerator with full data-driven options, scene/world
         * management, generation orchestration, and Properties-based configuration.
         */
        class WPProcedural_API CWorldGenerator : public CProceduralGenerator<IWorldGenerator>
        {
        public:
            CWorldGenerator();
            ~CWorldGenerator() override;

            void unload( SmartPtr<ISharedObject> data ) override;

            SmartPtr<IProceduralWorld> getProceduralWorld() const override;
            void setProceduralWorld( SmartPtr<IProceduralWorld> proceduralWorld ) override;

            const Array<SmartPtr<IProceduralScene>> getScenes() const override;
            void addScene( SmartPtr<IProceduralScene> scene ) override;
            void removeScene( SmartPtr<IProceduralScene> scene ) override;
            void setScenes( Array<SmartPtr<IProceduralScene>> scenes ) override;

            void generate() override;
            void validate();
            void clear();

            bool isFinished() const override;

            // Bulk option accessors
            const SWorldGeneratorOptions &getOptions() const;
            void setOptions( const SWorldGeneratorOptions &options );

            void resetToDefaults();

            // Per-option accessors
            int getWorldWidth() const;
            void setWorldWidth( int width );

            int getWorldHeight() const;
            void setWorldHeight( int height );

            int getWorldDepth() const;
            void setWorldDepth( int depth );

            int getChunkSize() const;
            void setChunkSize( int size );

            int getSeed() const;
            void setSeed( int seed );

            float getScale() const;
            void setScale( float scale );

            float getLacunarity() const;
            void setLacunarity( float lacunarity );

            float getPersistence() const;
            void setPersistence( float persistence );

            float getSeaLevel() const;
            void setSeaLevel( float seaLevel );

            float getMountainLevel() const;
            void setMountainLevel( float mountainLevel );

            int getOctaves() const;
            void setOctaves( int octaves );

            bool getRandomizeSeed() const;
            void setRandomizeSeed( bool randomize );

            bool getAutoUpdate() const;
            void setAutoUpdate( bool autoUpdate );

            bool getGenerateTerrain() const;
            void setGenerateTerrain( bool generate );

            bool getGenerateCities() const;
            void setGenerateCities( bool generate );

            bool getGenerateRoads() const;
            void setGenerateRoads( bool generate );

            bool getGenerateVegetation() const;
            void setGenerateVegetation( bool generate );

            // Data-driven configuration
            void loadOptions( SmartPtr<Properties> properties );
            void saveOptions( SmartPtr<Properties> properties ) const;

        protected:
            void onOptionChanged();

            void generateWorld();
            void generateScenes();
            SmartPtr<IProceduralScene> createDefaultScene();

            SWorldGeneratorOptions m_options;
            SmartPtr<IProceduralWorld> m_world;
            Array<SmartPtr<IProceduralScene>> m_scenes;

            bool m_finished = false;
            bool m_isGenerating = false;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // CWorldGenerator_h__
