#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/IProceduralCity.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

#include <stdexcept>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, IProceduralCity, ISharedObject );

    IProceduralCity::~IProceduralCity() = default;

}  // namespace workphone::procedural

void workphone::procedural::IProceduralCity::setBlocks( const Array<SmartPtr<ICityBlock>> & )
{
    throw std::logic_error( "This procedural city does not support atomic block replacement" );
}
