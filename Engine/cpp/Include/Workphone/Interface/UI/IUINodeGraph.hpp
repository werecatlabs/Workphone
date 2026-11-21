#ifndef IUINodeGraph_h__
#define IUINodeGraph_h__

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {
        class WPCore_API IUINodeGraph : public IUIElement
        {
        public:
            IUINodeGraph();

            IUINodeGraph( u32 poolTypeId );

            ~IUINodeGraph() override;

            /** Sets the name of the node graph used. */
            virtual void setNodeGraphName( const String &nodeGraphName ) = 0;

            /** Gets the name of the node graph used. */
            virtual String getNodeGraphName() const = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace ui
}  // namespace workphone

#endif  // IUINodeGraph_h__
