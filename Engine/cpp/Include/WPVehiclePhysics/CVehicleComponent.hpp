#ifndef CVehicleComponent_h__
#define CVehicleComponent_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Vehicle/IVehicle.hpp>
#include <Workphone/Interface/Vehicle/IVehicleComponent.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Core/Handle.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Math/Transform3.hpp>

namespace workphone
{
    /**
     * @brief Template base class for vehicle components that provides common functionality.
     *
     * CVehicleComponent is a CRTP (Curiously Recurring Template Pattern) base class that
     * implements the IVehicleComponent interface. It provides standard functionality for
     * vehicle components including transform management, state handling, owner relationships,
     * and debugging capabilities.
     *
     * @tparam T The derived class type that inherits from this template
     *
     * @par Thread Safety
     * This class uses atomic smart pointers for the owner reference to ensure thread safety
     * when accessing the vehicle owner from multiple threads.
     *
     * @par Usage Example
     * @code
     * class MyWheelComponent : public CVehicleComponent<IWheelComponent>
     * {
     *     // Implementation specific to wheel components
     * };
     * @endcode
     */
    template <class T>
    class CVehicleComponent : public T
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Initializes the component with default state (AWAKE) and generates unique debug IDs
         * for debugging purposes. Creates 100 unique hash identifiers for debug tracking.
         */
        CVehicleComponent();

        /**
         * @brief Virtual destructor.
         *
         * Cleans up resources and ensures the owner reference is properly released.
         */
        ~CVehicleComponent() override;

        /**
         * @brief Loads component data from a shared object.
         *
         * This method is called during the component initialization phase to load
         * any configuration or state data.
         *
         * @param data Shared pointer to data object containing initialization parameters
         */
        void load(SmartPtr<ISharedObject> data) override;

        /**
         * @brief Unloads the component and cleans up resources.
         *
         * This method is called during component destruction to properly clean up
         * resources and break circular references.
         *
         * @param data Shared pointer to data object (currently unused)
         */
        void unload(SmartPtr<ISharedObject> data) override;

        /**
         * @brief Gets a raw pointer to the vehicle owner.
         *
         * @return Raw pointer to the IVehicle owner, or nullptr if no owner is set
         *
         * @warning The returned pointer should not be stored long-term as it may become invalid.
         * Use getOwner() for safe smart pointer access.
         */
        virtual IVehicle *getOwnerPtr() const;

        /**
         * @brief Gets a smart pointer to the vehicle owner.
         *
         * @return Smart pointer to the IVehicle owner, thread-safe access
         */
        virtual SmartPtr<IVehicle> getOwner() const;

        /**
         * @brief Sets the vehicle owner for this component.
         *
         * @param vehicle Smart pointer to the IVehicle that owns this component
         */
        virtual void setOwner(SmartPtr<IVehicle> vehicle);

        /**
         * @brief Updates the world transform of the component.
         *
         * Calculates the world transform by combining the parent vehicle's world transform
         * with this component's local transform. Includes validation to ensure transforms
         * have valid scales.
         *
         * @note This method performs assertions in debug builds to validate transform integrity
         */
        virtual void updateTransform() override;

        /**
         * @brief Updates the body transform without modifying the world transform.
         *
         * Similar to updateTransform() but calculates the transformation without storing
         * the result. Used for physics body positioning calculations.
         */
        virtual void updateBodyTransform() override;

        /**
         * @brief Updates the geometry representation of the component.
         *
         * Override this method in derived classes to update visual or collision geometry
         * based on current component state.
         */
        virtual void updateGeometry();

        /**
         * @brief Checks if the component is in a valid state.
         *
         * A component is considered valid if it has a valid owner vehicle assigned.
         *
         * @return true if the component has a valid owner, false otherwise
         */
        bool isValid() const override;

        /**
         * @brief Gets the world space transform of the component.
         *
         * @return The current world transform including position, rotation, and scale
         */
        virtual Transform3<physics_Num> getWorldTransform() const override;

        /**
         * @brief Sets the world space transform of the component.
         *
         * @param worldTransform The new world transform to apply
         */
        virtual void setWorldTransform(Transform3<physics_Num> worldTransform) override;

