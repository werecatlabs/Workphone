#ifndef PhysicsShape2_h__
#define PhysicsShape2_h__

#include <Workphone/Interface/Physics/IPhysicsShape2.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @file PhysicsShape2.hpp
         * @brief 2D physics shape default stub implementation.
         *
         * This header provides a small, generic adapter that implements the
         * IPhysicsShape2 interface by forwarding to a template base type `T`.
         * The current implementation provides safe defaults / stubs for each
         * interface method and is intended to be specialized or used as a base
         * for concrete shape implementations.
         */

        /**
         * @brief Generic 2D physics shape adapter.
         *
         * PhysicsShape2<T> provides a default implementation of the
         * IPhysicsShape2 interface. It inherits from the template parameter `T`
         * and implements all virtual methods required by the interface with
         * conservative default behaviour. Concrete shapes should either:
         * - inherit from `T` that already implements the behaviour, or
         * - specialize/override the methods in derived classes.
         *
         * @tparam T Base type which should conform to IPhysicsShape2 contract.
         */
        template <class T>
        class PhysicsShape2 : public T
        {
        public:
            /**
             * @brief Construct a PhysicsShape2 instance.
             *
             * Default constructor performs no heavy work; derived shape classes
             * can initialize internal geometry here.
             */
            PhysicsShape2();

            /**
             * @brief Virtual destructor.
             *
             * Ensures correct cleanup in class hierarchies when used via
             * IPhysicsShape2 pointers.
             */
            ~PhysicsShape2() override;

            /**
             * @brief Get the bounding sphere of the shape.
             *
             * @return Sphere2<real_Num> Bounding sphere in local shape coordinates.
             */
            Sphere2<real_Num> getSphere() const override;

            /**
             * @brief Get the axis-aligned bounding box (AABB) of the shape.
             *
             * @return AABB2<real_Num> AABB in local shape coordinates.
             */
            AABB2<real_Num> getAABB() const override;

            /**
             * @brief Retrieve the vertex/point list that represents the shape.
             *
             * The provided array is filled with the shape points. Implementations
             * should append or set the container contents as appropriate.
             *
             * @param[out] points Container to receive shape points.
             */
            void getPoints( Array<Vector2<real_Num>> &points ) const override;

            /**
             * @brief Compute mass properties for the shape.
             *
             * Implementations should fill `massData` using the provided
             * material density.
             *
             * @param[out] massData Output mass properties (mass, centroid, inertia).
             * @param density Material density used for mass computation.
             */
            void computeMass( SmartPtr<IMassData2> massData, real_Num density ) const override;

            /**
             * @brief Retrieve an opaque pointer to the underlying engine object.
             *
             * Many physics backends expose a raw pointer to an internal object.
             * If available, set `*ppObject` to that pointer; otherwise set to
             * nullptr.
             *
             * @param[out] ppObject Receiving pointer location.
             */
            void _getObject( void **ppObject ) const override;

            /**
             * @brief Get numeric type identifier for the shape.
             *
             * The meaning of returned type values is backend-specific.
             *
             * @return u8 Numeric type id.
             */
            u8 getType() const override;

            /**
             * @brief Query whether the shape is attached to a body/scene.
             *
             * @return true if attached; false otherwise.
             */
            bool isAttached() const override;

            /**
             * @brief Enable or disable the shape.
             *
             * Disabled shapes typically do not participate in collision queries.
             *
             * @param enabled true to enable, false to disable.
             */
            void setEnabled( bool enabled ) override;

            /**
             * @brief Query whether the shape is enabled.
             *
             * @return true if enabled; false otherwise.
             */
            bool isEnabled() const override;

            /**
             * @brief Mark the shape as a trigger (overlap-only) or solid.
             *
             * Trigger shapes do not resolve collisions but still report overlaps.
             *
             * @param trigger true to set as trigger, false for solid collisions.
             */
            void setTrigger( bool trigger ) override;

            /**
             * @brief Query whether the shape is a trigger.
             *
             * @return true if trigger; false if solid.
             */
            bool isTrigger() const override;

            /**
             * @brief Associate a state context with the shape.
             *
             * A state context is used to store and manipulate state machine
             * information related to the shape.
             *
             * @param stateContext Smart pointer to an IStateContext implementation.
             */
            void setStateContext( SmartPtr<IStateContext> stateContext ) override;

            /**
             * @brief Get the associated state context.
             *
             * @return SmartPtr<IStateContext> Current state context or nullptr.
             */
            SmartPtr<IStateContext> getStateContext() const override;

            /**
             * @brief Set a listener to receive state change notifications.
             *
             * @param stateListener Smart pointer to an IStateListener.
             */
            void setStateListener( SmartPtr<IStateListener> stateListener ) override;

            /**
             * @brief Get the current state listener.
             *
             * @return SmartPtr<IStateListener> Current listener or nullptr.
             */
            SmartPtr<IStateListener> getStateListener() const override;

            /**
             * @brief Set the shape collision type (bitmask).
             *
             * Collision type is used together with collision mask to filter
             * collision tests.
             *
             * @param mask Collision type bitmask.
             */
            void setCollisionType( u32 mask ) override;

            /**
             * @brief Get the shape collision type bitmask.
             *
             * @return u32 Collision type bitmask.
             */
            u32 getCollisionType() const override;

            /**
             * @brief Set the collision mask (which types this shape collides with).
             *
             * @param mask Collision mask bitmask.
             */
            void setCollisionMask( u32 mask ) override;

            /**
             * @brief Get the collision mask bitmask.
             *
             * @return u32 Collision mask.
             */
            u32 getCollisionMask() const override;

            /**
             * @brief Handle a state change described by a message.
             *
             * Implementations should react to incoming state messages and return
             * true if the message was handled.
             *
             * @param message State message describing the change.
             * @return true if handled; false otherwise.
             */
            bool handleStateChanged( const SmartPtr<IStateMessage> &message ) override;

            /**
             * @brief Handle a direct state object change.
             *
             * @param state The state instance that changed.
             * @return true if handled; false otherwise.
             */
            bool handleStateChanged( SmartPtr<IState> &state ) override;

            WP_CLASS_REGISTER_TEMPLATE_DECL( PhysicsShape2, T );
        };

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone::physics, PhysicsShape2, T, T );

        template <class T>
        PhysicsShape2<T>::~PhysicsShape2()
        {
        }

        template <class T>
        PhysicsShape2<T>::PhysicsShape2()
        {
        }

        template <class T>
        Sphere2<real_Num> PhysicsShape2<T>::getSphere() const
        {
            return {};
        }

        template <class T>
        AABB2<real_Num> PhysicsShape2<T>::getAABB() const
        {
            return {};
        }

        template <class T>
        void PhysicsShape2<T>::getPoints( Array<Vector2<real_Num>> &points ) const
        {
        }

        template <class T>
        void PhysicsShape2<T>::computeMass( SmartPtr<IMassData2> massData, real_Num density ) const
        {
        }

        template <class T>
        void PhysicsShape2<T>::_getObject( void **ppObject ) const
        {
        }

        template <class T>
        u8 PhysicsShape2<T>::getType() const
        {
            return 0;
        }

        template <class T>
        bool PhysicsShape2<T>::isAttached() const
        {
            return true;
        }

        template <class T>
        void PhysicsShape2<T>::setEnabled( bool enabled )
        {
        }

        template <class T>
        bool PhysicsShape2<T>::isEnabled() const
        {
            return true;
        }

        template <class T>
        void PhysicsShape2<T>::setTrigger( bool trigger )
        {
        }

        template <class T>
        bool PhysicsShape2<T>::isTrigger() const
        {
            return true;
        }

        template <class T>
        void PhysicsShape2<T>::setStateContext( SmartPtr<IStateContext> stateContext )
        {
        }

        template <class T>
        SmartPtr<IStateContext> PhysicsShape2<T>::getStateContext() const
        {
            return nullptr;
        }

        template <class T>
        void PhysicsShape2<T>::setStateListener( SmartPtr<IStateListener> stateListener )
        {
        }

        template <class T>
        SmartPtr<IStateListener> PhysicsShape2<T>::getStateListener() const
        {
            return nullptr;
        }

        template <class T>
        void PhysicsShape2<T>::setCollisionType( u32 mask )
        {
        }

        template <class T>
        u32 PhysicsShape2<T>::getCollisionType() const
        {
            return 0;
        }

        template <class T>
        void PhysicsShape2<T>::setCollisionMask( u32 mask )
        {
        }

        template <class T>
        u32 PhysicsShape2<T>::getCollisionMask() const
        {
            return 0;
        }

        template <class T>
        bool PhysicsShape2<T>::handleStateChanged( const SmartPtr<IStateMessage> &message )
        {
            return false;
        }

        template <class T>
        bool PhysicsShape2<T>::handleStateChanged( SmartPtr<IState> &state )
        {
            return false;
        }

    }  // namespace physics
}  // namespace workphone

#endif  // PhysicsShape2_h__
