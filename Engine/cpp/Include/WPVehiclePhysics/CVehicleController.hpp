#ifndef CVehicleController_h__
#define CVehicleController_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Atomics/AtomicFloat.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Atomics/AtomicValue.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/Vehicle/IVehicle.hpp>
#include <Workphone/Interface/Vehicle/IVehicleBody.hpp>
#include <Workphone/Interface/Vehicle/IVehicleCallback.hpp>
#include <Workphone/Interface/Vehicle/IWheelComponent.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/FixedArray.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Math/MathUtil.hpp>
#include <Workphone/Core/FixedArray.hpp>

namespace workphone
{
    /**
     * @brief Generic base implementation for a vehicle controller.
     *
     * CVehicleController provides common vehicle control behaviour (force/torque accumulation,
     * transform bookkeeping, debug drawing helpers and bridging to a platform-specific callback).
     * The template parameter @a T should be an interface type derived from IVehicle (or any
     * compatible vehicle interface) to supply virtual contract.
     *
     * @tparam T Interface or base type that this controller implements.
     *
     * @note This class is header-only and meant to be used as a mixin or direct base for concrete
     *       vehicle controller implementations.
     */
    template <class T>
    class CVehicleController : public T
    {
    public:
        /** Default constructor. */
        CVehicleController();

        /** Virtual destructor. */
        ~CVehicleController() override;

        /**
         * @brief Load controller state from a shared object (serialized data).
         * @param data Serialized object containing saved state (may be null).
         */
        void load(SmartPtr<ISharedObject> data) override;

        /**
         * @brief Load controller state from a string (simple text path or serialized blob).
         * @param data String containing configuration or serialized state.
         */
        void load(const std::string &data);

        /**
         * @brief Reload runtime data.
         * @param pData Pointer to raw data used for reload (format depends on caller).
         */
        void reload(void *pData);

        /** @brief Reset controller to default state. */
        void reset() override;

        /** @brief Called every frame / simulation step to update controller logic. */
        void update() override;

        /** @brief Update internal transforms (body/local/world) from position/orientation state. */
        void updateTransform();

        /**
         * @brief Get control channel value.
         * @param idx Channel index (0..N-1).
         * @return Channel value.
         */
        f32 getChannel(s32 idx) const override;

        /**
         * @brief Set control channel value.
         * @param idx Channel index.
         * @param channel New channel value.
         */
        void setChannel(s32 idx, f32 channel) override;

        /** @brief Get world scale from the underlying callback (if available). */
        Vector3<physics_Num> getScale() const;

        /** @brief Get vehicle world position (queried from callback if present). */
        Vector3<physics_Num> getPosition() const override;

        /** @brief Set vehicle world position. */
        void setPosition(const Vector3<physics_Num> &position) override;

        /** @brief Get vehicle orientation (queried from callback if present). */
        Quaternion<physics_Num> getOrientation() const;

        /** @brief Set vehicle orientation. */
        void setOrientation(const Quaternion<physics_Num> &orientation);

        /** @brief True if the vehicle is currently controlled directly by the user. */
        bool isUserControlled() const override;

        /** @brief Mark/unmark vehicle as user controlled. */
        void setUserControlled(bool userControlled) override;

        /** @brief Get vehicle mass (delegates to attached IVehicleBody when available). */
        physics_Num getMass() const override;

        /** @brief Set vehicle mass on the attached IVehicleBody. */
        void setMass(physics_Num mass) override;

        /**
         * @brief Get the associated physics body wrapper.
         * @return Smart pointer to IVehicleBody or null if none attached.
         */
        SmartPtr<IVehicleBody> getBody() const override;

        /**
         * @brief Attach or detach a physics body wrapper.
         * @param body Smart pointer to IVehicleBody (may be null to detach).
         */
        void setBody(SmartPtr<IVehicleBody> body) override;

        /** @brief Get calculated world transform for the controller. */
        Transform3<physics_Num> getWorldTransform() const override;

        /** @brief Set the controller's world transform. */
        void setWorldTransform(const Transform3<physics_Num> &worldTransform) override;

        /** @brief Get the controller's local transform (relative to the body). */
        Transform3<physics_Num> getLocalTransform() const override;

        /** @brief Set the controller's local transform. */
        void setLocalTransform(const Transform3<physics_Num> &localTransform) override;

