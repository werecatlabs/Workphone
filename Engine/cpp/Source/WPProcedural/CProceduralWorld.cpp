#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/CProceduralWorld.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace procedural
    {
        CProceduralWorld::CProceduralWorld() = default;

        CProceduralWorld::~CProceduralWorld() = default;

        void CProceduralWorld::addScene( SmartPtr<IProceduralScene> scene )
        {
            m_scenes.push_back( scene );
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
            return m_scenes;
        }
    }  // namespace procedural
}  // namespace workphone
