#ifndef IRoadSection_h__
#define IRoadSection_h__

#include <Workphone/Interface/Procedural/IProceduralObject.hpp>

namespace workphone
{
    namespace procedural
    {

        /** A section of a road that contains multiple road elements.
         * This interface allows for the management of road elements within a section.
         */
        class WPCore_API IRoadSection : public IProceduralObject
        {
        public:
            ~IRoadSection() override = default;

            virtual void addRoadElement( SmartPtr<IRoadElement> roadElement ) = 0;
            virtual void removeRoadElement( SmartPtr<IRoadElement> roadElement ) = 0;
            virtual void clearRoadElements() = 0;

            virtual Array<SmartPtr<IRoadElement>> getElements() const = 0;
            virtual void setElements( const Array<SmartPtr<IRoadElement>> &elements ) = 0;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // IRoadSection_h__
