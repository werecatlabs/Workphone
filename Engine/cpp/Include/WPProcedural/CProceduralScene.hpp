#ifndef CProceduralScene_h__
#define CProceduralScene_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Procedural/IProceduralScene.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPProcedural_API CProceduralScene : public IProceduralScene
        {
        public:
            CProceduralScene();
            ~CProceduralScene() override;

            void addCity( SmartPtr<IProceduralCity> city ) override;
            void removeCity( SmartPtr<IProceduralCity> city ) override;
            Array<SmartPtr<IProceduralCity>> getCities() const override;
            void setCities( Array<SmartPtr<IProceduralCity>> cities ) override;

            void addTerrain( SmartPtr<IProceduralTerrain> terrain ) override;
            void removeTerrain( SmartPtr<IProceduralTerrain> terrain ) override;
            Array<SmartPtr<IProceduralTerrain>> getTerrains() const override;
            void setTerrains( Array<SmartPtr<IProceduralTerrain>> terrains ) override;

        private:
            Array<SmartPtr<IProceduralCity>> m_cities;
            Array<SmartPtr<IProceduralTerrain>> m_terrains;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // CProceduralScene_h__
