#ifndef IProceduralModelSystem_h__
#define IProceduralModelSystem_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace procedural
    {
        class IProceduralMesh;
        class IProceduralModelRule;

        /**
         * @brief Service that evaluates a procedural model rule tree.
         */
        class WPCore_API IProceduralModelSystem : public ISharedObject
        {
        public:
            ~IProceduralModelSystem() override;

            /**
             * Clears @p mesh, evaluates @p rootRule into it and generates its vertex normals.
             * Implementations should leave the mesh empty when no root rule is supplied.
             */
            virtual void bake( SmartPtr<IProceduralModelRule> rootRule,
                               SmartPtr<IProceduralMesh> mesh ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace procedural
}  // namespace workphone

#endif  // IProceduralModelSystem_h__
