#ifndef IBlockGenerator_h__
#define IBlockGenerator_h__

#include <Workphone/Interface/Procedural/IProceduralGenerator.hpp>

namespace workphone
{
    namespace procedural
    {
        /**
         * @brief Interface for generating city blocks procedurally.
         *
         * Inherits from IProceduralGenerator and provides methods for block generation
         * and city association.
         */
        class WPCore_API IBlockGenerator : public IProceduralGenerator
        {
        public:
            /** Virtual destructor. */
            ~IBlockGenerator() override = default;

            /**
             * @brief Generates the city blocks.
             */
            void generate() override = 0;

            /**
             * @brief Gets the associated procedural city.
             */
            virtual SmartPtr<IProceduralCity> getCity() const = 0;

            /**
             * @brief Sets the associated procedural city.
             */
            virtual void setCity( SmartPtr<IProceduralCity> city ) = 0;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // IBlockGenerator_h__
