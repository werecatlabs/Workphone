#ifndef CAircraftAttachment_h__
#define CAircraftAttachment_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Interface/Vehicle/IAircraft.hpp>
#include <Workphone/Interface/Vehicle/IVehicleBody.hpp>
#include <Workphone/Interface/Vehicle/IVehicleComponent.hpp>

namespace workphone::vehicle
{
    template <class T>
    class CAircraftAttachment : public T
    {
    public:
        CAircraftAttachment();
        ~CAircraftAttachment() override;

        virtual void update(const double &t, const double &dt);

        virtual void updateGeometry() override;

        virtual void load(void *pData);
        void unload(SmartPtr<ISharedObject> data) override;

        bool isValid() const override;

        bool isMirror() const;
        void setMirror(bool mirror);

        SmartPtr<CAircraftAttachment> getMirrorObject() const;
        void setMirrorObject(SmartPtr<CAircraftAttachment> mirrorObject);

        virtual void updateTransform() override;
        virtual void updateBodyTransform() override;

        SmartPtr<IAircraft> getParentAircraft() const;
        void setParentAircraft(SmartPtr<IAircraft> parentAircraft);

        SmartPtr<IVehicleBody> getParent() const;
        void setParent(SmartPtr<IVehicleBody> parent);

        Transform3<real_Num> getWorldTransform() const override;
        void setWorldTransform(Transform3<real_Num> worldTransform) override;

        Transform3<real_Num> getLocalTransform() const override;
        void setLocalTransform(Transform3<real_Num> worldTransform) override;

        Transform3<real_Num> getLocalBodyTransform() const;
        void setLocalBodyTransform(Transform3<real_Num> worldTransform);

        IVehicle *getOwnerPtr() const override;

        virtual SmartPtr<IVehicle> getOwner() const override;
        virtual void setOwner(SmartPtr<IVehicle> vehicle) override;

        void *getData() const override;

        void setData(void *data) override;

        virtual void reset() override;

        /** @copydoc IComponent::setState */
        void setState(IVehicleComponent::State state);

        /** @copydoc IComponent::setState */
        IVehicleComponent::State getState() const;

    protected:
        SmartPtr<IAircraft> m_parentAircraft;
        SmartPtr<IVehicleBody> m_parent;
        Transform3<real_Num> m_worldTransform;
        Transform3<real_Num> m_localTransform;
        Transform3<real_Num> m_localBodyTransform;
        SmartPtr<CAircraftAttachment> m_mirrorObject;
        bool m_isMirror;
        void *m_data = nullptr;
    };

    template <class T>
    CAircraftAttachment<T>::CAircraftAttachment()
    {
    }

    template <class T>
    CAircraftAttachment<T>::~CAircraftAttachment()
    {
    }

    template <class T>
    void CAircraftAttachment<T>::update(const double &t, const double &dt)
    {
    }

    template <class T>
    void CAircraftAttachment<T>::updateGeometry()
    {
    }

    template <class T>
    bool CAircraftAttachment<T>::isValid() const
    {
        return m_parentAircraft != nullptr && m_parent != nullptr && m_worldTransform.isValid() &&
               m_localTransform.isValid();
    }

    template <class T>
    void CAircraftAttachment<T>::load(void *pData)
    {
    }

    template <class T>
    void CAircraftAttachment<T>::unload(SmartPtr<ISharedObject> data)
    {
        m_parentAircraft = nullptr;
        m_parent = nullptr;
    }

    template <class T>
    bool CAircraftAttachment<T>::isMirror() const
    {
        return m_isMirror;
    }

    template <class T>
    void CAircraftAttachment<T>::setMirror(bool mirror)
    {
        m_isMirror = mirror;
    }

    template <class T>
    SmartPtr<CAircraftAttachment<T>> CAircraftAttachment<T>::getMirrorObject() const
    {
        return m_mirrorObject;
    }

    template <class T>
    void CAircraftAttachment<T>::setMirrorObject(SmartPtr<CAircraftAttachment> mirrorObject)
    {
        m_mirrorObject = mirrorObject;
    }

