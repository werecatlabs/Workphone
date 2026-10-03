#ifndef WPProcedural_h__
#define WPProcedural_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <WPProcedural/Area.hpp>
#include <WPProcedural/BlockNode.hpp>
#include <WPProcedural/CBlockGenerator.hpp>
#include <WPProcedural/CCityBlock.hpp>
#include <WPProcedural/CCollisionManager.hpp>
#include <WPProcedural/CityCenter.hpp>
#include <WPProcedural/CityGeneratorDefault.hpp>
#include <WPProcedural/CProceduralCity.hpp>
#include <WPProcedural/CProceduralCityCenter.hpp>
#include <WPProcedural/CProceduralGenerator.hpp>
#include <WPProcedural/CProceduralWorld.hpp>
#include <WPProcedural/CProceduralScene.hpp>
#include <WPProcedural/CProceduralTerrain.hpp>
#include <WPProcedural/CTerrainGeneratorDefault.hpp>
#include <WPProcedural/CRoadGeneratorGrid.hpp>
#include <WPProcedural/CRoad.hpp>
#include <WPProcedural/CRoadElement.hpp>
#include <WPProcedural/CRoadNetwork.hpp>
#include <WPProcedural/CRoadNode.hpp>
#include <WPProcedural/CRoadSection.hpp>
#include <WPProcedural/CProceduralManager.hpp>
#include <WPProcedural/CTerrainGenerator.hpp>
#include <WPProcedural/MeshGeneratorDefault.hpp>
#include <WPProcedural/WPVehicleDamage.hpp>
#include <WPProcedural/WPVehicleDynamics.hpp>
#include <WPProcedural/WPVehicleEffects.hpp>
#include <WPProcedural/WPVehicleGenerator.hpp>
#include <WPProcedural/WPVehiclePresentation.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPProcedural_API WPProcedural : public ISharedObject
        {
        public:
            WPProcedural();
            ~WPProcedural() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            static SmartPtr<WPProcedural> instance();
            static void setInstance( SmartPtr<WPProcedural> plugin );

        protected:
            static SmartPtr<WPProcedural> m_sPlugin;
        };
    }  // namespace procedural
}  // namespace workphone

#endif  // WPProcedural_h__
