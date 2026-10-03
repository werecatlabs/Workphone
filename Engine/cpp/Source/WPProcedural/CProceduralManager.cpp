#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/CProceduralManager.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace procedural
    {
        WP_CLASS_REGISTER_DERIVED( workphone, CProceduralManager, IProceduralManager );

        CProceduralManager::CProceduralManager()
        {
        }

        CProceduralManager::~CProceduralManager()
        {
        }

        void CProceduralManager::generate()
        {
        }

        void CProceduralManager::removeObject( SmartPtr<ISharedObject> object )
        {
        }

        void CProceduralManager::addObject( SmartPtr<ISharedObject> object )
        {
            // ProceduralNodePtr node = object;
            // const Handle& handle = node->getHandle();
            // String idStr = handle.getName();
            // m_object[idStr] = node;
        }

        SmartPtr<ISharedObject> CProceduralManager::findObject( Handle handle )
        {
            SmartPtr<ISharedObject> object;
            return object;
        }

        void CProceduralManager::setAi( SmartPtr<IAi> value )
        {
            m_ai = value;
        }

        SmartPtr<IAiManager> CProceduralManager::getAiManager() const
        {
            return m_aiManager;
        }

        void CProceduralManager::setAiManager( SmartPtr<IAiManager> aiManager )
        {
            m_aiManager = aiManager;
        }

        SmartPtr<ICityGenerator> CProceduralManager::getCityGenerator() const
        {
            return m_cityGenerator;
        }

        void CProceduralManager::setCityGenerator( SmartPtr<ICityGenerator> value )
        {
            m_cityGenerator = value;
        }

        SmartPtr<ITerrainGenerator> CProceduralManager::getTerrainGenerator() const
        {
            return m_terrainGenerator;
        }

        void CProceduralManager::setTerrainGenerator( SmartPtr<ITerrainGenerator> value )
        {
            m_terrainGenerator = value;
        }

        SmartPtr<IProceduralCollision> CProceduralManager::getCollisionManager() const
        {
            return m_collisionManager;
        }

        void CProceduralManager::setCollisionManager( SmartPtr<IProceduralCollision> value )
        {
            m_collisionManager = value;
        }

        const SmartPtr<IAi> &CProceduralManager::getAi() const
        {
            return m_ai;
        }

        SmartPtr<IAi> &CProceduralManager::getAi()
        {
            return m_ai;
        }
    }  // namespace procedural
}  // namespace workphone