    template <class T>
    void CAircraftAttachment<T>::updateTransform()
    {
        auto parentTransform = getParentAircraft()->getWorldTransform();

        WP_ASSERT(parentTransform.getScale().length() > std::numeric_limits<f32>::epsilon());
        WP_ASSERT(getLocalTransform().getScale().length() > std::numeric_limits<f32>::epsilon());

        getWorldTransform().transformFromParent(parentTransform, getLocalTransform());
        WP_ASSERT(getWorldTransform().getScale().length() > std::numeric_limits<f32>::epsilon());
    }

    template <class T>
    void CAircraftAttachment<T>::updateBodyTransform()
    {
        auto parentTransform = getParentAircraft()->getWorldTransform();

        WP_ASSERT(parentTransform.getScale().length() > std::numeric_limits<f32>::epsilon());
        WP_ASSERT(getLocalTransform().getScale().length() > std::numeric_limits<f32>::epsilon());

        getLocalBodyTransform().transformFromParent(parentTransform, getLocalTransform());
        WP_ASSERT(getWorldTransform().getScale().length() > std::numeric_limits<f32>::epsilon());
    }

    template <class T>
    SmartPtr<IAircraft> CAircraftAttachment<T>::getParentAircraft() const
    {
        return m_parentAircraft;
    }

    template <class T>
    void CAircraftAttachment<T>::setParentAircraft(SmartPtr<IAircraft> parentAircraft)
    {
        m_parentAircraft = parentAircraft;
    }

    template <class T>
    SmartPtr<IVehicleBody> CAircraftAttachment<T>::getParent() const
    {
        return m_parent;
    }

    template <class T>
    void CAircraftAttachment<T>::setParent(SmartPtr<IVehicleBody> parent)
    {
        m_parent = parent;
    }

    template <class T>
    Transform3<real_Num> CAircraftAttachment<T>::getWorldTransform() const
    {
        return m_worldTransform;
    }

    template <class T>
    void CAircraftAttachment<T>::setWorldTransform(Transform3<real_Num> worldTransform)
    {
        m_worldTransform = worldTransform;
    }

    template <class T>
    Transform3<real_Num> CAircraftAttachment<T>::getLocalTransform() const
    {
        return m_localTransform;
    }

    template <class T>
    void CAircraftAttachment<T>::setLocalTransform(Transform3<real_Num> localTransform)
    {
        m_localTransform = localTransform;
    }

    template <class T>
    Transform3<real_Num> CAircraftAttachment<T>::getLocalBodyTransform() const
    {
        return m_localBodyTransform;
    }

    template <class T>
    void CAircraftAttachment<T>::setLocalBodyTransform(Transform3<real_Num> localTransform)
    {
        m_localBodyTransform = localTransform;
    }

    template <class T>
    IVehicle *CAircraftAttachment<T>::getOwnerPtr() const
    {
        return m_parentAircraft.get();
    }

    template <class T>
    SmartPtr<IVehicle> CAircraftAttachment<T>::getOwner() const
    {
        return m_parentAircraft;
    }

    template <class T>
    void CAircraftAttachment<T>::setOwner(SmartPtr<IVehicle> vehicle)
    {
        m_parentAircraft = workphone::static_pointer_cast<IAircraft>(vehicle);
    }

    template <class T>
    void *CAircraftAttachment<T>::getData() const
    {
        return m_data;
    }

    template <class T>
    void CAircraftAttachment<T>::setData(void *data)
    {
        m_data = data;
    }

    template <class T>
    void CAircraftAttachment<T>::reset()
    {
    }

    template <class T>
    void CAircraftAttachment<T>::setState(IVehicleComponent::State state)
    {
    }

    template <class T>
    IVehicleComponent::State CAircraftAttachment<T>::getState() const
    {
        return static_cast<IVehicleComponent::State>(0);
    }
}

#endif // AircraftAttachment_h__
