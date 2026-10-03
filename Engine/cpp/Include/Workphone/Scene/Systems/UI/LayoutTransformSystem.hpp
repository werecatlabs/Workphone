#ifndef CanvasTransformSystem_h__
#define CanvasTransformSystem_h__

#include <Workphone/Scene/Systems/ComponentSystem.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Core/Pool.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/State/States/UIAnchorStateData.hpp>
#include <Workphone/State/States/UITransformStateData.hpp>
#include <Workphone/State/States/UILayoutStateData.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @brief Component system responsible for computing UI layout transforms.
         *
         * The LayoutTransformSystem maintains state for UI layout, transform and
         * anchor components and computes element positions and sizes based on
         * parent metrics, anchor rules and local transform data. It supports
         * incremental (dirty list) updates as well as a brute-force update path.
         *
         * This system is intended to be used by the UI pipeline to resolve the
         * final screen-space position and size of UI elements before rendering
         * or hit-testing.
         */
        class WPCore_API LayoutTransformSystem : public ComponentSystem
        {
        public:
            /**
             * @brief Construct a new LayoutTransformSystem.
             */
            LayoutTransformSystem();

            /**
             * @brief Destroy the LayoutTransformSystem.
             */
            ~LayoutTransformSystem() override;

            /**
             * @copydoc ComponentSystem::load
             *
             * Expects optional shared data used to configure the system. Any
             * system-specific deserialization should be handled here.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc ComponentSystem::unload
             *
             * Releases resources owned by the system and clears internal
             * pools and lists.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc ComponentSystem::update
             *
             * Executes layout updates. This will either process the dirty
             * list incrementally or perform a full update depending on the
             * internal strategy.
             */
            void update() override;

            /**
             * @copydoc ComponentSystem::addComponent
             *
             * Adds a component to the system and allocates any required state
             * objects.
             *
             * @return index or id of the created component data within the
             *         system.
             */
            u32 addComponent( SmartPtr<IComponent> component ) override;

            /**
             * @copydoc ComponentSystem::removeComponent
             *
             * Removes a component and frees associated state entries.
             */
            void removeComponent( SmartPtr<IComponent> component ) override;

            /**
             * @brief Mark a component as dirty so it will be updated on the
             * next update pass.
             *
             * @param component Component to mark dirty.
             */
            void addDirtyComponent( SmartPtr<IComponent> component ) override;

            /**
             * @copydoc ComponentSystem::handleEvent
             *
             * Handles system-level events relevant to layout (for example
             * resolution changes, layout invalidation, etc.).
             */
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

        protected:
            /**
             * @brief Update only dirty components using an optimized path.
             *
             * This method walks the dirty list and updates elements that have
             * been marked as needing layout recalculation.
             */
            void updateSmart();

            /**
             * @brief Update all registered components using a brute-force
             *        approach.
             *
             * This forces layout calculation for every component regardless
             * of its dirty state. Useful for full-layout passes.
             */
            void updateBruteForce();

            /**
             * @brief Calculate the final position of an element within its
             *        parent.
             *
             * This function resolves anchor rules together with the local
             * position and the parent's position/size to produce the final
             * element position in parent space.
             *
             * @param parentPosition Position of the parent element.
             * @param parentSize Size of the parent element.
             * @param position Local position/offset of the element.
             * @param size Local size of the element.
             * @param anchor Normalized anchor point of the element.
             * @param anchorMin Normalized minimum anchor (for stretch behavior).
             * @param anchorMax Normalized maximum anchor (for stretch behavior).
             * @return Resolved element position in parent space.
             */
            Vector2<real_Num> calculateElementPosition( const Vector2<real_Num> &parentPosition,
                                                        const Vector2<real_Num> &parentSize,
                                                        const Vector2<real_Num> &position,
                                                        const Vector2<real_Num> &size,
                                                        const Vector2<real_Num> &anchor,
                                                        const Vector2<real_Num> &anchorMin,
                                                        const Vector2<real_Num> &anchorMax );

            /**
             * @brief Calculate both the position and size of an element given
             *        its parent metrics and anchor settings.
             *
             * @param parentPosition Parent element position.
             * @param parentSize Parent element size.
             * @param position Local position/offset.
             * @param size Local size.
             * @param anchor Element anchor.
             * @param anchorMin Minimum anchor for stretch.
             * @param anchorMax Maximum anchor for stretch.
             * @param[out] elementPosition Resolved element position.
             * @param[out] elementSize Resolved element size.
             */
            void calculateElementPositionAndSize(
                const Vector2<real_Num> &parentPosition, const Vector2<real_Num> &parentSize,
                const Vector2<real_Num> &position, const Vector2<real_Num> &size,
                const Vector2<real_Num> &anchor, const Vector2<real_Num> &anchorMin,
                const Vector2<real_Num> &anchorMax, Vector2<real_Num> &elementPosition,
                Vector2<real_Num> &elementSize );

            /**
             * @brief Calculate element position and size when only local
             *        metrics are available (no parent metrics required).
             *
             * Useful for root elements or when the element's layout is
             * independent of its parent.
             */
            void calculateElementPositionAndSize( const Vector2<real_Num> &position,
                                                  const Vector2<real_Num> &size,
                                                  const Vector2<real_Num> &anchor,
                                                  const Vector2<real_Num> &anchorMin,
                                                  const Vector2<real_Num> &anchorMax,
                                                  Vector2<real_Num> &elementPosition,
                                                  Vector2<real_Num> &elementSize );

            /**
             * @brief Calculate edge offsets (left, right, top, bottom) of the
             *        element within its parent based on anchor and size.
             *
             * This populates numeric edge values rather than Vector2 types and
             * can be used by systems that need explicit edge offsets for
             * collision, clipping or layout constraints.
             */
            void calculateElementPositionAndSize(
                const Vector2<real_Num> &parentPosition, const Vector2<real_Num> &parentSize,
                const Vector2<real_Num> &position, const Vector2<real_Num> &size,
                const Vector2<real_Num> &anchor, const Vector2<real_Num> &anchorMin,
                const Vector2<real_Num> &anchorMax, f32 &left, f32 &right, f32 &top, f32 &bottom );

            /**
             * @brief Rebuilds the internal dirty list of components that need
             *        layout recalculation.
             */
            void updateDirtyList();

            /**
             * @brief Internal per-component data stored by the system.
             *
             * Stores raw pointers to component and its state objects managed
             * by the pools. These are not owning pointers; lifetime is
             * controlled by the system pools.
             */
            struct LayoutTransformData
            {
                IComponent *component = nullptr; /**< Owning component pointer. */
                u32 slot =
                    std::numeric_limits<u32>::max(); /**< Stable slot owned by the component system. */
                UILayoutStateData *layoutState = nullptr;       /**< Pointer to layout state. */
                UITransformStateData *transformState = nullptr; /**< Pointer to transform state. */
                UIAnchorStateData *anchorState = nullptr;       /**< Pointer to anchor state. */
            };

            Pool<LayoutTransformData>
                m_layoutTransformDataPool; /**< Pool of per-component layout data. */

            Pool<UILayoutStateData> m_layoutPool; /**< Pool containing UILayoutState instances. */

            Pool<UITransformStateData>
                m_transformPool; /**< Pool containing UITransformState instances. */

            Pool<UIAnchorStateData> m_anchorPool; /**< Pool containing UIAnchorState instances. */

            /**
             * @brief A list of pointers to components that are dirty and need
             *        layout updates.
             *
             * The list contains raw component pointers and is maintained by
             * the system; entries are valid as long as their corresponding
             * data remains allocated in the pools.
             */
            Array<IComponent *> m_componentPointers;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // CanvasTransformSystem_h__
