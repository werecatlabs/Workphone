#ifndef _CCompositor_H_
#define _CCompositor_H_

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace render
    {
        class CCompositor : public ISharedObject
        {
        public:
            CCompositor();
            ~CCompositor() override;

            /** */
            void setEnabled( bool isEnabled );

            /** */
            bool isEnabled() const;

        protected:
            bool m_isEnabled;
        };
    }  // end namespace render
}  // namespace workphone

#endif
