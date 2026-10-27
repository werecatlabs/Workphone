#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/IProceduralWorld.hpp>
#include <stdexcept>

namespace workphone::procedural
{
    void IProceduralWorld::setScenes( const Array<SmartPtr<IProceduralScene>> & )
    {
        throw std::logic_error( "This procedural world does not support atomic scene replacement" );
    }
}
