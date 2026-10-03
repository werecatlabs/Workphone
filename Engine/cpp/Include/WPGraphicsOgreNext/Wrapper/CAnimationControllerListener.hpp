#ifndef CAnimationControllerListener_h__
#define CAnimationControllerListener_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Interface/Graphics/IAnimationControllerListener.hpp>

namespace workphone
{
    namespace render
    {
        class CAnimationControllerListener : public IAnimationControllerListener
        {
        public:
            CAnimationControllerListener();
            ~CAnimationControllerListener() override;

            void update( const s32 &task, const time_interval &t, const time_interval &dt );

            void handleAnimationEnd( const String &name ) override;
        };
    }  // namespace render
}  // namespace workphone

#endif  // CAnimationControllerListener_h__
