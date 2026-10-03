#ifndef IProceduralTexture_h__
#define IProceduralTexture_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPCore_API IProceduralTexture : public ISharedObject
        {
        public:
            ~IProceduralTexture() override;

            /** Generate or regenerate the procedural texture. */
            virtual void generate() = 0;

            /** Set the size of the texture. */
            virtual void setSize( int width, int height ) = 0;
            virtual int getWidth() const = 0;
            virtual int getHeight() const = 0;

            /** Set or get the raw pixel data (row-major, RGBA). */
            virtual void setData( const Array<ColourF> &data ) = 0;
            virtual Array<ColourF> getData() const = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // IProceduralTexture_h__
