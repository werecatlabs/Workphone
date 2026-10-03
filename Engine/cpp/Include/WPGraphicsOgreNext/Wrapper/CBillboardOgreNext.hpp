#ifndef _CBillboard_H
#define _CBillboard_H

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Interface/Graphics/IBillboard.hpp>

namespace workphone
{
    namespace render
    {
        class CBillboardOgreNext : public IBillboard
        {
        public:
            CBillboardOgreNext();
            ~CBillboardOgreNext() override;

            void initialise( Ogre::v1::Billboard *bb );

            void setPosition( const Vector3F &position ) override;
            Vector3F getPosition() const override;

            void setDimensions( const Vector2F &dimensions );
            Vector2F getDimensions() const;

            void setColour( const ColourF &colour ) override;
            ColourF getColour() const override;

            void _getObject( void **ppObject ) const override;

            static bool IsFree( void *element );

            void setOrientation( const QuaternionF &orientation ) override;

            QuaternionF getOrientation() const override;

            void setScale( const Vector3F &dimensions ) override;

            Vector3F getScale() const override;

            void *_getRenderSystemTransform() const override;

            void *getRenderData() const override;

            void setRenderData( void *renderData ) override;

        protected:
            Ogre::v1::Billboard *m_bb;
            Vector3F m_position;
            Vector3F m_scale = Vector3F::zero();
            QuaternionF m_orientation = QuaternionF::identity();
        };
    }  // namespace render
}  // namespace workphone

#endif
