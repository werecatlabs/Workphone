#ifndef _IBillboardSet_H
#define _IBillboardSet_H

#include <Workphone/Interface/Graphics/IGraphicsObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Math/AABB3.hpp>

namespace workphone
{
    namespace render
    {

        /** Manages billboards. */
        class WPCore_API IBillboardSet : public IGraphicsObject
        {
        public:
            /** Destructor. */
            ~IBillboardSet() override;

            /** Removes all the billboards in this set. */
            virtual void clear() = 0;

            /** Create a billboard that belongs to this set. */
            virtual SmartPtr<IBillboard> createBillboard(
                const Vector3<real_Num> &position = Vector3<real_Num>::zero() ) = 0;

            /** Remove a billboard thats part of this set. */
            virtual bool removeBillboard( SmartPtr<IBillboard> billboard ) = 0;

            /** Gets a list of billboards. */
            virtual Array<SmartPtr<IBillboard>> getBillboards() const = 0;

            /** Sets a boolean to cull the billboards in this set individually. */
            virtual void setCullIndividually( bool cullIndividually ) = 0;

            /** Sets whether or not sorting is enabled. */
            virtual void setSortingEnabled( bool sortingEnabled ) = 0;

            /** Sets whether or not accurate facing is used. */
            virtual void setUseAccurateFacing( bool useAccurateFacing ) = 0;

            /** Sets the bounding of the billboard set. */
            virtual void setBounds( const AABB3<real_Num> &box, real_Num radius ) = 0;

            /** Sets the default dimensions of the billboards in this set. */
            virtual void setDefaultDimensions( const Vector3<real_Num> &dimension ) = 0;

            /** Sets the material name. */
            virtual void setMaterialName( const String &materialName, s32 index = -1 ) = 0;

            /** Gets the material name. */
            virtual String getMaterialName( s32 index = -1 ) const = 0;

            /** Sets the material to use. */
            virtual void setMaterial( SmartPtr<IMaterial> material, s32 index = -1 ) = 0;

            /** Gets the material name being used. */
            virtual SmartPtr<IMaterial> getMaterial( s32 index = -1 ) const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif
