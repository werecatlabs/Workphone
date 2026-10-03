#ifndef _COverlayElementVector_H
#define _COverlayElementVector_H

#include <WPGraphics/ClawOverlayElement.hpp>
#include <Workphone/Interface/Graphics/IOverlayElementVector.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @class COverlayElementVector
         * @brief Base implementation of a vector graphic overlay element.
         */
        class ClawOverlayElementVector : public ClawOverlayElement<IOverlayElementVector>
        {
        public:
            ClawOverlayElementVector();
            ~ClawOverlayElementVector() override;

            String getFileName() const override;
            void setFileName( const String &fileName ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            String m_fileName;
        };
    }  // end namespace render
}  // namespace workphone

#endif
