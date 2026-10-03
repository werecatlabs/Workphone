#ifndef FalloffGenerator_h__
#define FalloffGenerator_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Math/Math.hpp>

namespace workphone
{
    /**
     * @class FalloffGenerator
     * @brief A utility class for generating 2D falloff maps using mathematical falloff functions.
     * @tparam T The numeric type for calculations (e.g., float, double).
     *
     * The FalloffGenerator creates smooth falloff patterns that are commonly used in procedural
     * generation, terrain generation, and graphics applications. It generates a 2D array where
     * values transition smoothly from the center to the edges based on configurable falloff
     * parameters.
     *
     * The falloff function uses the formula:
     * @code
     * value^falloffDirection / (value^falloffDirection + (falloffRange - falloffRange *
     * value)^falloffDirection)
     * @endcode
     *
     * This creates smooth transitions where:
     * - falloffDirection controls the steepness of the falloff curve
     * - falloffRange controls how far the falloff extends
     *
     * @par Example Usage:
     * @code
     * FalloffGenerator<float> generator;
     * generator.setSize(256);
     * generator.setFalloffDirection(2.0f);
     * generator.setFalloffRange(1.0f);
     * auto falloffMap = generator.generate();
     * @endcode
     *
     * @see Math
     * @see Array
     */
    template <class T>
    class WPCore_API FalloffGenerator : public ISharedObject
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Initializes the generator with default values:
         * - falloffDirection = 1.0
         * - falloffRange = 1.0
         * - size = 0
         */
        FalloffGenerator();

        /**
         * @brief Virtual destructor.
         */
        ~FalloffGenerator() override;

        /**
         * @brief Generates a 2D falloff map based on the current parameters.
         * @return A 2D array containing falloff values, where each element is in the range [0, 1].
         *
         * The generated map is a square array of size x size elements. Values are calculated
         * based on the distance from the center, creating a smooth falloff pattern. The center
         * typically has values close to 0, transitioning to 1 at the edges.
         *
         * @note The size must be set to a value greater than 0 before calling this function.
         * @see setSize()
         */
        Array<Array<T>> generate();

        /**
         * @brief Evaluates the falloff function for a given input value.
         * @param value The input value to evaluate, typically in the range [0, 1].
         * @return The falloff value calculated using the current falloffDirection and falloffRange.
         *
         * This function applies the mathematical falloff formula to transform the input value.
         * The result creates smooth transitions based on the configured parameters.
         *
         * @see setFalloffDirection()
         * @see setFalloffRange()
         */
        T evaluate( T value );

        /**
         * @brief Sets the falloff direction parameter.
         * @param falloffDirection Controls the steepness of the falloff curve.
         *
         * Higher values create steeper falloff curves, while lower values create gentler transitions.
         * Typical values range from 0.5 to 5.0:
         * - Values < 1.0 create gentle, gradual falloffs
         * - Values > 1.0 create steeper, more dramatic falloffs
         * - Value = 1.0 creates a linear falloff
         *
         * @note Negative values are not recommended as they may produce unexpected results.
         */
        void setFalloffDirection( T falloffDirection );

        /**
         * @brief Sets the falloff range parameter.
         * @param falloffRange Controls how far the falloff effect extends.
         *
         * This parameter affects the overall scaling of the falloff function:
         * - Values close to 0 create very localized falloffs
         * - Values close to 1.0 create more extended falloffs
         * - Values > 1.0 may create inverted effects
         *
         * Typical values range from 0.1 to 2.0.
         */
        void setFalloffRange( T falloffRange );

        /**
         * @brief Sets the size of the generated falloff map.
         * @param size The width and height of the square falloff map to generate.
         *
         * The generated map will be a size x size 2D array. Larger sizes provide
         * higher resolution but require more memory and computation time.
         *
         * @note Size must be greater than 0. Common values range from 64 to 1024.
         */
        void setSize( s32 size );

        /**
         * @brief Gets the current falloff direction parameter.
         * @return The current falloff direction value.
         */
        T getFalloffDirection() const;

        /**
         * @brief Gets the current falloff range parameter.
         * @return The current falloff range value.
         */
        T getFalloffRange() const;

        /**
         * @brief Gets the current size setting.
         * @return The current size of the falloff map to be generated.
         */
        s32 getSize() const;

    private:
        /**
         * @brief The falloff direction parameter controlling curve steepness.
         *
         * Default value is 1.0, which creates a linear falloff.
         */
        T m_falloffDirection = T( 1.0 );

        /**
         * @brief The falloff range parameter controlling falloff extent.
         *
         * Default value is 1.0, which provides standard falloff behavior.
         */
        T m_falloffRange = T( 1.0 );

        /**
         * @brief The size of the square falloff map to generate.
         *
         * Default value is 0, which must be set before generating a map.
         */
        s32 m_size = 0;
    };

}  // namespace workphone

#endif  // FalloffGenerator_h__
