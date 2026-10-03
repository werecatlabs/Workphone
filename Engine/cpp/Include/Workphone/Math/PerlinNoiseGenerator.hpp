#ifndef PerlinNoiseGenerator_h__
#define PerlinNoiseGenerator_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Math/Math.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    /**
     * @brief A template class for generating 2D Perlin noise maps.
     *
     * The PerlinNoiseGenerator creates coherent noise using the Perlin noise algorithm
     * with configurable octaves, scale, persistence, and lacunarity parameters.
     * This is commonly used for procedural terrain generation, texture synthesis,
     * and other applications requiring natural-looking random patterns.
     *
     * @tparam T The numeric type for noise values (typically float or double)
     *
     * @note The generator produces a square noise map of size x size dimensions.
     * @note All noise values are normalized between the calculated min and max heights.
     *
     * @see Math<T>::PerlinNoise for the underlying noise function
     *
     * @author Workphone Math Library
     * @version 1.0
     */
    template <class T>
    class WPCore_API PerlinNoiseGenerator : public ISharedObject
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Initializes the generator with default parameters:
         * - Size: 0
         * - Octaves: 0
         * - Scale: 1.0
         * - Offset: 0.0
         * - Persistence: 0.0
         * - Lacunarity: 0.0
         */
        PerlinNoiseGenerator();

        /**
         * @brief Virtual destructor.
         */
        ~PerlinNoiseGenerator() override;

        /**
         * @brief Generates a 2D noise map without returning height bounds.
         *
         * This is a convenience method that internally tracks the minimum and maximum
         * noise heights but doesn't expose them to the caller.
         *
         * @return A 2D array containing the generated noise values
         *
         * @note The returned array dimensions are [size][size]
         * @note Requires size > 0 and octaves > 0 to produce meaningful results
         */
        Array<Array<T>> generate();

        /**
         * @brief Generates a 2D noise map and returns the height bounds.
         *
         * @param[out] maxLocalNoiseHeight Reference to store the maximum noise value generated
         * @param[out] minLocalNoiseHeight Reference to store the minimum noise value generated
         *
         * @return A 2D array containing the generated noise values
         *
         * @note The returned array dimensions are [size][size]
         * @note The height bounds can be used for normalizing the noise values
         */
        Array<Array<T>> generate( T &maxLocalNoiseHeight, T &minLocalNoiseHeight );

        /**
         * @brief Core noise generation function with height bound tracking.
         *
         * This method implements the multi-octave Perlin noise algorithm:
         * 1. For each octave, generates noise at different frequencies and amplitudes
         * 2. Combines octaves using persistence and lacunarity parameters
         * 3. Tracks minimum and maximum values for normalization
         *
         * @param[out] maxLocalNoiseHeight Reference to store the maximum noise value
         * @param[out] minLocalNoiseHeight Reference to store the minimum noise value
         *
         * @return A 2D array of noise values with dimensions [size][size]
         *
         * @note Scale values <= 0 are automatically clamped to 0.0001 to prevent division by zero
         * @note The algorithm centers the noise around the middle of the size dimensions
         */
        Array<Array<T>> generateNoise( T &maxLocalNoiseHeight, T &minLocalNoiseHeight );

        /**
         * @brief Gets the size of the generated noise map.
         *
         * @return The width and height of the square noise map
         */
        s32 getSize() const;

        /**
         * @brief Sets the size of the noise map to generate.
         *
         * @param size The width and height of the square noise map (must be > 0)
         *
         * @note Larger sizes produce more detailed maps but require more computation time
         */
        void setSize( s32 size );

        /**
         * @brief Gets the number of octaves used in noise generation.
         *
         * @return The current number of octaves
         */
        s32 getOctaves() const;

        /**
         * @brief Sets the number of octaves for noise generation.
         *
         * More octaves add finer detail to the noise at the cost of performance.
         * Typical values range from 1 to 8.
         *
         * @param octaves The number of noise octaves to combine (must be > 0)
         *
         * @note Each octave roughly doubles the frequency and halves the amplitude
         * @note More octaves create more detailed, realistic-looking noise
         */
        void setOctaves( s32 octaves );

        /**
         * @brief Gets the scale factor for noise coordinates.
         *
         * @return The current scale value
         */
        T getScale() const;

        /**
         * @brief Sets the scale factor for noise sampling.
         *
         * Scale controls the "zoom level" of the noise. Smaller values create
         * more zoomed-in, higher frequency noise, while larger values create
         * more zoomed-out, lower frequency noise.
         *
         * @param scale The scale factor (must be > 0, will be clamped to 0.0001 minimum)
         *
         * @note A scale of 1.0 samples the noise at unit intervals
         * @note Smaller scales (e.g., 0.1) create finer, more detailed patterns
         * @note Larger scales (e.g., 10.0) create broader, smoother patterns
         */
        void setScale( T scale );

        /**
         * @brief Gets the offset applied to noise coordinates.
         *
         * @return The current offset value
         */
        T getOffset() const;

        /**
         * @brief Sets the offset for noise sampling coordinates.
         *
         * The offset shifts the sampling position in the noise space, effectively
         * allowing you to sample different regions of the infinite noise field.
         *
         * @param offset The coordinate offset to apply to all octaves
         *
         * @note Different offset values produce different but deterministic noise patterns
         * @note Can be used to generate multiple non-repeating noise maps
         */
        void setOffset( T offset );

        /**
         * @brief Gets the persistence value controlling amplitude decay.
         *
         * @return The current persistence value
         */
        T getPersistance() const;

        /**
         * @brief Sets the persistence factor for octave amplitude scaling.
         *
         * Persistence controls how much each successive octave contributes to the final result.
         * It determines the amplitude multiplier between octaves.
         *
         * @param persistance The persistence factor (typically between 0.0 and 1.0)
         *
         * @note Values close to 0 make higher octaves contribute very little (smoother result)
         * @note Values close to 1 make all octaves contribute equally (more chaotic result)
         * @note Typical values range from 0.25 to 0.75 for natural-looking terrain
         */
        void setPersistance( T persistance );

        /**
         * @brief Gets the lacunarity value controlling frequency scaling.
         *
         * @return The current lacunarity value
         */
        T getLacunarity() const;

        /**
         * @brief Sets the lacunarity factor for octave frequency scaling.
         *
         * Lacunarity controls how much the frequency increases between octaves.
         * It determines the frequency multiplier for each successive octave.
         *
         * @param lacunarity The lacunarity factor (typically > 1.0)
         *
         * @note A value of 2.0 doubles the frequency with each octave (standard)
         * @note Higher values create more dramatic frequency increases between octaves
         * @note Lower values create more gradual frequency transitions
         * @note Typical values range from 1.5 to 3.0
         */
        void setLacunarity( T lacunarity );

    private:
        s32 m_size = 0;              ///< Width and height of the square noise map
        s32 m_octaves = 0;           ///< Number of noise octaves to combine
        T m_scale = T( 1.0 );        ///< Scale factor for noise coordinate sampling
        T m_offset = T( 0.0 );       ///< Offset applied to noise coordinates
        T m_persistance = T( 0.0 );  ///< Amplitude decay factor between octaves
        T m_lacunarity = T( 0.0 );   ///< Frequency scaling factor between octaves
    };

}  // namespace workphone

#endif  // PerlinNoiseGenerator_h__
