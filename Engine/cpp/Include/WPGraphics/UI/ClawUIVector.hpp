#ifndef ClawUIVector_h__
#define ClawUIVector_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/UI/IUIVector3.hpp>
#include <WPGraphics/UI/ClawUIElement.hpp>

namespace workphone
{
    namespace ui
    {
        class ClawUIVector : public ClawUIElement<IUIVector3>
        {
        public:
            ClawUIVector();
            ~ClawUIVector() override;

            String getFileName() const;
            void setFileName( const String &fileName );

            SmartPtr<render::IMaterial> getMaterial() const;
            void setMaterial( SmartPtr<render::IMaterial> material );

            Vector3<real_Num> getValue() const override;
            void setValue( const Vector3<real_Num> &value ) override;

            void draw( struct wp_context *ctx ) override;

        protected:
            String m_fileName;
            SmartPtr<render::IMaterial> m_material;
            Vector3<real_Num> m_value = Vector3<real_Num>::zero();
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // ClawUIVector_h__
