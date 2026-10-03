#ifndef CRoadGeneratorGrid_h__
#define CRoadGeneratorGrid_h__

#include <Workphone/Core/Properties.hpp>
#include <Workphone/Interface/Procedural/IRoadGenerator.hpp>
#include <WPProcedural/CRoadGenerator.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPProcedural_API CRoadGeneratorGrid : public CRoadGenerator
        {
        public:
            CRoadGeneratorGrid();
            ~CRoadGeneratorGrid() override;

            void generate() override;

            String getPatternData() const override;
            void setPatternData( const String &value ) override;

            SmartPtr<IProceduralCity> getCity() const override;
            void setCity( SmartPtr<IProceduralCity> value ) override;

        protected:
            void generateGrid();
            void generateGrid2();
            void generateGrid3();
            void generateGridAAA();

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

#endif  // RoadGeneratorDefault_h__
