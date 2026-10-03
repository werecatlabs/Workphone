#ifndef ILSystemRule_h__
#define ILSystemRule_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    namespace procedural
    {

        /**
         * @brief Interface for an L-system production rule.
         *
         * An L-system rule maps a predecessor symbol (or symbol sequence)
         * to a successor string used when expanding an L-system. Implementations
         * provide accessors for the predecessor and successor and can expose
         * additional behaviour such as debug printing.
         *
         * This is an abstract shared object; concrete rules should inherit
         * from this interface and implement the pure virtual methods.
         */
        class WPCore_API ILSystemRule : public ISharedObject
        {
        public:
            /**
             * @brief Virtual destructor.
             *
             * Ensures derived classes are correctly destroyed through this interface.
             */
            ~ILSystemRule() override;

            /**
             * @brief Get the rule predecessor.
             *
             * The predecessor is the symbol (or symbol sequence) that this rule
             * matches when the L-system is being expanded.
             *
             * @return The predecessor as a `String`.
             */
            virtual String getPredecessor() const = 0;

            /**
             * @brief Set the rule predecessor.
             *
             * @param predecessor The symbol or symbol sequence that the rule will match.
             */
            virtual void setPredecessor( const String &predecessor ) = 0;

            /**
             * @brief Get the rule successor.
             *
             * The successor is the replacement string that will substitute the predecessor
             * during L-system expansion. It may contain terminals, non-terminals, or
             * embedded parameters depending on the L-system dialect in use.
             *
             * @return The successor as a `String`.
             */
            virtual String getSuccessor() const = 0;

            /**
             * @brief Set the rule successor.
             *
             * @param successor The replacement string used when this rule is applied.
             */
            virtual void setSuccessor( const String &successor ) = 0;

            /**
             * @brief Print a human readable representation of the rule.
             *
             * Implementations should output useful debug information, typically the
             * predecessor and successor in a concise format (for example "`A -> AB`").
             * This is intended for logging and debugging and may write to console,
             * a log system or other debug sinks as appropriate.
             */
            virtual void print() = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace procedural
}  // namespace workphone

#endif  // ILSystemRule_h__