        /**
         * @brief Draw a world-space debug point (renders three short axis lines).
         * @param body Body index used by the underlying callback.
         * @param id Base identifier for debug primitives.
         * @param positon Point in local coordinates that will be transformed to world.
         * @param color Color for the debug primitives.
         */
        void drawPoint(s32 body, int id, const Vector3<physics_Num> &positon, u32 color) override;

        /**
         * @brief Draw a debug point whose input is already in local space.
         * @param body Body index used by the underlying callback.
         * @param id Base identifier for debug primitives.
         * @param positon Local-space point to draw.
         * @param color Color for the debug primitives.
         */
        void drawLocalPoint(s32 body, int id, const Vector3<physics_Num> &positon, u32 color) override;

        /**
         * @brief Display a vector in local coordinates via the vehicle callback.
         * @param bodyId Body identifier.
         * @param start Vector start in local space.
         * @param end Vector end in local space.
         * @param colour Debug colour.
         */
        void displayLocalVector(s32 bodyId, const Vector3<physics_Num> &start,
                                const Vector3<physics_Num> &end, u32 colour) override;

        /**
         * @brief Display a world-space vector via the vehicle callback.
         * @param bodyId Body identifier.
         * @param id Unique id for the debug primitive (used by renderer/debugger).
         * @param start World-space start.
         * @param end World-space end.
         * @param colour Debug colour.
         */
        void displayVector(s32 bodyId, s32 id, const Vector3<physics_Num> &start,
                           const Vector3<physics_Num> &end, u32 colour) override;

        /**
         * @brief Display a local-space vector after transforming it into world space first.
         * @param bodyId Body identifier.
         * @param id Unique id for the debug primitive.
         * @param start Local-space start.
         * @param end Local-space end.
         * @param colour Debug colour.
         */
        void displayLocalVector(s32 bodyId, s32 id, const Vector3<physics_Num> &start,
                                const Vector3<physics_Num> &end, u32 colour) override;

        /** @brief Get whether debug drawing is enabled for this controller. */
        bool getDisplayDebugData() const override;

        /** @brief Enable/disable debug drawing for this controller. */
        void setDisplayDebugData(bool displayDebugData) override;

        /**
         * @brief Apply a world-space force at a world-space location (accumulates force + torque).
         * @param bodyIdx Index of the body (passed to callback in some implementations).
         * @param Force Force vector in world coordinates.
         * @param Loc World-space application point.
         */
        void addForce(s32 bodyIdx, const Vector3<physics_Num> &Force,
                      const Vector3<physics_Num> &Loc) override;

        /**
         * @brief Apply a world-space torque (accumulates torque).
         * @param bodyIdx Index of the body.
         * @param Torque Torque vector in world coordinates.
         */
        void addTorque(s32 bodyIdx, const Vector3<physics_Num> &Torque) override;

        /**
         * @brief Apply a force given in local coordinates (transformed to world and accumulated).
         * @param bodyIdx Index of the body.
         * @param Force Local-space force vector.
         * @param Loc Local-space application point.
         */
        void addLocalForce(s32 bodyIdx, const Vector3<physics_Num> &Force,
                           const Vector3<physics_Num> &Loc) override;

        /**
         * @brief Apply a torque given in local coordinates (transformed to world and accumulated).
         * @param bodyIdx Index of the body.
         * @param Torque Local-space torque.
         */
        void addLocalTorque(s32 bodyIdx, const Vector3<physics_Num> &torque) override;

        /**
         * @brief Get linear velocity of a point on the vehicle in world space.
         * @param p Point in world coordinates.
         * @return Linear velocity at point p.
         */
        Vector3<physics_Num> getPointVelocity(const Vector3<physics_Num> &p) override;

        /** @brief Get current angular velocity (world-space). */
        Vector3<physics_Num> getAngularVelocity() override;

        /** @brief Get current linear velocity (world-space). */
        Vector3<physics_Num> getLinearVelocity() override;

        /** @brief Get angular velocity expressed in local space. */
        Vector3<physics_Num> getLocalAngularVelocity() override;

        /** @brief Get linear velocity expressed in local space. */
        Vector3<physics_Num> getLocalLinearVelocity() override;

