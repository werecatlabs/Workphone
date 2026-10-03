#ifndef IProceduralModelRule_h__
#define IProceduralModelRule_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace procedural
    {
        class IProceduralMesh;

        /**
         * @brief A rule that contributes geometry to a procedural mesh.
         *
         * Rules append to the supplied mesh rather than clearing it, which allows composite rules
         * to build a model by invoking multiple child rules in sequence.
         */
        class WPCore_API IProceduralModelRule : public ISharedObject
        {
        public:
            ~IProceduralModelRule() override;

            /** Builds this rule's geometry into @p mesh. */
            virtual void build( SmartPtr<IProceduralMesh> mesh ) const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace procedural
}  // namespace workphone

#endif  // IProceduralModelRule_h__