        /**
         * @brief Gets the local space transform relative to the parent vehicle.
         *
         * @return The current local transform relative to the owner vehicle
         */
        virtual Transform3<physics_Num> getLocalTransform() const override;

        /**
         * @brief Sets the local space transform relative to the parent vehicle.
         *
         * @param localTransform The new local transform to apply
         */
        virtual void setLocalTransform(Transform3<physics_Num> localTransform) override;

        /**
         * @copydoc IVehicleComponent::setState
         *
         * @param state The new state to set for this component
         */
        virtual void setState(IVehicleComponent::State state);

        /**
         * @copydoc IVehicleComponent::getState
         *
         * @return The current state of this component
         */
        virtual IVehicleComponent::State getState() const;

        /**
         * @brief Gets the generic data pointer associated with this component.
         *
         * @return Pointer to user-defined data, or nullptr if no data is set
         *
         * @warning The caller is responsible for knowing the actual type of the data
         */
        void *getData() const;

        /**
         * @brief Sets the generic data pointer for this component.
         *
         * @param data Pointer to user-defined data to associate with this component
         *
         * @warning The component does not take ownership of the pointed-to data
         */
        void setData(void *data);

        /**
         * @brief Gets a debug identifier for this component.
         *
         * Debug IDs are used for tracking and visualization purposes during development.
         * Each component has multiple debug IDs for different debugging contexts.
         *
         * @param i Index of the debug ID to retrieve (0-99)
         * @return The hash value of the debug ID, or 0 if index is out of range
         */
        s32 getDebugId(s32 i) const;

        /**
         * @brief Resets the component to its default state.
         *
         * Override this method in derived classes to implement component-specific reset logic.
         */
        void reset() override;

        SmartPtr<Properties> getProperties() const override;
        void setProperties(SmartPtr<Properties> properties) override;

        /// RTTI class registration declaration
        WP_CLASS_REGISTER_TEMPLATE_DECL(CVehicleComponent, T);

    protected:
        /// Current state of the component (AWAKE, DESTROYED, EDIT, PLAY, RESET)
        IVehicleComponent::State m_state = IVehicleComponent::State::AWAKE;

        /// Thread-safe reference to the owning vehicle
        AtomicSmartPtr<IVehicle> m_owner;

        /// Local transform relative to the parent vehicle
        Transform3<physics_Num> m_localTransform;

        /// World space transform of the component
        Transform3<physics_Num> m_worldTransform;

        /// Array of debug identifiers for tracking and visualization
        Array<hash_type> m_ids;