        /**
         * @brief Access vehicle callback used to bridge to simulation/rendering backend.
         * @return Reference to smart pointer holding the callback.
         */
        IVehicleCallback *getVehicleCallbackPtr() const override;

        /**
         * @brief Const access to the vehicle callback.
         * @return Const reference to the callback pointer.
         */
        SmartPtr<IVehicleCallback> getVehicleCallback() const override;

        /** @brief Assign the vehicle callback used for platform-specific operations. */
        void setVehicleCallback(SmartPtr<IVehicleCallback> callback) override;

        /** @brief Get the vehicle centre of gravity in local coordinates. */
        Vector3<physics_Num> getCG() const override;

        /** @copydoc IVehicleController::setState */
        virtual void setState(IVehicle::State state) override;

        /** @copydoc IVehicleController::getState */
        virtual IVehicle::State getState() const override;

        /**
         * @brief Get wheel controller by index.
         * @param index Wheel index.
         * @return Smart pointer to a wheel component or nullptr if not supported.
         */
        SmartPtr<IWheelComponent> getWheelController(u32 index) const;

        WP_CLASS_REGISTER_TEMPLATE_DECL(CVehicleController, T);

    protected:
        /** @brief Accumulate a world-space force (internal helper). */
        void addForce(const Vector3<physics_Num> &force);

        /** @brief Accumulate a world-space torque (internal helper). */
        void addTorque(const Vector3<physics_Num> &torque);

        /** @brief Clear accumulated forces and torques. */
        void clearForces();

        /** @brief Current vehicle state (atomic). */
        AtomicValue<IVehicle::State> m_vehicleState = IVehicle::State::AWAKE;

        /** @brief Transform from body local to controller-local coordinates. */
        Transform3<physics_Num> m_bodyTransform;
        /** @brief Calculated world transform of the controller. */
        Transform3<physics_Num> m_worldTransform;
        /** @brief Local transform relative to the body/origin. */
        Transform3<physics_Num> m_localTransform;

        /** @brief Physics body wrapper (atomic smart pointer). */
        AtomicSmartPtr<IVehicleBody> m_rigidbody;
        /** @brief Bridge callback to the physics/render layer. */
        SmartPtr<IVehicleCallback> m_callback;

        /** @brief Accumulated world-space force for the current update step. */
        Vector3<physics_Num> m_force = Vector3<physics_Num>::zero();
        /** @brief Accumulated world-space torque for the current update step. */
        Vector3<physics_Num> m_torque = Vector3<physics_Num>::zero();

        /** @brief Aerodynamic or frictional drag expressed in local space. */
        Vector3<physics_Num> m_drag = Vector3<physics_Num>::zero();
        /** @brief Centre of gravity in local coordinates. */
        Vector3<physics_Num> m_cg = Vector3<physics_Num>::zero();

        /** @brief If true, debug drawing calls will be forwarded to the callback. */
        atomic_bool m_displayDebugData = false;

        /** @brief Control channels (small fixed-size array, e.g. throttle/steer/...) */
        FixedArray<atomic_f32, 8> m_channels;
    };

    WP_CLASS_REGISTER_DERIVED_TEMPLATE(workphone, CVehicleController, T, T);

    template <class T>
    CVehicleController<T>::CVehicleController()
    {
    }

    template <class T>
    CVehicleController<T>::~CVehicleController()
    {
    }

    template <class T>
    void CVehicleController<T>::load(SmartPtr<ISharedObject> data)
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();

        // auto worldTransform = factoryManager->make_ptr<Transform3<physics_Num>>();
        // setWorldTransform(worldTransform);

        // auto localTransform = factoryManager->make_ptr<Transform3<physics_Num>>();
        // setLocalTransform(localTransform);

