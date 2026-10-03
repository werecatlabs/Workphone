#ifndef CAnimationControllerListener_h__
#define CAnimationControllerListener_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
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

            void update() override;

            void handleAnimationEnd( const String &name ) override;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // CAnimationControllerListener_h__
