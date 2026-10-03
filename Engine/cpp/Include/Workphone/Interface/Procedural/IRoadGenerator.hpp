#ifndef IRoadGenerator_h__
#define IRoadGenerator_h__

#include <Workphone/Interface/Procedural/IProceduralGenerator.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPCore_API IRoadGenerator : public IProceduralGenerator
        {
        public:
            ~IRoadGenerator() override;

            virtual String getPatternData() const = 0;
            virtual void setPatternData( const String &patternData ) = 0;

            virtual SmartPtr<IProceduralCity> getCity() const = 0;
            virtual void setCity( SmartPtr<IProceduralCity> city ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // IRoadGenerator_h__
