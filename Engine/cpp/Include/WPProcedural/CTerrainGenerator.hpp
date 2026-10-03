#ifndef __CTerrainGenerator_h__
#define __CTerrainGenerator_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Interface/Procedural/ITerrainGenerator.hpp>
#include <WPProcedural/CProceduralGenerator.hpp>
#include <Workphone/Math/LinearSpline1.hpp>

namespace workphone
{
    class Properties;

    namespace procedural
    {
        /**
         * @brief Centralised, serialisable configuration for CTerrainGenerator.
         *
         * Keeping all tunables in one structure makes the generator data-driven:
         * presets can be swapped, duplicated, saved to disk, and edited by tools
         * without touching the generation logic.
         */
        struct WPProcedural_API STerrainGeneratorOptions
        {
            int width = 256;
            int height = 256;
            int depth = 64;
            int octaves = 4;
            int seed = 0;

            float scale = 50.0f;
            float lacunarity = 2.0f;
            float persistence = 0.5f;
            float offset = 100.0f;
            float falloffDirection = 3.0f;
            float falloffRange = 3.0f;

            bool useFalloffMap = false;
            bool randomize = true;
            bool autoUpdate = false;
            bool normalize = true;

            SmartPtr<LinearSpline1<real_Num>> heightCurve;
        };

        /**
         * @brief Production-ready terrain generator.
         *
         * CTerrainGenerator is now a fully functional, data-driven generator.
         * All parameters live in STerrainGeneratorOptions and can be read/written
         * individually, in bulk, or through the Workphone Properties system.
         */
        class WPProcedural_API CTerrainGenerator : public CProceduralGenerator<ITerrainGenerator>
        {
        public:
            CTerrainGenerator();
            ~CTerrainGenerator() override;

            void unload( SmartPtr<ISharedObject> data ) override;

            SmartPtr<IProceduralScene> getProceduralScene() const override;
            void setProceduralScene( SmartPtr<IProceduralScene> scene ) override;

            void generate() override;

            int getWidth() const override;
            void setWidth( int width ) override;

            int getHeight() const override;
            void setHeight( int height ) override;

            int getDepth() const override;
            void setDepth( int depth ) override;

            int getOctaves() const override;
            void setOctaves( int octaves ) override;

            float getScale() const override;
            void setScale( float scale ) override;

            float getLacunarity() const override;
            void setLacunarity( float lacunarity ) override;

            float getPersistence() const override;
            void setPersistence( float persistence ) override;

            float getOffset() const override;
            void setOffset( float offset ) override;

            float getFalloffDirection() const override;
            void setFalloffDirection( float direction ) override;

            float getFalloffRange() const override;
            void setFalloffRange( float range ) override;

            bool getUseFalloffMap() const override;
            void setUseFalloffMap( bool useFalloff ) override;

            bool getRandomize() const override;
            void setRandomize( bool randomize ) override;

            bool getAutoUpdate() const override;
            void setAutoUpdate( bool autoUpdate ) override;

            void validate() override;
            void clear() override;

            SmartPtr<IProceduralTerrain> getTerrain() const override;
            void setTerrain( SmartPtr<IProceduralTerrain> terrain ) override;

            bool isFinished() const override;

            // Bulk option accessors
            const STerrainGeneratorOptions &getOptions() const;
            void setOptions( const STerrainGeneratorOptions &options );

            void resetToDefaults();

            // Extra production accessors
            int getSeed() const;
            void setSeed( int seed );

            bool getNormalize() const;
            void setNormalize( bool normalize );

            SmartPtr<LinearSpline1<real_Num>> getHeightCurve() const;
            void setHeightCurve( SmartPtr<LinearSpline1<real_Num>> curve );

            // Data-driven configuration

            virtual void loadOptions( SmartPtr<Properties> properties );

            virtual void saveOptions( SmartPtr<Properties> properties ) const;

        protected:
            virtual void generateRandom();
            virtual void generateTerrain();
            virtual Array<Array<real_Num>> generateNoise( const Array<Array<real_Num>> &falloffMap );

            void onOptionChanged();

            STerrainGeneratorOptions m_options;
            SmartPtr<IProceduralScene> m_scene;
            SmartPtr<IProceduralTerrain> m_terrain;
            bool m_finished = false;
            bool m_isGenerating = false;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // CTerrainGenerator_h__