        /// Generic data pointer for user-defined data
        void *m_data = nullptr;
    };

    /// RTTI class registration for template instantiations
    WP_CLASS_REGISTER_DERIVED_TEMPLATE(workphone, CVehicleComponent, T, T);

    template <class T>
    CVehicleComponent<T>::CVehicleComponent()
    {
        // Generate unique debug identifiers
        auto uuid = StringUtil::getUUID();
        m_ids.resize(100);

        for(size_t i = 0; i < m_ids.size(); ++i)
        {
            m_ids[i] = StringUtil::getHash(uuid + "_" + StringUtil::toString(static_cast<s32>(i)));
        }
    }

    template <class T>
    CVehicleComponent<T>::~CVehicleComponent()
    {
        m_owner = nullptr;
    }

    template <class T>
    void CVehicleComponent<T>::load(SmartPtr<ISharedObject> data)
    {
        // Override in derived classes to implement specific loading logic
    }

    template <class T>
    void CVehicleComponent<T>::unload(SmartPtr<ISharedObject> data)
    {
        m_owner = nullptr;
    }

    template <class T>
    IVehicle *CVehicleComponent<T>::getOwnerPtr() const
    {
        return m_owner.get();
    }

    template <class T>
    SmartPtr<IVehicle> CVehicleComponent<T>::getOwner() const
    {
        return m_owner;
    }

    template <class T>
    void CVehicleComponent<T>::setOwner(SmartPtr<IVehicle> vehicle)
    {
        m_owner = vehicle;
    }

    template <class T>
    void CVehicleComponent<T>::updateTransform()
    {
        if(auto owner = getOwner())
        {
            auto parentTransform = owner->getWorldTransform();

            WP_ASSERT(parentTransform.getScale().length() > std::numeric_limits<f32>::epsilon());
            WP_ASSERT(getLocalTransform().getScale().length() > std::numeric_limits<f32>::epsilon());

            auto localTransform = getLocalTransform();
            auto worldTransform = getWorldTransform();

            worldTransform.transformFromParent(parentTransform, localTransform);
            WP_ASSERT(worldTransform.getScale().length() > std::numeric_limits<f32>::epsilon());

            setWorldTransform(worldTransform);
        }
    }

    template <class T>
    void CVehicleComponent<T>::updateBodyTransform()
    {
        if(auto owner = getOwner())
        {
            auto parentTransform = owner->getWorldTransform();

            WP_ASSERT(parentTransform.getScale().length() > std::numeric_limits<f32>::epsilon());
            WP_ASSERT(getLocalTransform().getScale().length() > std::numeric_limits<f32>::epsilon());

            auto localTransform = getLocalTransform();
            auto worldTransform = getWorldTransform();

            worldTransform.transformFromParent(parentTransform, localTransform);
            WP_ASSERT(worldTransform.getScale().length() > std::numeric_limits<f32>::epsilon());
        }
    }

    template <class T>
    void CVehicleComponent<T>::updateGeometry()
    {
        // Override in derived classes to implement geometry update logic
    }

    template <class T>
    bool CVehicleComponent<T>::isValid() const
    {
        auto owner = getOwner();
        return owner != nullptr;
    }

    template <class T>
    Transform3<physics_Num> CVehicleComponent<T>::getWorldTransform() const
    {
        return m_worldTransform;
    }

    template <class T>
    void CVehicleComponent<T>::setWorldTransform(Transform3<physics_Num> worldTransform)
    {
        m_worldTransform = worldTransform;
    }

    template <class T>
    Transform3<physics_Num> CVehicleComponent<T>::getLocalTransform() const
    {
        return m_localTransform;
    }

    template <class T>
    void CVehicleComponent<T>::setLocalTransform(Transform3<physics_Num> localTransform)
    {
        m_localTransform = localTransform;
    }

    template <class T>
    void CVehicleComponent<T>::setState(IVehicleComponent::State state)
    {
        m_state = state;
    }

    template <class T>
    IVehicleComponent::State CVehicleComponent<T>::getState() const
    {
        return m_state;
    }

    template <class T>
    void *CVehicleComponent<T>::getData() const
    {
        return m_data;
    }

    template <class T>
    void CVehicleComponent<T>::setData(void *data)
    {
        m_data = data;
    }

    template <class T>
    s32 CVehicleComponent<T>::getDebugId(s32 i) const
    {
        if(i < static_cast<s32>(m_ids.size()))
        {
            return static_cast<s32>(m_ids[static_cast<size_t>(i)]);
        }

        return 0;
    }

    template <class T>
    void CVehicleComponent<T>::reset()
    {
        // Override in derived classes to implement component-specific reset logic
    }

    template <class T>
    SmartPtr<Properties> CVehicleComponent<T>::getProperties() const
    {
        auto properties = T::getProperties();
        WP_ASSERT(properties);

        properties->setProperty("Local Transform", getLocalTransform());
        properties->setProperty("World Transform", getWorldTransform());
        properties->setProperty("State", static_cast<s32>(getState()));

        return properties;
    }

    template <class T>
    void CVehicleComponent<T>::setProperties(SmartPtr<Properties> properties)
    {
        WP_ASSERT(properties);
        if(!properties)
        {
            WP_LOG_ERROR("CVehicleComponent::setProperties received null properties.");
            return;
        }

        T::setProperties(properties);

        auto localTransform = getLocalTransform();
        auto worldTransform = getWorldTransform();
        auto state = static_cast<s32>(getState());

        properties->getPropertyValue("Local Transform", localTransform);
        properties->getPropertyValue("World Transform", worldTransform);
        properties->getPropertyValue("State", state);

        setLocalTransform(localTransform);
        setWorldTransform(worldTransform);
        setState(static_cast<IVehicleComponent::State>(state));
    }
} // namespace workphone

#endif // CVehicleComponent_h__
