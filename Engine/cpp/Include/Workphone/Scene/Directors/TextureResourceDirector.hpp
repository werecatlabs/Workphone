#ifndef TextureResourceDirector_h__
#define TextureResourceDirector_h__

#include <Workphone/Scene/Directors/ResourceDirector.hpp>

namespace workphone
{
    namespace scene
    {

        /** Texture resource director implementation. */
        class WPCore_API TextureResourceDirector : public ResourceDirector
        {
        public:
            /** Constructor. */
            TextureResourceDirector();

            /** Destructor. */
            ~TextureResourceDirector() override;

            /** @copydoc IBuildDirector::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc IBuildDirector::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** Gets if the texture is using tiling. */
            bool getUseTiling() const;

            /** Sets if the texture is using tiling. */
            void setUseTiling( bool useTiling );

            /** Get the boarder left. */
            s32 getBorderLeft() const;

            /** Set the boarder left. */
            void setBorderLeft( s32 borderLeft );

            /** Get the boarder right. */
            s32 getBorderRight() const;

            /** Set the boarder right. */
            void setBorderRight( s32 borderRight );

            /** Get the boarder top. */
            s32 getBorderTop() const;

            /** Set the boarder top. */
            void setBorderTop( s32 borderTop );

            /** Get the boarder bottom. */
            s32 getBorderBottom() const;

            /** Set the boarder bottom. */
            void setBorderBottom( s32 borderBottom );

            WP_CLASS_REGISTER_DECL;

        protected:
            // The texture type.
            String m_textureType;

            // The size of the texture.
            s32 m_textureSize = 8192;

            // The left border.
            s32 m_borderLeft = 0;

            // The right border.
            s32 m_borderRight = 0;

            // The top border.
            s32 m_borderTop = 0;

            // The bottom border.
            s32 m_borderBottom = 0;

            // If the texture is using tiling.
            bool m_useTiling = false;
        };

    }  // namespace scene
}  // namespace workphone

#endif  // TextureResourceDirector_h__
