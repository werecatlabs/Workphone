#ifndef _CBillboardSet_H
#define _CBillboardSet_H

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Graphics/IBillboardSet.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <WPGraphicsOgre/Wrapper/CGraphicsObjectOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CBillboardOgre.hpp>
#include <Workphone/Core/Pool.hpp>
#include <vector>
#include <list>

namespace workphone
{
    namespace render
    {

        /**
         * @file CBillboardSetOgre.hpp
         * @brief Ogre wrapper for the engine billboard set abstraction.
         *
         * This header defines `CBillboardSetOgre`, a renderer-specific
         * implementation of the engine `IBillboardSet` interface which
         * manages a collection of billboards backed by an Ogre::BillboardSet.
         */

        /**
         * @class CBillboardSetOgre
         * @brief Wrapper that exposes an Ogre::BillboardSet to the engine.
         *
         * This class implements the engine's `IBillboardSet` contract
         * and provides lifecycle hooks, material and rendering control,
         * and management for individual `CBillboardOgre` instances.
         *
         * The wrapper does not implicitly manage the lifetime of the native
         * Ogre billboard set unless explicitly initialised with one via
         * `initialise()`. Loading/unloading is handled through `load` and
         * `unload` engine lifecycle calls.
         */
        class CBillboardSetOgre : public CGraphicsObjectOgre<IBillboardSet>
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Constructs an empty wrapper. Call `initialise` to attach a native
             * Ogre::BillboardSet, or use `load` if the owning scene will supply
             * resources later.
             */
            CBillboardSetOgre();

            /**
             * @brief Construct and associate with a creator scene.
             * @param creator Smart pointer to the scene that creates/manages this object.
             */
            CBillboardSetOgre( SmartPtr<IGraphicsScene> creator );

            /**
             * @brief Destructor.
             *
             * Does not assume ownership of the underlying Ogre::BillboardSet
             * unless the owning scene/manager documents that behaviour.
             * Use `unload` to release engine resources explicitly.
             */
            ~CBillboardSetOgre() override;

            /**
             * @brief Called by the engine to load resources for this object.
             * @param data Optional shared data provided by the loader.
             *
             * Typical implementations acquire or register renderer resources here.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Called by the engine to unload resources for this object.
             * @param data Optional shared data provided by the loader.
             *
             * Typical implementations release or unregister renderer resources here.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Attach a native Ogre::BillboardSet to this wrapper.
             * @param bbSet Pointer to an existing Ogre::BillboardSet instance.
             *
             * After calling `initialise`, operations on this wrapper will be
             * forwarded to the provided native object. The caller retains
             * responsibility for the native object's lifetime unless another
             * manager takes ownership.
             */
            void initialise( Ogre::BillboardSet *bbSet );

            // IGraphicsObject functions

            /**
             * @brief Detach this graphics object from its parent scene node.
             *
             * If attached to a scene node, detaches and ensures the native
             * object is removed from the parent.
             */
            void detachFromParent();

            /**
             * @brief Attach this graphics object to a parent scene node.
             * @param parent Smart pointer to the parent IGraphicsSceneNode.
             */
            void attachToParent( SmartPtr<IGraphicsSceneNode> parent );

            /**
             * @brief Set the material name used by this billboard set.
             * @param materialName Material resource name.
             * @param index Optional sub-index for multi-material billboard sets (-1 = all).
             */
            void setMaterialName( const String &materialName, s32 index = -1 ) override;

            /**
             * @brief Get the material name used by this billboard set.
             * @param index Optional sub-index for multi-material billboard sets (-1 = primary).
             * @return Material name string.
             */
            String getMaterialName( s32 index = -1 ) const override;

            /**
             * @brief Enable or disable casting shadows for this billboard set.
             * @param castShadows True to enable casting shadows.
             */
            void setCastShadows( bool castShadows ) override;

            /**
             * @brief Query whether this billboard set casts shadows.
             * @return True if casting shadows is enabled.
             */
            bool getCastShadows() const override;

            /**
             * @brief Enable or disable receiving shadows for this billboard set.
             * @param recieveShadows True to receive shadows.
             */
            void setReceiveShadows( bool recieveShadows ) override;

            /**
             * @brief Query whether this billboard set receives shadows.
             * @return True if receiving shadows is enabled.
             */
            bool getReceiveShadows() const override;

            /**
             * @brief Set overall visibility for this billboard set.
             * @param isVisible True to make the set visible.
             */
            void setVisible( bool isVisible ) override;

            /**
             * @brief Query overall visibility.
             * @return True if the set is visible.
             */
            bool isVisible() const override;

            /**
             * @brief Set the render queue group index used by Ogre for sorting.
             * @param renderQueue Render queue group (0-255).
             */
            void setRenderQueueGroup( u8 renderQueue );

            /**
             * @brief Set visibility bitmask flags for this object.
             * @param flags Bitmask used by the scene for culling/visibility.
             */
            void setVisibilityFlags( u32 flags ) override;

            /**
             * @brief Get the visibility bitmask flags.
             * @return Current visibility flags.
             */
            u32 getVisibilityFlags() const override;

