#ifndef __VehicleComponent_h__
#define __VehicleComponent_h__

#include <Workphone/Interface/Vehicle/IVehicle.hpp>
#include <Workphone/Interface/Vehicle/IVehicleComponent.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Math/Transform3.hpp>

namespace workphone
{
    namespace vehicle
    {

        /**
         * @class VehicleComponent
         * @brief Template base class for vehicle components providing common functionality and state
         * management.
         *
         * This template class serves as a foundation for all vehicle components, providing essential
         * functionality such as transform management, owner relationships, state tracking, and debugging
         * capabilities. It follows the CRTP (Curiously Recurring Template Pattern) design pattern.
         *
         * @tparam T The derived component type that inherits from this class
         *
         * @ingroup Vehicle
         * @author WorkPhone Development Team
         * @date 2025
         *
         * @par Thread Safety
         * This class is not thread-safe. External synchronization is required for concurrent access.
         */
        template <class T>
        class VehicleComponent : public T
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Initializes the component with default state (AWAKE) and generates unique debug IDs
             * for debugging and identification purposes. Creates 100 unique hash IDs based on UUID.
             */
            VehicleComponent();

            /**
             * @brief Virtual destructor.
             *
             * Ensures proper cleanup of resources and clears the owner reference to prevent
             * circular dependencies.
             */
            ~VehicleComponent() override;

            /**
             * @brief Loads component data and initializes the component.
             *
             * This method is called during the component loading phase. Override in derived
             * classes to implement specific loading behavior.
             *
             * @param data Shared object containing initialization data (currently unused in base
             * implementation)
             *
             * @see ISharedObject::load
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads component data and performs cleanup.
             *
             * This method is called during the component unloading phase. Clears the owner
             * reference and performs necessary cleanup operations.
             *
             * @param data Shared object containing cleanup data (currently unused in base
             * implementation)
             *
             * @see ISharedObject::unload
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Gets the vehicle that owns this component.
             *
             * @return Smart pointer to the owning vehicle, or nullptr if no owner is set
             *
             * @see setOwner
             */
            IVehicle *getOwnerPtr() const override;

            /**
             * @brief Gets the vehicle that owns this component.
             *
             * @return Smart pointer to the owning vehicle, or nullptr if no owner is set
             *
             * @see setOwner
             */
            SmartPtr<IVehicle> getOwner() const override;

            /**
             * @brief Sets the vehicle that owns this component.
             *
             * Establishes the parent-child relationship between the vehicle and this component.
             * This is essential for transform hierarchy and component management.
             *
             * @param vehicle Smart pointer to the vehicle that will own this component
             *
             * @see getOwner
             */
            void setOwner( SmartPtr<IVehicle> vehicle ) override;

            /**
             * @brief Updates the component's world transform based on the owner's transform.
             *
             * Calculates and applies the world transform by combining the owner's world transform
             * with this component's local transform. Includes validation to ensure transforms
             * have valid scale values.
             *
             * @note This method should be called whenever the parent's transform or the local
             *       transform changes to maintain proper transform hierarchy.
             *
             * @see updateBodyTransform, setLocalTransform, getWorldTransform
             */
            void updateTransform() override;

            /**
             * @brief Updates the component's body transform without applying it to the world transform.
             *
             * Similar to updateTransform() but performs the calculation without setting the result
             * as the new world transform. Useful for preview calculations or specialized update
             * patterns.
             *
             * @see updateTransform
             */
            void updateBodyTransform() override;

            /**
             * @brief Updates the component's geometry representation.
             *
             * This method should be overridden in derived classes to update any visual or
             * collision geometry associated with the component. The base implementation is empty.
             */
            void updateGeometry() override;

            /**
             * @brief Checks if the component is in a valid state.
             *
             * A component is considered valid if it has a valid owner (vehicle) assigned.
             * This is used for validation and error checking throughout the system.
             *
             * @return true if the component has a valid owner, false otherwise
             */
            bool isValid() const override;

            /**
             * @brief Gets the component's world transform.
             *
             * The world transform represents the component's position, rotation, and scale
             * in world coordinates, taking into account the entire transform hierarchy.
             *
             * @return The current world transform
             *
             * @see setWorldTransform, getLocalTransform
             */
            Transform3<real_Num> getWorldTransform() const override;

