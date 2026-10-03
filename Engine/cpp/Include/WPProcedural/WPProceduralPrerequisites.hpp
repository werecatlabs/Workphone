#ifndef WPProceduralPrerequisites_h__
#define WPProceduralPrerequisites_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Procedural/RoadTypes.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/WorkphoneConfig.hpp>

#ifdef WP_PLATFORM_WIN32
#    ifndef _WP_STATIC_LIB_
#        ifdef WPProcedural_EXPORTS
#            define WPProcedural_API __declspec( dllexport )
#        else
#            define WPProcedural_API __declspec( dllimport )
#        endif  // WP_EXPORT
#    else
#        define WPProcedural_API
#    endif  // _WP_STATIC_LIB_
#else
#    define WPProcedural_API
#endif

namespace workphone
{
    namespace procedural
    {
        class LSystem;
        class LRule;

        class Area;
        class CCityBlock;
        class BlockNode;
        class BuildingNode;
        class CCollisionManager;
        class CRoad;
        class CProceduralCity;
        class CityCenter;
        class MeshFace;
        class ICityGenerator;
        class IRoadGenerator;
        class CRoadNetwork;
        class CRoadNode;
        class IMeshGenerator;
        class CRoadElement;

        typedef IProceduralCityCenter ICityCenter;

    }  // namespace procedural

    // json data structures
    namespace data
    {
        struct road;
        struct road_connection;
        struct road_network;
        struct city_data;
        struct terrain_data;
        struct procedural_scene;
    }  // namespace data
}  // namespace workphone

#endif  // WPProceduralPrerequisites_h__
