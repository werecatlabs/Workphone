#ifndef ClawBillboardSet_h__
#define ClawBillboardSet_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Graphics/BillboardSet.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @class ClawBillboardSet
         * @brief Implementation of a billboard set for the Claw graphics engine.
         *
         * A billboard set manages a collection of 2D quads (billboards) that always
         * face the camera, typically used for particles, foliage, or simple 2D effects
         * in a 3D scene.
         */
        class WPGraphics_API ClawBillboardSet : public BillboardSet
        {
        public:
            /**
             * @brief Constructs a new ClawBillboardSet instance.
             */
            ClawBillboardSet();

            /**
             * @brief Destroys the ClawBillboardSet instance.
             */
            ~ClawBillboardSet() override;

            /**
             * @brief Removes all billboards from the set.
             */
            void clear() override;

            /**
             * @brief Creates a new billboard at the specified position.
             * @param position The 3D position where the billboard should be placed.
             * @return A SmartPtr to the created IBillboard.
             */
            SmartPtr<IBillboard> createBillboard( const Vector3<real_Num> &position ) override;

            /**
             * @brief Removes a specific billboard from the set.
             * @param billboard The billboard to remove.
             * @return True if the billboard was found and removed, false otherwise.
             */
            bool removeBillboard( SmartPtr<IBillboard> billboard ) override;

            /**
             * @brief Retrieves all billboards currently managed by the set.
             * @return An Array containing SmartPtrs to the billboards.
             */
            Array<SmartPtr<IBillboard>> getBillboards() const override;

            /**
             * @brief Sets whether each billboard should be culled individually.
             * @param cullIndividually If true, each billboard is tested for visibility; if false, the whole set is tested.
             */
            void setCullIndividually( bool cullIndividually ) override;

            /**
             * @brief Enables or disables sorting of billboards (typically for transparency).
             * @param sortingEnabled True to enable depth sorting.
             */
            void setSortingEnabled( bool sortingEnabled ) override;

            /**
             * @brief Sets whether billboards use accurate facing calculations.
             * @param useAccurateFacing True for high-precision facing, false for faster approximations.
             */
            void setUseAccurateFacing( bool useAccurateFacing ) override;

            /**
             * @brief Defines the bounding volume for the billboard set.
             * @param box The Axis-Aligned Bounding Box (AABB) for the set.
             * @param radius An optional radius for spherical bounding.
             */
            void setBounds( const AABB3<real_Num> &box, real_Num radius ) override;

            /**
             * @brief Sets the default size for billboards created in this set.
             * @param dimension The Vector3 representing width, height, and depth.
             */
            void setDefaultDimensions( const Vector3<real_Num> &dimension ) override;

            /**
             * @brief Sets the material for the billboard set by name.
             * @param materialName The name of the material to use.
             * @param index Optional index for multi-material sets. Defaults to -1.
             */
            void setMaterialName( const String &materialName, s32 index = -1 ) override;

            /**
             * @brief Retrieves the material name for the billboard set.
             * @param index Optional index for multi-material sets. Defaults to -1.
             * @return The name of the material.
             */
            String getMaterialName( s32 index = -1 ) const override;

            /**
             * @brief Sets the material for the billboard set using a material object.
             * @param material A SmartPtr to the IMaterial to use.
             * @param index Optional index for multi-material sets. Defaults to -1.
             */
            void setMaterial( SmartPtr<IMaterial> material, s32 index = -1 ) override;

            /**
             * @brief Retrieves the material object for the billboard set.
             * @param index Optional index for multi-material sets. Defaults to -1.
             * @return A SmartPtr to the material.
             */
            SmartPtr<IMaterial> getMaterial( s32 index = -1 ) const override;

            /**
             * @brief Internal method to retrieve the base object pointer.
             * @param ppObject Pointer to receive the object address.
             */
            void _getObject( void **ppObject ) const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /** @brief The list of managed billboards. */
            Array<SmartPtr<IBillboard>> m_billboards;

            /** @brief The default dimensions applied to new billboards. */
            Vector3<real_Num> m_defaultDimensions = Vector3<real_Num>( 1, 1, 1 );

            /** @brief The identifier of the assigned material. */
            String m_materialName;

            /** @brief The assigned material object. */
            SmartPtr<IMaterial> m_material;
        };
    }  // namespace render
}  // namespace workphone

#endif  // ClawBillboardSet_h__