            /**
             * @brief Sets the component's world transform.
             *
             * Directly sets the world transform without affecting the local transform.
             * Use with caution as this bypasses the normal transform hierarchy.
             *
             * @param transform The new world transform to apply
             *
             * @see getWorldTransform, setLocalTransform
             */
            void setWorldTransform( Transform3<real_Num> worldTransform ) override;

            /**
             * @brief Gets the component's local transform relative to its owner.
             *
             * The local transform represents the component's position, rotation, and scale
             * relative to its parent (owner) coordinate system.
             *
             * @return The current local transform
             *
             * @see setLocalTransform, getWorldTransform
             */
            Transform3<real_Num> getLocalTransform() const override;

            /**
             * @brief Sets the component's local transform relative to its owner.
             *
             * The local transform defines the component's position, rotation, and scale
             * relative to its parent. Call updateTransform() after setting to update
             * the world transform accordingly.
             *
             * @param transform The new local transform to apply
             *
             * @see getLocalTransform, updateTransform
             */
            void setLocalTransform( Transform3<real_Num> localTransform ) override;

            /**
             * @brief Sets the component's current state.
             *
             * The state determines the component's operational mode and behavior.
             * Different states may affect how the component responds to updates and interactions.
             *
             * @param state The new state to assign to the component
             *
             * @see getState, IVehicleComponent::State
             */
            void setState( IVehicleComponent::State state ) override;

            /**
             * @brief Gets the component's current state.
             *
             * @return The current operational state of the component
             *
             * @see setState, IVehicleComponent::State
             */
            IVehicleComponent::State getState() const override;

            /**
             * @brief Gets the custom data pointer associated with this component.
             *
             * This provides a way to attach arbitrary user data to the component.
             * The component does not manage the lifetime of this data.
             *
             * @return Pointer to the custom data, or nullptr if no data is set
             *
             * @warning The caller is responsible for the lifetime management of the pointed data
             *
             * @see setData
             */
            void *getData() const override;

            /**
             * @brief Sets custom data pointer for this component.
             *
             * Allows attaching arbitrary user data to the component. The component
             * does not take ownership or manage the lifetime of the provided data.
             *
             * @param data Pointer to the custom data to associate with this component
             *
             * @warning The caller must ensure the data remains valid for the component's lifetime
             *
             * @see getData
             */
            void setData( void *data ) override;

            /**
             * @brief Gets a debug identifier for the component.
             *
             * Returns one of the pre-generated unique hash identifiers used for debugging
             * and tracking purposes. These IDs are generated during construction.
             *
             * @param i Index of the debug ID to retrieve (0-99)
             * @return The debug hash ID at the specified index, or 0 if index is out of range
             *
             * @note The component generates 100 unique debug IDs during construction
             */
            s32 getDebugId( s32 i ) const;

            /**
             * @brief Resets the component to its default state.
             *
             * This method should be overridden in derived classes to implement specific
             * reset behavior. The base implementation is empty.
             */
            void reset() override;

            /**
             * @brief Class registration declaration for the template.
             *
             * Enables runtime type identification and factory creation for the templated class.
             */
            WP_CLASS_REGISTER_TEMPLATE_DECL( VehicleComponent, T );

        protected:
            /// @brief Current operational state of the component (default: AWAKE)
            IVehicleComponent::State m_state = IVehicleComponent::State::AWAKE;

            /// @brief Thread-safe smart pointer to the owning vehicle
            AtomicSmartPtr<IVehicle> m_owner;

            /// @brief Local transform relative to the owner
            Transform3<real_Num> m_localTransform;

            /// @brief World transform in global coordinates
            Transform3<real_Num> m_worldTransform;

            /// @brief Array of unique hash IDs for debugging purposes
            Array<hash_type> m_ids;

            /// @brief Pointer to custom user data (not managed by the component)
            void *m_data = nullptr;
        };

