#ifndef ITerrainGenerator_h__
#define ITerrainGenerator_h__

#include <Workphone/Interface/Procedural/IProceduralGenerator.hpp>

namespace workphone
{
    namespace procedural
    {
        /**
         * @brief Interface for procedural terrain generation.
         *
         * This interface defines the contract for terrain generator implementations, providing
         * accessors and mutators for terrain parameters, as well as methods for validation,
         * clearing, and access to generated terrain and procedural scenes.
         */
        class WPCore_API ITerrainGenerator : public IProceduralGenerator
        {
        public:
            /**
             * @brief Virtual destructor.
             */
            ~ITerrainGenerator() override;

            /**
             * @brief Gets the procedural scene associated with this terrain generator.
             * @return Smart pointer to the procedural scene.
             */
            virtual SmartPtr<IProceduralScene> getProceduralScene() const = 0;

            /**
             * @brief Sets the procedural scene for this terrain generator.
             * @param scene Smart pointer to the procedural scene.
             */
            virtual void setProceduralScene( SmartPtr<IProceduralScene> scene ) = 0;

            // Terrain parameter accessors

            /**
             * @brief Gets the width of the terrain.
             * @return Width in units.
             */
            virtual int getWidth() const = 0;

            /**
             * @brief Sets the width of the terrain.
             * @param width Width in units.
             */
            virtual void setWidth( int width ) = 0;

            /**
             * @brief Gets the height of the terrain.
             * @return Height in units.
             */
            virtual int getHeight() const = 0;

            /**
             * @brief Sets the height of the terrain.
             * @param height Height in units.
             */
            virtual void setHeight( int height ) = 0;

            /**
             * @brief Gets the depth of the terrain.
             * @return Depth in units.
             */
            virtual int getDepth() const = 0;

            /**
             * @brief Sets the depth of the terrain.
             * @param depth Depth in units.
             */
            virtual void setDepth( int depth ) = 0;

            /**
             * @brief Gets the number of octaves used in noise generation.
             * @return Number of octaves.
             */
            virtual int getOctaves() const = 0;

            /**
             * @brief Sets the number of octaves for noise generation.
             * @param octaves Number of octaves.
             */
            virtual void setOctaves( int octaves ) = 0;

            /**
             * @brief Gets the scale factor for noise generation.
             * @return Scale factor.
             */
            virtual float getScale() const = 0;

            /**
             * @brief Sets the scale factor for noise generation.
             * @param scale Scale factor.
             */
            virtual void setScale( float scale ) = 0;

            /**
             * @brief Gets the lacunarity value for fractal noise.
             * @return Lacunarity value.
             */
            virtual float getLacunarity() const = 0;

            /**
             * @brief Sets the lacunarity value for fractal noise.
             * @param lacunarity Lacunarity value.
             */
            virtual void setLacunarity( float lacunarity ) = 0;

            /**
             * @brief Gets the persistence value for fractal noise.
             * @return Persistence value.
             */
            virtual float getPersistence() const = 0;

            /**
             * @brief Sets the persistence value for fractal noise.
             * @param persistence Persistence value.
             */
            virtual void setPersistence( float persistence ) = 0;

            /**
             * @brief Gets the offset value for noise generation.
             * @return Offset value.
             */
            virtual float getOffset() const = 0;

            /**
             * @brief Sets the offset value for noise generation.
             * @param offset Offset value.
             */
            virtual void setOffset( float offset ) = 0;

            /**
             * @brief Gets the direction of the falloff effect.
             * @return Falloff direction.
             */
            virtual float getFalloffDirection() const = 0;

            /**
             * @brief Sets the direction of the falloff effect.
             * @param direction Falloff direction.
             */
            virtual void setFalloffDirection( float direction ) = 0;

            /**
             * @brief Gets the range of the falloff effect.
             * @return Falloff range.
             */
            virtual float getFalloffRange() const = 0;

            /**
             * @brief Sets the range of the falloff effect.
             * @param range Falloff range.
             */
            virtual void setFalloffRange( float range ) = 0;

            /**
             * @brief Checks if a falloff map is used in terrain generation.
             * @return True if a falloff map is used, false otherwise.
             */
            virtual bool getUseFalloffMap() const = 0;

            /**
             * @brief Sets whether to use a falloff map in terrain generation.
             * @param useFalloff True to use a falloff map, false otherwise.
             */
            virtual void setUseFalloffMap( bool useFalloff ) = 0;

            /**
             * @brief Checks if randomization is enabled for terrain generation.
             * @return True if randomization is enabled, false otherwise.
             */
            virtual bool getRandomize() const = 0;

            /**
             * @brief Sets whether to randomize terrain generation.
             * @param randomize True to randomize, false otherwise.
             */
            virtual void setRandomize( bool randomize ) = 0;

            /**
             * @brief Checks if auto-update is enabled for terrain generation.
             * @return True if auto-update is enabled, false otherwise.
             */
            virtual bool getAutoUpdate() const = 0;

            /**
             * @brief Sets whether to enable auto-update for terrain generation.
             * @param autoUpdate True to enable auto-update, false otherwise.
             */
            virtual void setAutoUpdate( bool autoUpdate ) = 0;

            // Validation and utility

            /**
             * @brief Validates the current terrain parameters and configuration.
             */
            virtual void validate() = 0;

            /**
             * @brief Clears the current terrain data and resets parameters.
             */
            virtual void clear() = 0;

            // Noise/heightmap generation (optional, can be pure virtual or default)
            // virtual Array<Array<real_Num>> generateNoise(const Array<Array<real_Num>>& falloffMap) =
            // 0;

            // Access to generated terrain(s)

            /**
             * @brief Gets the generated procedural terrain.
             * @return Smart pointer to the procedural terrain.
             */
            virtual SmartPtr<IProceduralTerrain> getTerrain() const = 0;

            /**
             * @brief Sets the generated procedural terrain.
             * @param terrain Smart pointer to the procedural terrain.
             */
            virtual void setTerrain( SmartPtr<IProceduralTerrain> terrain ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // ITerrainGenerator_h__
