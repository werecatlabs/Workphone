#ifndef CTerrainGeneratorDefault_h__
#define CTerrainGeneratorDefault_h__

#include <WPProcedural/CTerrainGenerator.hpp>

namespace workphone
{
    namespace procedural
    {
        /**
         * @brief Default terrain generator implementation.
         *
         * This generator reuses the centralised STerrainGeneratorOptions from
         * CTerrainGenerator and provides the same production accessors. It keeps
         * the original square-terrain behaviour (width drives both width and height)
         * while being fully data-driven and serialisable through Properties.
         */
        class WPProcedural_API CTerrainGeneratorDefault : public CTerrainGenerator
        {
        public:
            CTerrainGeneratorDefault();
            ~CTerrainGeneratorDefault() override;

            void generate() override;
            void validate() override;
            void clear() override;

            // Properties-driven configuration
            void loadOptions( SmartPtr<Properties> properties ) override;
            void saveOptions( SmartPtr<Properties> properties ) const override;

        protected:
            void generateRandom() override;
            void generateTerrain() override;
            Array<Array<real_Num>> generateNoise( const Array<Array<real_Num>> &falloffMap ) override;
        };
    }  // namespace procedural
}  // namespace workphone

#endif  // CTerrainGeneratorDefault_h__
