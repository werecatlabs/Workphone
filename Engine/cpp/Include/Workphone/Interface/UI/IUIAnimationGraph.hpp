#ifndef IUIAnimationGraph_h__
#define IUIAnimationGraph_h__

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {

        class IUIAnimationGraph
        {
        public:
            IUIAnimationGraph();

            IUIAnimationGraph( u32 poolTypeId );

            ~IUIAnimationGraph();

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // IUIAnimationGraph_h__
