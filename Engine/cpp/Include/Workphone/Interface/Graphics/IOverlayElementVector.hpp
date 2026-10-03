#ifndef _IOverlayElementVector_H
#define _IOverlayElementVector_H

#include <Workphone/Interface/Graphics/IOverlayElement.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * An overlay element that displays a vector graphic.
         */
        class WPCore_API IOverlayElementVector : public IOverlayElement
        {
        public:
            /** Virtual destructor. */
            ~IOverlayElementVector() override;

            /** Gets the filename of the vector graphic. */
            virtual String getFileName() const = 0;

            /** Sets the filename of the vector graphic. */
            virtual void setFileName( const String &fileName ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif
