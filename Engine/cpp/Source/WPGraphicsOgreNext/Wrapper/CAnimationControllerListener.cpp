#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CAnimationControllerListener.hpp>

namespace workphone::render
{

    CAnimationControllerListener::CAnimationControllerListener() = default;

    CAnimationControllerListener::~CAnimationControllerListener() = default;

    void CAnimationControllerListener::update( const s32 &task, const time_interval &t,
                                               const time_interval &dt )
    {
    }

    void CAnimationControllerListener::handleAnimationEnd( const String &name )
    {
    }

}  // namespace workphone::render