            /**
             * @brief Create a shallow copy of this graphics object.
             * @param name Optional name for the cloned object.
             * @return Smart pointer to the cloned IGraphicsObject.
             *
             * The clone shares native resources where appropriate; the exact
             * cloning semantics depend on renderer implementation.
             */
            SmartPtr<IGraphicsObject> clone(
                const String &name = StringUtil::EmptyString ) const override;

            /**
             * @brief Retrieve the underlying native renderer object.
             * @param ppObject Output pointer location that receives the native pointer.
             *
             * If non-null, writes a raw pointer (e.g. `Ogre::BillboardSet*`) into `ppObject`.
             */
            void _getObject( void **ppObject ) const override;

            // IBillboardSet functions

            /**
             * @brief Remove all billboards from the set.
             *
             * Clears both the native billboard set and the wrapper's bookkeeping.
             */
            void clear() override;

            /**
             * @brief Create and return a new billboard wrapper.
             * @param position Optional initial world position (defaults to zero).
             * @return Smart pointer to the newly created IBillboard.
             *
             * The returned billboard is managed by this set's pool/list and
             * is attached to the native set if `initialise` was called.
             */
            SmartPtr<IBillboard> createBillboard( const Vector3F &position = Vector3F::zero() ) override;

            /**
             * @brief Remove a previously created billboard from the set.
             * @param billboard Smart pointer to the billboard to remove.
             * @return True if removal succeeded.
             */
            bool removeBillboard( SmartPtr<IBillboard> billboard ) override;

            /**
             * @brief Configure per-billboard frustum culling behaviour.
             * @param cullIndividually True to cull billboards individually.
             */
            void setCullIndividually( bool cullIndividually ) override;

            /**
             * @brief Enable or disable sorting for correct rendering order.
             * @param sortingEnabled True to enable sorting.
             */
            void setSortingEnabled( bool sortingEnabled ) override;

            /**
             * @brief Choose whether to use accurate facing (more CPU) for billboards.
             * @param useAccurateFacing True to use accurate facing when supported.
             */
            void setUseAccurateFacing( bool useAccurateFacing ) override;

            /**
             * @brief Provide explicit bounds for the billboard set.
             * @param box Axis-aligned bounding box in local space.
             * @param radius Bounding sphere radius (optional, typically used by Ogre).
             *
             * This can be used to override automatic bounds when billboards are
             * updated dynamically in a way that the renderer cannot automatically detect.
             */
            void setBounds( const AABB3F &box, f32 radius ) override;

            /**
             * @brief Set default 2D dimensions (width, height) for new billboards.
             * @param dimension 2D vector containing width and height.
             *
             * Overload that accepts Vector2F; other overload accepts Vector3F.
             */
            void setDefaultDimensions( const Vector2F &dimension );

            /**
             * @brief Set default 3D dimensions (used as scale) for new billboards.
             * @param dimension 3D vector containing size/scale.
             */
            void setDefaultDimensions( const Vector3F &dimension ) override;

        protected:
            /**
             * @brief Expand a typed object pool by creating new wrapper instances.
             * @tparam T Pool element type (expected SmartPtr<CBillboardOgre>).
             * @param pool Reference to the pool to expand.
             * @param numNewElements Number of elements to add.
             *
             * The function constructs new wrapper objects and places them into `pool`.
             * Access to the pool should be externally synchronised as required.
             */
            template <class T>
            void expandPool( Pool<SmartPtr<CBillboardOgre>> &pool, u32 numNewElements );

            /**
             * @brief Return the current set of billboards as an Array.
             * @return Array of smart pointers to IBillboard representing active billboards.
             *
             * The returned collection is a snapshot; modifications to the native
             * set after the call will not be reflected in the returned Array.
             */
            Array<SmartPtr<IBillboard>> getBillboards() const override;

            /**
             * @brief Internal event handler.
             * @param event Event object to process.
             *
             * Used to receive and respond to engine or scene events (e.g. visibility changes).
             */
            void handleEvent( SmartPtr<IEvent> event );

            /**
             * @brief Set the Z-order (render priority) for this billboard set.
             * @param zOrder Integer Z-order value.
             */
            void setZOrder( u32 zOrder ) override;

            /**
             * @brief Get the Z-order (render priority) for this billboard set.
             * @return Current Z-order value.
             */
            u32 getZOrder() const override;

            /**
             * @brief Get local axis-aligned bounding box for this set.
             * @return AABB in local space.
             */
            AABB3F getLocalAABB() const override;

            /**
             * @brief Set the local axis-aligned bounding box for this set.
             * @param localAABB New local AABB value.
             */
            void setLocalAABB( const AABB3F &localAABB ) override;

            /// Pointer to the native Ogre billboard set. May be nullptr if not initialised.
            Ogre::BillboardSet *m_bbSet = nullptr;

            /// Scene that created or owns this billboard set (may be null).
            SmartPtr<IGraphicsScene> m_creator;

            /// Container of wrapper objects that represent individual billboards.
            using BillboardList = std::list<SmartPtr<CBillboardOgre>>;
            BillboardList m_bbs;

            /// Pool used to recycle `CBillboardOgre` instances for allocation efficiency.
            Pool<SmartPtr<CBillboardOgre>> m_billboardPool;

            /// Mutex to guard access to billboard set data structures. Marked mutable
            /// to allow const getters to obtain read locks when necessary.
            mutable SpinRWMutex BBSetMutex;
        };
    }  // namespace render
}  // namespace workphone

#endif
