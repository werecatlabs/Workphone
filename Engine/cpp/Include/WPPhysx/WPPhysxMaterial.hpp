#ifndef WPPhysxMaterial_h__
#define WPPhysxMaterial_h__

#include <WPPhysx/WPPhysxPrerequisites.hpp>
#include <Workphone/Physics/PhysicsMaterial3.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief PhysX-backed implementation of the PhysicsMaterial3 interface.
         *
         * This class wraps a `physx::PxMaterial` and exposes it through the
         * engine's `PhysicsMaterial3` interface so the rest of the codebase can
         * interact with physics materials without depending on PhysX types.
         */
        class PhysxMaterial : public PhysicsMaterial3
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Constructs an empty `PhysxMaterial`. The internal PxMaterial
             * pointer is initially null and must be set via `setMaterial`
             * before use.
             */
            PhysxMaterial();

            /**
             * @brief Destructor.
             *
             * Releases references held by this wrapper. It does not directly
             * manage the lifetime of the underlying PhysX material unless the
             * material is reference-counted by the RawPtr wrapper semantics.
             */
            ~PhysxMaterial() override;

            /**
             * @brief Access the underlying PhysX material pointer.
             *
             * @return RawPtr to the `physx::PxMaterial` held by this object.
             *         May be null if no material has been assigned.
             */
            RawPtr<physx::PxMaterial> getMaterial() const;

            /**
             * @brief Set the underlying PhysX material pointer.
             *
             * The wrapper will use the provided `physx::PxMaterial` for
             * queries and operations. Ownership semantics follow the
             * `RawPtr`/project conventions; the caller is responsible for
             * ensuring the pointer remains valid while in use.
             *
             * @param material RawPtr to a `physx::PxMaterial` to associate
             *                 with this wrapper. May be null to clear it.
             */
            void setMaterial( RawPtr<physx::PxMaterial> material );

            /**
             * @copydoc IPhysicsMaterial3::getProperties
             *
             * Retrieves the material properties (friction, restitution, etc.)
             * from the underlying PhysX material and returns them as the
             * engine `Properties` object.
             *
             * @return SmartPtr to a `Properties` object describing the
             *         material. May be null if no properties are available.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @copydoc IPhysicsMaterial3::setProperties
             *
             * Applies the provided `Properties` to the underlying PhysX
             * material. If no PhysX material is set this call may be a no-op
             * or may create a material depending on implementation.
             *
             * @param properties SmartPtr to the properties to apply. May be
             *                   null to reset properties to defaults.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /** The PhysX material. */
            RawPtr<physx::PxMaterial> m_material;
        };

    } // end namespace physics
} // namespace workphone

#endif // WPPhysxMaterial_h__
