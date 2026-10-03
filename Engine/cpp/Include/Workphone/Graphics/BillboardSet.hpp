#ifndef __WP_BillboardSet_h__
#define __WP_BillboardSet_h__

#include <Workphone/Graphics/GraphicsObject.hpp>
#include <Workphone/Interface/Graphics/IBillboardSet.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @file BillboardSet.hpp
         * @brief Declaration of the BillboardSet class — a container for renderable billboards.
         *
         * The BillboardSet manages a collection of billboards, their materials and
         * rendering / culling behaviour. Instances are graphics objects that can be
         * cloned and queried for the underlying native rendering object via _getObject.
         */

        /**
         * @class BillboardSet
         * @brief Represents a collection of screen-aligned or camera-facing sprites.
         *
         * A BillboardSet stores multiple @c IBillboard instances and associated material
         * information. It provides operations to create and remove billboards, control
         * per-billboard culling, sorting and facing behaviour, and to set bounding
         * information used by the renderer for visibility tests.
         */
        class WPCore_API BillboardSet : public GraphicsObject<IBillboardSet>
        {
        public:
            /**
             * @brief Construct a new BillboardSet.
             *
             * Initializes an empty set with default parameters (no material, zero bounds,
             * culling and sorting disabled).
             */
            BillboardSet();

            /**
             * @brief Destroy the BillboardSet.
             *
             * Releases contained billboards and other resources. Overrides base destructor.
             */
            ~BillboardSet() override;

            /**
             * @brief Remove all billboards and reset internal state to defaults.
             *
             * This does not unregister the object itself; it clears the contained
             * billboard list and resets dimensions/bounds to defaults.
             */
            void clear() override;

            /**
             * @brief Create and add a new billboard to the set.
             * @param position Initial world-space position of the billboard. Defaults to (0,0,0).
             * @return SmartPtr<IBillboard> Handle to the newly created billboard.
             *
             * The returned smart pointer is also stored internally by the BillboardSet.
             */
            SmartPtr<IBillboard> createBillboard(
                const Vector3<real_Num> &position = Vector3<real_Num>::zero() ) override;

            /**
             * @brief Remove a billboard previously created by this set.
             * @param billboard Smart pointer to the billboard to remove.
             * @return true If the billboard was found and removed; false otherwise.
             *
             * If the same SmartPtr instance is not present in the internal list, removal fails.
             */
            bool removeBillboard( SmartPtr<IBillboard> billboard ) override;

            /**
             * @brief Retrieve the list of billboards owned by this set.
             * @return Array<SmartPtr<IBillboard>> Copy of the internal billboard list.
             *
             * The returned array contains smart pointers to the billboards; modifying
             * the returned array does not affect the internal storage.
             */
            Array<SmartPtr<IBillboard>> getBillboards() const override;

            /**
             * @brief Set whether each billboard should be culled individually.
             * @param cullIndividually When true, the renderer will perform per-billboard
             *        frustum/occlusion checks instead of using the set's bounds.
             */
            void setCullIndividually( bool cullIndividually ) override;

            /**
             * @brief Enable or disable sorting of billboards before rendering.
             * @param sortingEnabled When true, billboards will be sorted (typically by
             *        distance to camera) to ensure correct transparency blending.
             */
            void setSortingEnabled( bool sortingEnabled ) override;

            /**
             * @brief Control accurate facing for billboards.
             * @param useAccurateFacing When true, the billboard's orientation will be
             *        computed using more precise (and possibly more expensive) math so
             *        that it faces the camera exactly rather than approximating.
             */
            void setUseAccurateFacing( bool useAccurateFacing ) override;

            /**
             * @brief Set the world-space bounding box and bounding radius for this set.
             * @param box Axis-aligned bounding box enclosing the billboards.
             * @param radius Bounding sphere radius for coarse culling.
             *
             * These bounds are used by the renderer for visibility testing when
             * per-billboard culling is disabled.
             */
            void setBounds( const AABB3<real_Num> &box, real_Num radius ) override;

            /**
             * @brief Set the default dimensions for newly created billboards.
             * @param dimension Default width/height (and optional depth) applied to
             *        billboards created without explicit size information.
             */
            void setDefaultDimensions( const Vector3<real_Num> &dimension ) override;

            /**
             * @brief Assign a material name to one of the material slots.
             * @param materialName Name or resource path of the material.
             * @param index Material slot index. If -1, the operation targets the first/default slot.
             *
             * The name can be resolved later by the renderer or material manager to obtain
             * a concrete IMaterial instance.
             */
            void setMaterialName( const String &materialName, s32 index = -1 ) override;

            /**
             * @brief Get the material name assigned to the given slot.
             * @param index Material slot index. If -1, returns the first/default slot name.
             * @return String Material name or empty string if none assigned.
             */
            String getMaterialName( s32 index = -1 ) const override;

            /**
             * @brief Set a material instance for the specified slot.
             * @param material Shared pointer to an IMaterial.
             * @param index Material slot index. If -1, sets the first/default slot.
             */
            void setMaterial( SmartPtr<IMaterial> material, s32 index = -1 ) override;

            /**
             * @brief Get the material instance for the specified slot.
             * @param index Material slot index. If -1, returns the first/default slot material.
             * @return SmartPtr<IMaterial> Material instance or null if not set.
             */
            SmartPtr<IMaterial> getMaterial( s32 index = -1 ) const override;

            /**
             * @brief Create a runtime clone of this graphics object.
             * @param name Optional name for the cloned object. Defaults to an empty string.
             * @return SmartPtr<IGraphicsObject> Clone of this object.
             *
             * The clone is a shallow copy of the configuration and contains new internal
             * containers; contained billboards are typically cloned or referenced depending
             * on implementation.
             */
            SmartPtr<IGraphicsObject> clone(
                const String &name = StringUtil::EmptyString ) const override;

            /**
             * @brief Retrieve the underlying native renderer object pointer.
             * @param ppObject Out parameter that will receive the raw pointer to the
             *        renderer-specific object (void*). May be null if not applicable.
             *
             * This is provided to allow integration with renderer APIs while keeping
             * the interface generic.
             */
            void _getObject( void **ppObject ) const override;

            WP_CLASS_REGISTER_DECL;

        private:
            /// List of billboards managed by this set.
            Array<SmartPtr<IBillboard>> m_billboards;

            /// Material instances used by the billboard set; one per slot.
            Array<SmartPtr<IMaterial>> m_materials;

            /// Names of materials (resource identifiers) corresponding to material slots.
            Array<String> m_materialNames;

            /// If true, perform frustum/occlusion culling per-billboard instead of using m_bounds.
            bool m_cullIndividually = false;

            /// When true, billboards are sorted before rendering (useful for correct transparency).
            bool m_sortingEnabled = false;

            /// When true, compute billboard facing with higher accuracy (more CPU cost).
            bool m_useAccurateFacing = false;

            /// Axis-aligned bounding box for coarse visibility tests.
            AABB3<real_Num> m_bounds;

            /// Bounding sphere radius used for coarse culling.
            f32 m_radius = 0.0f;

            /// Default width/height/depth applied to newly created billboards.
            Vector3<real_Num> m_defaultDimensions = Vector3<real_Num>::zero();
        };

    }  // namespace render
}  // namespace workphone

#endif  // BillboardSet_h__
