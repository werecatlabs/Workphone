#ifndef _IBillboard_H
#define _IBillboard_H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Core/ColourF.hpp>

namespace workphone
{
    namespace render
    {

        /** An interface for a billboard.
         */
        class WPCore_API IBillboard : public ISharedObject
        {
        public:
            /** Destructor. */
            ~IBillboard() override;

            /** Sets the position of the billboard. */
            virtual void setPosition( const Vector3<real_Num> &position ) = 0;

            /** Gets the position of the billboard. */
            virtual Vector3<real_Num> getPosition() const = 0;

            /** Sets the orientation of the billboard. */
            virtual void setOrientation( const Quaternion<real_Num> &orientation ) = 0;

            /** Gets the orientation of the billboard. */
            virtual Quaternion<real_Num> getOrientation() const = 0;

            /** Sets the dimensions of the billboard. */
            virtual void setScale( const Vector3<real_Num> &dimensions ) = 0;

            /** Gets the dimensions of the billboard. */
            virtual Vector3<real_Num> getScale() const = 0;

            /** Gets the transformation cached for the render system. */
            virtual void *_getRenderSystemTransform() const = 0;

            /** Sets the colour of this billboard.
             */
            virtual void setColour( const ColourF &colour ) = 0;

            /** Gets the colour of this billboard.
             */
            virtual ColourF getColour() const = 0;

            /** Gets a pointer to the render data associated with this billboard.
             */
            virtual void *getRenderData() const = 0;

            /**
             * Sets a pointer to the render data associated with this billboard.
             */
            virtual void setRenderData( void *renderData ) = 0;

            /** Gets a pointer to the underlying object. This is dependent on the graphics library used.
             */
            virtual void _getObject( void **ppObject ) const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif
