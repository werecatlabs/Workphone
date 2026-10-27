#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/CProceduralWorld.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace procedural
    {
        CProceduralWorld::CProceduralWorld() = default;

        CProceduralWorld::~CProceduralWorld()
        {
            unload( nullptr );
        }

        void CProceduralWorld::unload( SmartPtr<ISharedObject> )
        {
            // Detach before final-owner destruction can reenter the world. Scenes
            // may still belong to a builder, another world, or a caller snapshot.
            auto scenes = std::move( m_scenes );
        }

        void CProceduralWorld::addScene( SmartPtr<IProceduralScene> scene )
        {
            m_scenes.push_back( scene );
        }

        void CProceduralWorld::setScenes( const Array<SmartPtr<IProceduralScene>> &scenes )
        {
            auto candidate = makeSourceBudgetSnapshot( scenes );
            m_scenes.swap( candidate );
        }

        void CProceduralWorld::removeScene( SmartPtr<IProceduralScene> scene )
        {
            auto it = std::find( m_scenes.begin(), m_scenes.end(), scene );
            if( it != m_scenes.end() )
            {
                m_scenes.erase( it );
            }
        }

        Array<SmartPtr<IProceduralScene>> CProceduralWorld::getScenes() const
        {
            return makeSourceBudgetSnapshot( m_scenes );
        }
    }  // namespace procedural
}  // namespace workphone
