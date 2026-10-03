#ifndef IProceduralGenerator_h__
#define IProceduralGenerator_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace procedural
    {
        /**
         * @brief An interface for procedural generation objects.
         *
         * This interface defines methods for generating procedural content,
         * as well as accessing and setting input and output objects and a parent
         * procedural generator object.
         *
         * This interface inherits from the `ISharedObject` interface.
         */
        class WPCore_API IProceduralGenerator : public ISharedObject
        {
        public:
            /**
             * @brief Destructor.
             */
            ~IProceduralGenerator() override;

            /**
             * @brief Gets the file path associated with this generator.
             *
             * @return The file path as a `String`.
             */
            virtual String getFilePath() const = 0;

            /**
             * @brief Sets the file path associated with this generator.
             *
             * @param filePath The file path to set as a `String`.
             */
            virtual void setFilePath( const String &filePath ) = 0;

            /**
             * @brief Generates the procedural content.
             *
             * This method generates the procedural content based on the input
             * and output objects and any parent procedural generator object.
             */
            virtual void generate() = 0;

            /**
             * @brief Returns whether the generation process has finished.
             *
             * @return True if the generation process has finished, false otherwise.
             */
            virtual bool isFinished() const = 0;

            /**
             * @brief Gets the input object associated with this generator.
             *
             * @return A smart pointer to an `IProceduralInput` object.
             */
            virtual SmartPtr<IBuildDirector> getInput() const = 0;

            /**
             * @brief Sets the input object associated with this generator.
             *
             * @param input A smart pointer to an `IProceduralInput` object.
             */
            virtual void setInput( SmartPtr<IBuildDirector> input ) = 0;

            /**
             * @brief Gets the output object associated with this generator.
             *
             * @return A smart pointer to an `IProceduralOutput` object.
             */
            virtual SmartPtr<IBuildDirector> getOutput() const = 0;

            /**
             * @brief Sets the output object associated with this generator.
             *
             * @param output A smart pointer to an `IProceduralOutput` object.
             */
            virtual void setOutput( SmartPtr<IBuildDirector> output ) = 0;

            /**
             * @brief Gets the parent procedural generator object.
             *
             * @return A smart pointer to an `IProceduralGenerator` object.
             */
            virtual SmartPtr<IProceduralGenerator> getParent() const = 0;

            /**
             * @brief Sets the parent procedural generator object.
             *
             * @param parent A smart pointer to an `IProceduralGenerator` object.
             */
            virtual void setParent( SmartPtr<IProceduralGenerator> parent ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace procedural
}  // namespace workphone

#endif  // IProceduralGenerator_h__
