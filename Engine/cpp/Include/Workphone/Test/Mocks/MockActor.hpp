#ifndef MockModelAircraft_h__
#define MockModelAircraft_h__

#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    class MockActor : public scene::IGameActor
    {
    public:
        MockActor();
        ~MockActor() override;
    };

}  // namespace workphone

#endif  // MockModelAircraft_h__
