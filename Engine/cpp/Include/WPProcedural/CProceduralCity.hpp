#ifndef __CProceduralCity_h__
#define __CProceduralCity_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Interface/Procedural/IProceduralCity.hpp>
#include <Workphone/Math/Sphere3.hpp>

namespace workphone
{
    class Properties;

    namespace procedural
    {
        /**
         * @brief Centralised, serialisable configuration for CProceduralCity.
         *
         * All city-level tunables live here so presets can be swapped, duplicated,
         * saved, and edited by tools without touching city logic.
         */
        struct WPProcedural_API SCityOptions
        {
            String name;
            Vector2<real_Num> size = Vector2<real_Num>( 500, 500 );
            Vector2<real_Num> minLatLong = Vector2<real_Num>::zero();
            Vector2<real_Num> maxLatLong = Vector2<real_Num>::zero();
            real_Num defaultCenterRadius = 100.0;
            u32 maxCenters = 8;
            u32 maxBlocks = 1024;
            bool autoBuildNetwork = true;
        };

        /**
         * @class CProceduralCity
         * @brief Concrete implementation of a procedural city.
         *
         * Manages the layout, road networks, city centers, and blocks of a procedurally generated city.
         * It handles spatial bounds using latitude/longitude and provides utilities for coordinate conversion.
         */
        class WPProcedural_API CProceduralCity : public IProceduralCity
        {
        public:
            CProceduralCity();
            ~CProceduralCity() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            SmartPtr<IRoadNetwork> getRoadNetwork() const override;
            void setRoadNetwork( SmartPtr<IRoadNetwork> value ) override;

            bool isWithin( const Sphere3F &sphere ) const override;

            SmartPtr<ICityMap> getArea() const override;
            void setArea( SmartPtr<ICityMap> value ) override;

            Array<SmartPtr<IProceduralCityCenter>> getCityCenters() const override;
            void setCityCenters( Array<SmartPtr<IProceduralCityCenter>> cityCenters ) override;
            void addCenter( SmartPtr<IProceduralCityCenter> center ) override;
            void removeCenter( SmartPtr<IProceduralCityCenter> center ) override;

            void addBlock( SmartPtr<ICityBlock> block ) override;
            void removeBlock( SmartPtr<ICityBlock> block ) override;
            Array<SmartPtr<ICityBlock>> getBlocks() const override;

            Vector2<real_Num> getSize() const override;
            void setSize( const Vector2<real_Num> &value ) override;

            Vector2<real_Num> getRelativeCoordinates( const Vector2<real_Num> &lat_long ) override;

            SmartPtr<IRoad> getRoadByName( const String &name ) override;
            Array<SmartPtr<IRoad>> getRoads() const override;

            Vector2<real_Num> getMinLatLong() const override;
            void setMinLatLong( const Vector2<real_Num> &minLatLong ) override;

            Vector2<real_Num> getMaxLatLong() const override;
            void setMaxLatLong( const Vector2<real_Num> &maxLatLong ) override;

            // Production accessors
            const SCityOptions &getOptions() const;
            void setOptions( const SCityOptions &options );
            void resetToDefaults();

            String getName() const;
            void setName( const String &name );

            real_Num getDefaultCenterRadius() const;
            void setDefaultCenterRadius( real_Num radius );

            u32 getMaxCenters() const;
            void setMaxCenters( u32 maxCenters );

            u32 getMaxBlocks() const;
            void setMaxBlocks( u32 maxBlocks );

            bool getAutoBuildNetwork() const;
            void setAutoBuildNetwork( bool autoBuild );

            // Data-driven configuration
            void loadOptions( SmartPtr<Properties> properties );
            void saveOptions( SmartPtr<Properties> properties ) const;

        protected:
            void validate();

            SmartPtr<IRoadNetwork> m_roadNetwork;  ///< Road network of the city
            SmartPtr<ICityMap> m_area;             ///< The map area representing the city layout
            Array<SmartPtr<IProceduralCityCenter>> m_cityCenters;  ///< List of city centers
            Array<SmartPtr<ICityBlock>> m_blocks;                  ///< List of city blocks

            SCityOptions m_options;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // ProceduralCity_h__