        // auto bodyTransform = factoryManager->make_ptr<Transform3<physics_Num>>();
        // m_bodyTransform = bodyTransform;
    }

    template <class T>
    void CVehicleController<T>::load(const std::string &data)
    {
        auto worldTransform = Transform3<physics_Num>();
        setWorldTransform(worldTransform);

        auto localTransform = Transform3<physics_Num>();
        setLocalTransform(localTransform);

        auto bodyTransform = Transform3<physics_Num>();
        // m_bodyTransform = bodyTransform;
    }

    template <class T>
    void CVehicleController<T>::reload(void *pData)
    {
    }

    template <class T>
    void CVehicleController<T>::reset()
    {
    }

    template <class T>
    void CVehicleController<T>::update()
    {
        updateTransform();

        if(m_callback)
        {
            m_callback->addForce(0, m_force, Vector3<physics_Num>::zero());
            m_callback->addTorque(0, m_torque);

            m_force = Vector3<physics_Num>::zero();
            m_torque = Vector3<physics_Num>::zero();
        }
    }

    template <class T>
    f32 CVehicleController<T>::getChannel(s32 idx) const
    {
        return m_channels[idx];
    }

    template <class T>
    void CVehicleController<T>::setChannel(s32 idx, f32 channel)
    {
        m_channels[idx] = channel;
    }

    template <class T>
    void CVehicleController<T>::updateTransform()
    {
        auto p = getPosition();
        auto q = getOrientation();

        WP_ASSERT(MathUtil<physics_Num>::isFinite( p ));
        WP_ASSERT(MathUtil<physics_Num>::isFinite( q ));

        // p = Vector3<physics_Num>::zero();
        // q = Quaternion<physics_Num>::identity();

        // if (m_bodyTransform)
        {
            m_bodyTransform.setPosition(p);
            m_bodyTransform.setOrientation(q);
            m_bodyTransform.setScale(Vector3<physics_Num>::unit());

            // if (m_worldTransform)
            {
                // if (m_localTransform)
                {
                    m_worldTransform.transformFromParent(m_bodyTransform, m_localTransform);
                }
            }
        }

        if(auto body = getBody())
        {
            auto cg = m_bodyTransform.transformPoint(m_cg);
            body->setWorldCenterOfMass(cg);
        }
    }

    template <class T>
    Vector3<physics_Num> CVehicleController<T>::getScale() const
    {
        if(m_callback)
        {
            return m_callback->getScale();
        }

        return Vector3<physics_Num>::zero();
    }

    template <class T>
    Vector3<physics_Num> CVehicleController<T>::getPosition() const
    {
        if(m_callback)
        {
            return m_callback->getPosition();
        }

        return Vector3<physics_Num>::zero();
    }

    template <class T>
    void CVehicleController<T>::setPosition(const Vector3<physics_Num> &position)
    {
    }

    template <class T>
    Quaternion<physics_Num> CVehicleController<T>::getOrientation() const
    {
        if(m_callback)
        {
            return m_callback->getOrientation();
        }

        return Quaternion<physics_Num>::identity();
    }

    template <class T>
    void CVehicleController<T>::setOrientation(const Quaternion<physics_Num> &orientation)
    {
    }

    template <class T>
    bool CVehicleController<T>::isUserControlled() const
    {
        return false;
    }

    template <class T>
    void CVehicleController<T>::setUserControlled(bool userControlled)
    {
    }

    template <class T>
    physics_Num CVehicleController<T>::getMass() const
    {
        if(auto body = getBody())
        {
            return body->getMass();
        }

        return 1.0;
    }

    template <class T>
    void CVehicleController<T>::setMass(physics_Num mass)
    {
        if(auto body = getBody())
        {
            body->setMass(mass);
        }
    }

    template <class T>
    SmartPtr<IVehicleBody> CVehicleController<T>::getBody() const
    {
        return m_rigidbody;
    }

    template <class T>
    void CVehicleController<T>::setBody(SmartPtr<IVehicleBody> rigidbody)
    {
        m_rigidbody = rigidbody;
    }

    template <class T>
    Transform3<physics_Num> CVehicleController<T>::getWorldTransform() const
    {
        return m_worldTransform;
    }

    template <class T>
    void CVehicleController<T>::setWorldTransform(const Transform3<physics_Num> &worldTransform)
    {
        m_worldTransform = worldTransform;
    }

    template <class T>
    Transform3<physics_Num> CVehicleController<T>::getLocalTransform() const
    {
        return m_localTransform;
    }

    template <class T>
    void CVehicleController<T>::setLocalTransform(const Transform3<physics_Num> &localTransform)
    {
        m_localTransform = localTransform;
    }

    template <class T>
    void CVehicleController<T>::drawPoint(s32 body, int id, const Vector3<physics_Num> &positon,
                                          u32 color)
    {
        auto size = static_cast<physics_Num>(0.1);
        auto offset0 = Vector3<physics_Num>::forward() * size;
        auto offset1 = Vector3<physics_Num>::up() * size;
        auto offset2 = Vector3<physics_Num>::right() * size;

        auto p = m_worldTransform.getOrientation() * positon;
        p += m_worldTransform.getPosition();

        displayVector(body, id, p + offset0, p - offset0, color);
        displayVector(body, id + 1, p + offset1, p - offset1, color);
        displayVector(body, id + 2, p + offset2, p - offset2, color);
    }

    template <class T>
    void CVehicleController<T>::drawLocalPoint(s32 body, int id, const Vector3<physics_Num> &positon,
                                               u32 color)
    {
        auto size = 0.1f;
        auto offset0 = Vector3<physics_Num>::forward() * size;
        auto offset1 = Vector3<physics_Num>::up() * size;
        auto offset2 = Vector3<physics_Num>::right() * size;

        // if (m_worldTransform)
        {
            auto p = m_worldTransform.getOrientation() * positon;
            p += m_worldTransform.getPosition();

            displayVector(body, id, p + offset0, p - offset0, color);
            displayVector(body, id + 1000, p + offset1, p - offset1, color);
            displayVector(body, id + 2000, p + offset2, p - offset2, color);
        }
    }

    template <class T>
    void CVehicleController<T>::displayLocalVector(s32 bodyId, const Vector3<physics_Num> &start,
                                                   const Vector3<physics_Num> &end, u32 colour)
    {
        try
        {
            if(m_callback)
            {
                m_callback->displayLocalVector(
                    bodyId, Vector3<physics_Num>(start.X(), start.Y(), start.Z()),
                    Vector3<physics_Num>(end.X(), end.Y(), end.Z()), colour);
            }
        }
        catch(std::exception &Err)
        {
            WP_LOG_EXCEPTION(Err);
        }
    }

    template <class T>
    void CVehicleController<T>::displayVector(s32 bodyId, s32 id, const Vector3<physics_Num> &start,
                                              const Vector3<physics_Num> &end, u32 colour)
    {
        try
        {
            if(m_callback)
            {
                m_callback->displayVector(bodyId, id, start, end, colour);
            }
        }
        catch(std::exception &Err)
        {
            WP_LOG_EXCEPTION(Err);
        }
    }

    template <class T>
    void CVehicleController<T>::displayLocalVector(s32 bodyId, s32 id,
                                                   const Vector3<physics_Num> &start,
                                                   const Vector3<physics_Num> &end, u32 colour)
    {
        try
        {
            if(m_callback)
            {
                auto worldTransform = getWorldTransform();

                WP_ASSERT(worldTransform.isValid());

                WP_ASSERT(start.isValid());
                WP_ASSERT(end.isValid());

                auto worldStart = worldTransform.transformPoint(start);
                auto worldEnd = worldTransform.transformPoint(end);

                WP_ASSERT(worldStart.isValid());
                WP_ASSERT(worldEnd.isValid());

                m_callback->displayVector(bodyId, id, start, end, colour);
            }
        }
        catch(std::exception &Err)
        {
            WP_LOG_EXCEPTION(Err);
        }
    }

    template <class T>
    bool CVehicleController<T>::getDisplayDebugData() const
    {
        return m_displayDebugData;
    }

    template <class T>
    void CVehicleController<T>::setDisplayDebugData(bool displayDebugData)
    {
        m_displayDebugData = displayDebugData;
    }

    template <class T>
    void CVehicleController<T>::addForce(s32 bodyIdx, const Vector3<physics_Num> &Force,
                                         const Vector3<physics_Num> &Loc)
    {
        WP_ASSERT(Force.length() < 1e5);
        WP_ASSERT(Loc.length() < 1e5);

        // if (m_callback)
        // {
        //     m_callback->addForce(Bdy, Force, Loc);
        // }

        const Vector3<physics_Num> centerOfMass = m_worldTransform.transformPoint(m_cg);
        const Vector3<physics_Num> torque = (Loc - centerOfMass).crossProduct(Force);

        addForce(Force);
        addTorque(torque);
    }

    template <class T>
    void CVehicleController<T>::addTorque(s32 bodyIdx, const Vector3<physics_Num> &Torque)
    {
        WP_ASSERT(Torque.length() < 1e5);

        // if (m_callback)
        // {
        //     m_callback->addTorque(Bdy, Torque);
        // }

        addTorque(Torque);
    }

    template <class T>
    void CVehicleController<T>::addLocalForce(s32 bodyIdx, const Vector3<physics_Num> &Force,
                                              const Vector3<physics_Num> &Loc)
    {
        WP_ASSERT(Force.length() < 1e10);
        WP_ASSERT(Loc.length() < 1e10);

        // if (m_callback)
        // {
        //     m_callback->addLocalForce(Bdy, Force, Loc);
        // }

        const auto worldPoint = m_worldTransform.transformPoint(Loc);
        const auto force = m_worldTransform.transformVector(Force);

        if(auto body = getBody())
        {
            const auto centerOfMass = body->getWorldCenterOfMass();
            const auto torque = (worldPoint - centerOfMass).crossProduct(force);

            addForce(force);
            addTorque(torque);
        }
    }

    template <class T>
    void CVehicleController<T>::addLocalTorque(s32 bodyIdx, const Vector3<physics_Num> &t)
    {
        WP_ASSERT(t.length() < 1e5);

        // if (m_callback)
        // {
        //     m_callback->addLocalTorque(Bdy, Torque);
        // }

        auto torque = m_worldTransform.getOrientation() * t;
        addTorque(torque);
    }

    template <class T>
    Vector3<physics_Num> CVehicleController<T>::getPointVelocity(const Vector3<physics_Num> &p)
    {
        if(m_callback)
        {
            return m_callback->getPointVelocity(p);
        }

        return Vector3<physics_Num>::zero();
    }

    template <class T>
    Vector3<physics_Num> CVehicleController<T>::getAngularVelocity()
    {
        if(m_callback)
        {
            return m_callback->getAngularVelocity();
        }

        return Vector3<physics_Num>::zero();
    }

    template <class T>
    Vector3<physics_Num> CVehicleController<T>::getLinearVelocity()
    {
        if(m_callback)
        {
            return m_callback->getLinearVelocity();
        }

        return Vector3<physics_Num>::zero();
    }

    template <class T>
    Vector3<physics_Num> CVehicleController<T>::getLocalAngularVelocity()
    {
        if(m_callback)
        {
            auto worldAngularVelocity = m_callback->getAngularVelocity();
            return m_worldTransform.inverseTransformVector(worldAngularVelocity);
        }

        return Vector3<physics_Num>::zero();
    }

    template <class T>
    Vector3<physics_Num> CVehicleController<T>::getLocalLinearVelocity()
    {
        if(m_callback)
        {
            return m_callback->getLocalLinearVelocity();
        }

        return Vector3<physics_Num>::zero();
    }

    template <class T>
    IVehicleCallback *CVehicleController<T>::getVehicleCallbackPtr() const
    {
        return m_callback.get();
    }

    template <class T>
    SmartPtr<IVehicleCallback> CVehicleController<T>::getVehicleCallback() const
    {
        return m_callback;
    }

    template <class T>
    void CVehicleController<T>::setVehicleCallback(SmartPtr<IVehicleCallback> callback)
    {
        m_callback = callback;
    }

    template <class T>
    Vector3<physics_Num> CVehicleController<T>::getCG() const
    {
        return m_cg;
    }

    template <class T>
    void CVehicleController<T>::setState(IVehicle::State state)
    {
        m_vehicleState = state;
    }

    template <class T>
    IVehicle::State CVehicleController<T>::getState() const
    {
        return m_vehicleState;
    }

    template <class T>
    void CVehicleController<T>::addForce(const Vector3<physics_Num> &force)
    {
        m_force += force;
    }

    template <class T>
    void CVehicleController<T>::addTorque(const Vector3<physics_Num> &torque)
    {
        m_torque += torque;
    }

    template <class T>
    void CVehicleController<T>::clearForces()
    {
        m_force = Vector3<physics_Num>::zero();
        m_torque = Vector3<physics_Num>::zero();
    }

    template <class T>
    SmartPtr<IWheelComponent> CVehicleController<T>::getWheelController(u32 index) const
    {
        return nullptr;
    }
} // namespace workphone

#endif // CVehicleController_h__
