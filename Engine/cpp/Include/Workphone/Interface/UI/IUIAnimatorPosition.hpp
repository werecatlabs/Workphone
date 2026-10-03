#ifndef _IGUIAnimatorPosition_H
#define _IGUIAnimatorPosition_H

#include "Workphone/Interface/UI/IUIAnimator.hpp"

namespace workphone
{
    namespace ui
    {

        /** Animates a gui item.
         */
        class WPCore_API IUIAnimatorPosition : public IUIAnimator
        {
        public:
            /** Virtual destructor. */
            ~IUIAnimatorPosition() override = default;

            /** */
            virtual void setStart( const Vector2<real_Num> &position ) = 0;

            /** */
            virtual Vector2<real_Num> getStart() const = 0;

            /** */
            virtual void setEnd( const Vector2<real_Num> &position ) = 0;

            /** */
            virtual Vector2<real_Num> getEnd() const = 0;
        };

    }  // end namespace ui
}  // namespace workphone

#endif
