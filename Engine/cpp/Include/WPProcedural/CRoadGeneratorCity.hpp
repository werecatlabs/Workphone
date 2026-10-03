#ifndef CRoadGeneratorCity_h__
#define CRoadGeneratorCity_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Interface/Procedural/IRoadGenerator.hpp>
#include "WPProcedural/CRoadGenerator.hpp"
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    namespace procedural
    {

        // City road network generator. Generates a road network for a city based on a grid pattern and other parameters.
        // For different city types.
        // - Grid city: generates a grid of roads with intersections.
        // Natural city: generates roads based on terrain and natural features.
        // City centers - generates roads radiating from a central point.
        class WPProcedural_API CRoadGeneratorCity : public CRoadGenerator
        {
        public:
            CRoadGeneratorCity();
            ~CRoadGeneratorCity() override;

            void generate() override;

            String getPatternData() const override;
            void setPatternData( const String &value ) override;

            SmartPtr<IProceduralCity> getCity() const override;
            void setCity( SmartPtr<IProceduralCity> value ) override;

        protected:
            void generateGrid();
            void generateRoads();
            void generateRoads8();
            void generateCityRoadsAAA();

            SmartPtr<ISharedObject> m_selectedLayer;

            Properties m_roadNodePropGrp;
            Properties m_roadPropertyGroup;
            String m_patternName;
            SmartPtr<IProceduralCity> m_city;
            SmartPtr<IRoadNetwork> m_roadNetwork;
            SmartPtr<ICityGenerator> m_cityGenerator;

            Vector2<s32> m_gridSize = Vector2<s32>( 10, 10 );

            real_Num roadDistance = static_cast<real_Num>( 20.0 );
            real_Num roadLength = static_cast<real_Num>( 200.0 );

            Array<SmartPtr<IProceduralCity>> m_cities;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // CRoadGeneratorCity_h__
