#ifndef ILSystem_h__
#define ILSystem_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    namespace procedural
    {

        /**
         * @brief Interface for an L-system (Lindenmayer system) representation.
         *
         * This interface models an L-system used for procedural generation.
         * Implementations manage the set of variables, constants, production rules,
         * and the current axiom (start string). The class provides methods to
         * mutate the grammar (add/remove variables, constants, rules), inspect
         * the current configuration (print methods) and generate subsequent levels.
         *
         * Lifetime:
         * - Inherits from ISharedObject; implementations should respect the shared
         *   ownership semantics provided by the project's smart-pointer conventions.
         *
         * Usage notes:
         * - `setStart` sets the initial axiom.
         * - `getNextLevel` applies the production rules to the current level and
         *   returns the next generated string.
         * - `getLevel(level)` should return the string at the requested generation
         *   index (implementation-defined whether levels are cached or generated on demand).
         *
         * @ingroup Procedural
         */
        class WPCore_API ILSystem : public ISharedObject
        {
        public:
            /**
             * @brief Virtual destructor.
             *
             * Ensures derived classes are destroyed correctly through this interface.
             */
            ~ILSystem() override;

            /**
             * @brief Reset the L-system to an empty/default state.
             *
             * Implementations should clear variables, constants, rules and the start
             * axiom (or reset to a well-defined default).
             */
            virtual void reset() = 0;

            /**
             * @brief Add a symbol to the variable alphabet.
             *
             * Variables are symbols that can be replaced by production rules.
             *
             * @param var Symbol to add (as a String).
             */
            virtual void addVariable( const String &var ) = 0;

            /**
             * @brief Remove a symbol from the variable alphabet.
             *
             * Removing a variable typically prevents it from being matched by rules.
             * Implementations may also remove any rules that reference the variable,
             * or leave rules unchanged — behaviour should be documented in the concrete class.
             *
             * @param var Symbol to remove.
             */
            virtual void removeVariable( const String &var ) = 0;

            /**
             * @brief Print the current set of variables to the implementation's logging/output.
             *
             * Intended for debugging and inspection. How output is delivered depends on the concrete
             * implementation.
             */
            virtual void printVariables() = 0;

            /**
             * @brief Add a constant symbol.
             *
             * Constants are symbols that are not replaced by rules (often used for drawing commands).
             *
             * @param cons Constant symbol to add.
             */
            virtual void addConstant( const String &cons ) = 0;

            /**
             * @brief Remove a constant symbol.
             *
             * @param cons Constant symbol to remove.
             */
            virtual void removeConstant( const String &cons ) = 0;

            /**
             * @brief Print the current set of constants to the implementation's logging/output.
             */
            virtual void printConstants() = 0;

            /**
             * @brief Add a production rule to the system.
             *
             * Rules define how variables are rewritten during generation.
             *
             * @param rule Smart pointer to an ILSystemRule instance describing the production.
             */
            virtual void addRule( SmartPtr<ILSystemRule> rule ) = 0;

            /**
             * @brief Remove a production rule from the system.
             *
             * @param rule Smart pointer to the rule to remove. Equality semantics are
             * determined by the concrete rule type / SmartPtr comparison.
             */
            virtual void removeRule( SmartPtr<ILSystemRule> rule ) = 0;

            /**
             * @brief Print the set of production rules to the implementation's logging/output.
             */
            virtual void printRules() = 0;

            /**
             * @brief Set the starting axiom (initial string) for the L-system.
             *
             * The axiom is the seed from which successive generations are derived.
             *
             * @param start The axiom string.
             */
            virtual void setStart( const String &start ) = 0;

            /**
             * @brief Print the current start/axiom string.
             */
            virtual void printStart() = 0;

            /**
             * @brief Generate and return the next level (next generation) of the system.
             *
             * This method should apply production rules to the current level/axiom
             * and return the resulting string. It may also advance internal state
             * so subsequent calls continue from the new level.
             *
             * @return The generated next-level string.
             */
            virtual String getNextLevel() = 0;

            /**
             * @brief Return the system string at a specific generation index.
             *
             * Implementations may compute levels on demand or retrieve them from a cache.
             *
             * @param level Generation index (0 for the axiom, 1 for the first application of rules,
             * etc.).
             * @return The string at the requested level. Behaviour for out-of-range indices is
             * implementation-defined (commonly an empty string or the last available level).
             */
            virtual String getLevel( s32 level ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace procedural
}  // namespace workphone

#endif  // ILSystem_h__
