#ifndef TextureState_h__
#define TextureState_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>

namespace workphone
{
    class WPCore_API TextureStateData : public StateData
    {
    public:
        TextureStateData();
        ~TextureStateData() override;

        WP_CLASS_REGISTER_DECL;

        Vector2I size = Vector2I( 1024, 1024 );
        Vector2I actualSize = Vector2I( 1024, 1024 );
    };
}  // namespace workphone

#endif  // TextureState_h__
