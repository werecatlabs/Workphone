#ifndef SkySphere_h__
#define SkySphere_h__

#include <Workphone/Interface/Graphics/ISkySphere.hpp>
#include <Workphone/Graphics/Sky.hpp>
#include <Workphone/Graphics/SharedGraphicsObject.hpp>

/**
 * @file SkySphere.hpp
 * @brief Declaration of the spherical sky rendering object.
 */
namespace workphone
{
    namespace render
    {

        /**
         * @brief Sky implementation using a sphere.
         *
         * `SkySphere` is a concrete sky object that uses a spherical geometry
         * to represent the sky dome. It inherits common sky behaviour from
         * the `Sky` template, specialised with the `ISkySphere` interface.
         *
         * This class intentionally remains lightweight: renderer-specific
         * initialization and resource management should be handled by the
         * graphics backend or scene setup code that creates and configures
         * instances of this type.
         *
         * @see Sky
         * @see ISkySphere
         */
        class WPCore_API SkySphere : public Sky<ISkySphere>
        {
        public:
            /**
             * @brief Construct a new SkySphere object.
             *
             * The constructor performs minimal initialization. Any heavy work
             * depending on rendering devices or external systems should be
             * deferred.
             */
            SkySphere();

            /**
             * @brief Destroy the SkySphere object.
             *
             * Virtual destructor to ensure proper cleanup through base
             * pointers to `Sky` / `ISkySphere`.
             */
            ~SkySphere() override;

            /**
             * @brief Engine RTTI / class registration macro.
             *
             * Declares the necessary hooks for the engine's runtime type
             * system so this class can be discovered, reflected and
             * constructed by factories and tooling.
             */
            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace render
}  // namespace workphone

#endif  // SkySphere_h__
