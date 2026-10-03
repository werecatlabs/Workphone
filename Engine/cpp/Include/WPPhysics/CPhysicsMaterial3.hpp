#ifndef WP_CPHYSICSMATERIAL3_HPP
#define WP_CPHYSICSMATERIAL3_HPP

#include <WPPhysics/CPhysicsConversions3.hpp>
#include <Workphone/Interface/Physics/IPhysicsMaterial3.hpp>

namespace workphone::physics
{
    /**
     * @class CPhysicsMaterial3
     * @brief Implementation of a physics material defining surface properties.
     *
     * This class specifies physical properties such as friction and restitution,
     * which determine how physics bodies interact during collisions.
     */
    class CPhysicsMaterial3 : public IPhysicsMaterial3
    {
    public:
        CPhysicsMaterial3();
        ~CPhysicsMaterial3() override;

        /** @brief Gets the friction coefficient for a given direction. */
        f32 getFriction( s32 direction ) const override;

        /** @brief Sets the friction coefficient for a given direction. */
        void setFriction( f32 friction, s32 direction ) override;

        /** @brief Gets the dynamic friction coefficient for a given direction. */
        f32 getDynamicFriction( s32 direction ) const override;

        /** @brief Sets the dynamic friction coefficient for a given direction. */
        void setDynamicFriction( f32 friction, s32 direction ) override;

        /** @brief Gets the static friction coefficient for a given direction. */
        f32 getStaticFriction( s32 direction ) const override;

        /** @brief Sets the static friction coefficient for a given direction. */
        void setStaticFriction( f32 friction, s32 direction ) override;

        /** @brief Gets the coefficient of restitution (bounciness). */
        f32 getRestitution() const override;

        /** @brief Sets the coefficient of restitution. */
        void setRestitution( f32 restitution ) override;

        /** @brief Gets the rolling friction coefficient (spheres/capsules). */
        f32 getRollingFriction() const override;

        /** @brief Sets the rolling friction coefficient (clamped to [0,1]). */
        void setRollingFriction( f32 friction ) override;

        /** @brief Gets the friction combine mode. */
        FrictionCombineMode getFrictionCombineMode() const override;

        /** @brief Sets the friction combine mode. */
        void setFrictionCombineMode( FrictionCombineMode mode ) override;

        /** @brief Gets the restitution combine mode. */
        RestitutionCombineMode getRestitutionCombineMode() const override;

        /** @brief Sets the restitution combine mode. */
        void setRestitutionCombineMode( RestitutionCombineMode mode ) override;

        /** @brief Gets the data-driven material name. */
        String getMaterialName() const override;

        /** @brief Sets the data-driven material name. */
        void setMaterialName( const String &name ) override;

        /** @brief Gets the contact position for the last interaction. */
        Vector3<real_Num> getContactPosition() const override;

        /** @brief Gets the contact normal for the last interaction. */
        Vector3<real_Num> getContactNormal() const override;

        /** @brief Gets the first physics body involved in the contact. */
        SmartPtr<IRigidBody3> getPhysicsBodyA() const override;

        /** @brief Gets the second physics body involved in the contact. */
        SmartPtr<IRigidBody3> getPhysicsBodyB() const override;

        /** @brief Saves the material properties to a file. */
        void saveToFile( const String &filePath ) override;

        /** @brief Loads the material properties from a file. */
        void loadFromFile( const String &filePath ) override;

        /** @brief Saves the current state of the material. */
        void save() override;

        /** @brief Imports the material data. */
        void import() override;

        /** @brief Re-imports the material data to refresh the current state. */
        void reimport() override;

        /** @brief Gets the unique filesystem identifier of the material. */
        UUID getFileSystemId() const override;

        /** @brief Sets the unique filesystem identifier. */
        void setFileSystemId( UUID id ) override;

        /** @brief Gets the path to the file where this material is stored. */
        String getFilePath() const override;

        /** @brief Sets the file path for this material. */
        void setFilePath( const String &filePath ) override;

        /** @brief Gets the filesystem ID specifically for the settings. */
        UUID getSettingsFileSystemId() const override;

        /** @brief Sets the filesystem ID for the settings. */
        void setSettingsFileSystemId( UUID id ) override;

        /** @brief Internal method to retrieve the underlying physics engine material object. */
        void _getObject( void **ppObject ) const override;

        /** @brief Returns a list of resources this material depends on. */
        Array<SmartPtr<IResource>> getDependencies() const override;

        /** @brief Gets a raw pointer to the associated resource manager. */
        IResourceManager *getResourceManagerPtr() const override;

        /** @brief Gets the smart pointer to the associated resource manager. */
        SmartPtr<IResourceManager> getResourceManager() const override;

        /** @brief Sets the resource manager associated with this material. */
        void setResourceManager( SmartPtr<IResourceManager> resourceManager ) override;

        /** @brief Gets a raw pointer to the state context. */
        IStateContext *getStateContextPtr() const override;

        /** @brief Gets the smart pointer to the state context. */
        SmartPtr<IStateContext> getStateContext() const override;

        /** @brief Processes a state message. Returns true if the message was handled. */
        bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

        /** @brief Handles updates to the state. Returns true if changes occurred. */
        bool handleStateChanged( SmartPtr<IState> &state ) override;

        /** @brief Gets the prototype this material is derived from. */
        SmartPtr<core::IPrototype> getParentPrototype() const override;

        /** @brief Sets the parent prototype for this material. */
        void setParentPrototype( SmartPtr<core::IPrototype> prototype ) override;

        /** @brief Gets the generalized properties of the material. */
        SmartPtr<Properties> getProperties() const override;

        /** @brief Sets the generalized properties of the material. */
        void setProperties( SmartPtr<Properties> properties ) override;

        /** @brief Internal helper to retrieve the underlying physics material pointer. */
        wp_physics_material *getMaterial() const;

    private:
        wp_physics_material       *m_material = nullptr; ///< Underlying physics engine material pointer.
        UUID                       m_fileSystemId;       ///< Unique identifier for filesystem tracking.
        UUID                       m_settingsFileSystemId; ///< Unique identifier for settings tracking.
        String                     m_filePath;             ///< Path to the material file on disk.
        SmartPtr<IResourceManager> m_resourceManager;      ///< Manager handling the material resource.
        SmartPtr<IStateContext>    m_stateContext;    ///< Context used for state tracking and updates.
        SmartPtr<core::IPrototype> m_parentPrototype; ///< The prototype source for this material.
        SmartPtr<Properties>       m_properties; ///< General properties associated with the material.
    };
} // namespace workphone::physics

#endif