        /**
         * @brief Template class registration for VehicleComponent.
         *
         * Registers the VehicleComponent template with the runtime type system,
         * enabling dynamic creation and type identification.
         */
        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone::vehicle, VehicleComponent, T, T );

        template <class T>
        VehicleComponent<T>::VehicleComponent()
        {
            // setWorldTransform(fb::make_ptr<Transform3<real_Num>>());
            // setLocalTransform(fb::make_ptr<Transform3<real_Num>>());

            auto uuid = StringUtil::getUUID();
            m_ids.resize( 100 );

            for( size_t i = 0; i < m_ids.size(); ++i )
            {
                m_ids[i] =
                    StringUtil::getHash( uuid + "_" + StringUtil::toString( static_cast<s32>( i ) ) );
            }
        }

        template <class T>
        VehicleComponent<T>::~VehicleComponent()
        {
            m_owner = nullptr;
        }

        template <class T>
        void VehicleComponent<T>::load( SmartPtr<ISharedObject> data )
        {
        }

        template <class T>
        void VehicleComponent<T>::unload( SmartPtr<ISharedObject> data )
        {
            m_owner = nullptr;
        }

        template <class T>
        IVehicle *VehicleComponent<T>::getOwnerPtr() const
        {
            return m_owner.get();
        }

        template <class T>
        SmartPtr<IVehicle> VehicleComponent<T>::getOwner() const
        {
            return m_owner;
        }

        template <class T>
        void VehicleComponent<T>::setOwner( SmartPtr<IVehicle> vehicle )
        {
            m_owner = vehicle;
        }

        template <class T>
        void VehicleComponent<T>::updateTransform()
        {
            if( auto owner = getOwner() )
            {
                auto parentTransform = owner->getWorldTransform();

                WP_ASSERT( parentTransform.getScale().length() > std::numeric_limits<f32>::epsilon() );
                WP_ASSERT( getLocalTransform().getScale().length() >
                           std::numeric_limits<f32>::epsilon() );

                auto localTransform = getLocalTransform();
                auto worldTransform = getWorldTransform();

                worldTransform.transformFromParent( parentTransform, localTransform );
                WP_ASSERT( worldTransform.getScale().length() > std::numeric_limits<f32>::epsilon() );

                setWorldTransform( worldTransform );
            }
        }

        template <class T>
        void VehicleComponent<T>::updateBodyTransform()
        {
            auto owner = getOwner();
            if( owner )
            {
                auto parentTransform = owner->getWorldTransform();

                WP_ASSERT( parentTransform.getScale().length() > std::numeric_limits<f32>::epsilon() );
                WP_ASSERT( getLocalTransform().getScale().length() >
                           std::numeric_limits<f32>::epsilon() );

                auto localTransform = getLocalTransform();
                auto worldTransform = getWorldTransform();

                worldTransform.transformFromParent( parentTransform, localTransform );
                WP_ASSERT( worldTransform.getScale().length() > std::numeric_limits<f32>::epsilon() );
            }
        }

        template <class T>
        void VehicleComponent<T>::updateGeometry()
        {
        }

        template <class T>
        bool VehicleComponent<T>::isValid() const
        {
            auto owner = getOwner();
            return owner != nullptr;
        }

        template <class T>
        Transform3<real_Num> VehicleComponent<T>::getWorldTransform() const
        {
            return m_worldTransform;
        }

        template <class T>
        void VehicleComponent<T>::setWorldTransform( Transform3<real_Num> transform )
        {
            m_worldTransform = transform;
        }

        template <class T>
        Transform3<real_Num> VehicleComponent<T>::getLocalTransform() const
        {
            return m_localTransform;
        }

        template <class T>
        void VehicleComponent<T>::setLocalTransform( Transform3<real_Num> transform )
        {
            m_localTransform = transform;
        }

        template <class T>
        void VehicleComponent<T>::setState( IVehicleComponent::State state )
        {
            m_state = state;
        }

        template <class T>
        IVehicleComponent::State VehicleComponent<T>::getState() const
        {
            return m_state;
        }

        template <class T>
        void *VehicleComponent<T>::getData() const
        {
            return m_data;
        }

        template <class T>
        void VehicleComponent<T>::setData( void *data )
        {
            m_data = data;
        }

        template <class T>
        s32 VehicleComponent<T>::getDebugId( s32 i ) const
        {
            if( i < m_ids.size() )
            {
                return (s32)m_ids[i];
            }

            return 0;
        }

        template <class T>
        void VehicleComponent<T>::reset()
        {
        }

    }  // namespace vehicle
}  // namespace workphone

#endif  // VehicleComponent_h__
