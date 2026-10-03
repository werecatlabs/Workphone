
#pragma once

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Interface/Procedural/ILSystemRule.hpp>
#include <string>

/**
 * @file LRule.hpp
 * @brief L-system rewriting rule declaration.
 *
 * This file declares the workphone::procedural::LRule class which models a
 * single production rule in an L-system: a predecessor is replaced by a
 * successor string during rewriting.
 */

namespace workphone
{
    namespace procedural
    {

        /**
         * @brief Represents a single L-system rewriting rule.
         *
         * An LRule holds a predecessor (left-hand side) and a successor
         * (right-hand side). It is a lightweight value-type used by the
         * procedural L-system implementation.
         */
        class WPProcedural_API LRule : public ILSystemRule
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Constructs an empty rule where both predecessor and successor
             * are empty strings.
             */
            LRule();

            /**
             * @brief Construct a rule with given predecessor and successor.
             *
             * @param pre The predecessor (left-hand side) of the rule.
             * @param succ The successor (right-hand side) of the rule.
             */
            LRule( const String &pre, const String &succ );

            /**
             * @brief Print a human-readable representation of the rule.
             *
             * Primarily intended for debugging/logging. Typical output is
             * the form "predecessor -> successor".
             */
            void print();

            /**
             * @brief Equality comparison for two LRule instances.
             *
             * Two rules are equal when both their predecessor and successor
             * strings are equal.
             *
             * @param rule1 Left operand.
             * @param rule2 Right operand.
             * @return true if equal, false otherwise.
             */
            friend bool operator==( LRule &rule1, LRule &rule2 );

            /**
             * @brief Get the predecessor of the rule.
             * @return The predecessor string.
             */
            String getPredecessor() const;

            /**
             * @brief Set the predecessor of the rule.
             * @param predecessor New predecessor string.
             */
            void setPredecessor( const String &predecessor );

            /**
             * @brief Get the successor of the rule.
             * @return The successor string.
             */
            String getSuccessor() const;

            /**
             * @brief Set the successor of the rule.
             * @param successor New successor string.
             */
            void setSuccessor( const String &successor );

        protected:
            /**
             * @brief Left-hand side of the rule.
             */
            String m_predecessor;

            /**
             * @brief Right-hand side of the rule.
             */
            String m_successor;
        };

    }  // namespace procedural
}  // namespace workphone
