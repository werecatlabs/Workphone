#ifndef RoadGeneratorDefault_h__
#define RoadGeneratorDefault_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Interface/Procedural/IRoadGenerator.hpp>
#include <Workphone/Interface/Procedural/IRoadConnection.hpp>
#include <WPProcedural/CProceduralGenerator.hpp>

namespace workphone
{
    class Properties;

    namespace procedural
    {
        /**
         * @brief Centralised, serialisable configuration for CRoadGenerator.
         *
         * All road-generation tunables live here so presets can be swapped,
         * duplicated, saved, and edited by tools without touching the
         * generation algorithms.
         */
        struct WPProcedural_API SRoadGeneratorOptions
        {
            String patternName;
            int seed = 0;

            u32 numMajorRoads = 6;
            s32 numMinorRoadsPerMajor = 1;

            f32 roadOffset = 100.0f;
            f32 roadDistance = 100.0f;
            f32 roadLength = 200.0f;
            f32 rotationVariation = 0.0f;
            f32 connectionSearchRadius = 20.0f;
            f32 nodeBiasHeight = 1000.0f;

            bool generateMajorRoads = true;
            bool generateMinorRoads = true;
            bool generateIntersections = true;
            bool randomizeSeed = false;
            bool autoUpdate = false;
        };

        /**
         * @brief Production-ready road generator.
         *
         * Implements IRoadGenerator with a data-driven option set, full
         * accessor coverage, validation, and Properties-based configuration.
         */
        class WPProcedural_API CRoadGenerator : public CProceduralGenerator<IRoadGenerator>
        {
        public:
            CRoadGenerator();
            ~CRoadGenerator() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            void generate() override;

            void validate();
            void clear();

            bool isFinished() const override;

            String getPatternData() const override;
            void setPatternData( const String &value ) override;

            SmartPtr<IProceduralCity> getCity() const override;
            void setCity( SmartPtr<IProceduralCity> value ) override;

            // Bulk option accessors
            const SRoadGeneratorOptions &getOptions() const;
            void setOptions( const SRoadGeneratorOptions &options );

            void resetToDefaults();

            // Per-option accessors
            int getSeed() const;
            void setSeed( int seed );

            u32 getNumMajorRoads() const;
            void setNumMajorRoads( u32 count );

            s32 getNumMinorRoadsPerMajor() const;
            void setNumMinorRoadsPerMajor( s32 count );

            float getRoadOffset() const;
            void setRoadOffset( float offset );

            float getRoadDistance() const;
            void setRoadDistance( float distance );

            float getRoadLength() const;
            void setRoadLength( float length );

            float getRotationVariation() const;
            void setRotationVariation( float variation );

            float getConnectionSearchRadius() const;
            void setConnectionSearchRadius( float radius );

            bool getGenerateMajorRoads() const;
            void setGenerateMajorRoads( bool generate );

            bool getGenerateMinorRoads() const;
            void setGenerateMinorRoads( bool generate );

            float getNodeBiasHeight() const;
            void setNodeBiasHeight( float height );

            bool getGenerateIntersections() const;
            void setGenerateIntersections( bool generate );

            bool getRandomizeSeed() const;
            void setRandomizeSeed( bool randomize );

            bool getAutoUpdate() const;
            void setAutoUpdate( bool autoUpdate );

            // Data-driven configuration
            void loadOptions( SmartPtr<Properties> properties );
            void saveOptions( SmartPtr<Properties> properties ) const;

        protected:
            void onOptionChanged();

            void generateGrid();
            void generateRoads();
            void generateRoads8();
            void createIntersections();
            void generateRoadsAAA();
            void createIntersectionsAAA();

            SmartPtr<IRoadConnection> createConnection( const String &assetPath,
                                                        Transform3<real_Num> t );
            SmartPtr<IRoad> createRingRoad( SmartPtr<IProceduralCityCenter> targetCityCenter,
                                            f32 distanceAlongRoad );

            // Legacy OSM helpers (currently stubs; original implementation relied on
            // OSM data that is no longer wired into this build).
            bool isRoad( SmartPtr<ISharedObject> data, const String &id );
            String getTagValue( SmartPtr<ISharedObject> data, const String &id, const String &tag );

            SmartPtr<ISharedObject> m_selectedLayer;
            Properties m_roadNodePropGrp;
            Properties m_roadPropertyGroup;

            SmartPtr<IProceduralCity> m_city;
            SmartPtr<IRoadNetwork> m_roadNetwork;
            SmartPtr<ICityGenerator> m_cityGenerator;
            Array<SmartPtr<IProceduralCity>> m_cities;

            SRoadGeneratorOptions m_options;
            bool m_finished = false;
            bool m_isGenerating = false;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // RoadGeneratorDefault_h__
