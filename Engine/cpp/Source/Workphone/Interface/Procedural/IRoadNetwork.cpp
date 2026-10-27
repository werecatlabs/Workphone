#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/IRoadNetwork.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <stdexcept>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone, IRoadNetwork, ISharedObject );

    IRoadNetwork::~IRoadNetwork() = default;

    void IRoadNetwork::appendGenerated( const Array<SmartPtr<IRoad>> &,
                                       const Array<SmartPtr<IRoadConnection>> & )
    {
        throw std::logic_error( "This road network does not support atomic generated publication" );
    }

}  // namespace workphone::procedural
