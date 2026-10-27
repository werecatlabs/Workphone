
#pragma once

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Interface/Procedural/ILSystem.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include "LRule.hpp"

namespace workphone
{
    namespace procedural
    {

        /**
         * @class LSystem
         * @brief Implements a simple L-system (Lindenmayer system) engine.
         *
         * The LSystem class provides a lightweight implementation of a
         * deterministic L-system used for procedural generation. It stores
         * the current level string, a start axiom, production rules, and
         * sets of variables and constants. Rules are applied to produce
         * subsequent levels of the string.
         *
         * This class implements the ILSystem interface.
         */
        class WPProcedural_API LSystem : public ILSystem, public SourceBudgetObject
        {
        public:
            using SourceBudgetObject::operator new;
            using SourceBudgetObject::operator delete;
            /**
             * @brief Construct a new LSystem object.
             *
             * Initializes internal state (level counter, current string, etc.).
             */
            LSystem();
            ~LSystem() override;
            LSystem( const LSystem & ) = delete;
            LSystem &operator=( const LSystem & ) = delete;

            // Detach all grammar/state storage before retiring rule owners.
            // Externally owned rules are never explicitly unloaded.
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Reset the L-system to its initial state.
             *
             * This resets the current level to 0 and sets the working
             * string back to the start axiom.
             */
            void reset() override;

            /**
             * @brief Add a symbol to the set of variables.
             *
             * Variables are symbols that can be replaced by rules during
             * evolution of the L-system.
             *
             * @param var Symbol (string) to add as a variable.
             */
            void addVariable( const String &var ) override;

            /**
             * @brief Remove a symbol from the set of variables.
             *
             * If the symbol is not present this is a no-op.
             *
             * @param var Symbol (string) to remove.
             */
            void removeVariable( const String &var ) override;

            /**
             * @brief Print the currently registered variables to the
             *        debug/log output.
             */
            void printVariables() override;

            /**
             * @brief Add a symbol to the set of constants.
             *
             * Constants are symbols that are not replaced by rules and remain
             * unchanged through production steps.
             *
             * @param cons Symbol (string) to add as a constant.
             */
            void addConstant( const String &cons ) override;

            /**
             * @brief Remove a symbol from the set of constants.
             *
             * If the symbol is not present this is a no-op.
             *
             * @param cons Symbol (string) to remove.
             */
            void removeConstant( const String &cons ) override;

            /**
             * @brief Print the currently registered constants to the
             *        debug/log output.
             */
            void printConstants() override;

            /**
             * @brief Register a production rule with the system.
             *
             * Rules describe how variables are replaced when advancing
             * the L-system to the next level.
             *
             * @param rule Shared pointer to an ILSystemRule implementation.
             */
            void addRule( SmartPtr<ILSystemRule> rule ) override;

            /**
             * @brief Remove a production rule from the system.
             *
             * If the rule is not found this is a no-op.
             *
             * @param rule Shared pointer to the rule to remove.
             */
            void removeRule( SmartPtr<ILSystemRule> rule ) override;

            /**
             * @brief Print all registered production rules to the
             *        debug/log output.
             */
            void printRules() override;

            /**
             * @brief Set the start axiom for the L-system.
             *
             * The start axiom is the initial string used when the system
             * is reset or when starting production from level 0.
             *
             * @param start The start axiom string.
             */
            void setStart( const String &start ) override;

            /**
             * @brief Print the start axiom to the debug/log output.
             */
            void printStart() override;

            /**
             * @brief Advance the L-system by one level and return the
             *        resulting string.
             *
             * This applies registered rules to the current working string
             * and updates the internal level counter.
             *
             * @return String The string produced for the next level.
             */
            String getNextLevel() override;

            /**
             * @brief Generate and return the string for a specific level.
             *
             * This will (potentially) iterate rules until the requested
             * level is reached and then return that level's string.
             *
             * @param level Target level to generate, starting with the axiom at 0.
             * Negative levels are rejected without changing the current state.
             * @return String The string corresponding to the requested level.
             */
            String getLevel( s32 level ) override;

        protected:
            /**
             * @brief Current level (depth) of the L-system.
             *
             * Level 0 corresponds to the start axiom. Each call to
             * getNextLevel increments this value.
             */
            s32 level = 0;

            /**
             * @brief Start axiom (initial string) for the system.
             */
            String start;

            /**
             * @brief Current working string representing the current level.
             */
            String curString;

            /**
             * @brief Registered production rules.
             */
            Array<SmartPtr<ILSystemRule>> rules{ Allocator<SmartPtr<ILSystemRule>>( sourceBudget ) };

            /**
             * @brief Symbols that can be replaced by rules.
             */
            Array<String> variables{ Allocator<String>( sourceBudget ) };

            /**
             * @brief Symbols that remain unchanged during production.
             */
            Array<String> constants{ Allocator<String>( sourceBudget ) };

        private:
            String evaluate( s32 targetLevel, bool fromStart );
            String rewrite( const String &input, const Array<SmartPtr<ILSystemRule>> &owners,
                            u64 generation, u64 denialEpoch ) const;
            void checkEvaluation( u64 generation, u64 denialEpoch ) const;

            u64 m_generation = 0;
            bool m_rewriting = false;
        };

    }  // namespace procedural
}  // namespace workphone
