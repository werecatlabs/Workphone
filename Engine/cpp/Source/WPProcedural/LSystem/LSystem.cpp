#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/LSystem/LSystem.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>

namespace workphone
{
    namespace procedural
    {
        namespace
        {
            template <class T>
            bool removeValues( Array<T> &values, const T &value )
            {
                if( std::find( values.begin(), values.end(), value ) == values.end() ) return false;
                auto candidate = makeSourceBudgetSnapshot<T>();
                candidate.reserve( values.capacity() );
                for( const auto &entry : values )
                {
                    if( !( entry == value ) ) candidate.push_back( entry );
                }
                values.swap( candidate );
                return true;
            }

            struct RewriteGuard
            {
                bool &active;
                ~RewriteGuard() { active = false; }
            };
        }

        LSystem::LSystem() = default;

        LSystem::~LSystem()
        {
            unload( nullptr );
        }

        void LSystem::unload( SmartPtr<ISharedObject> data )
        {
            ++m_generation;
            // All live fields are detached before the local rule inventory can
            // release a last owner and reenter this system.
            auto oldRules = std::move( rules );
            auto oldVariables = std::move( variables );
            auto oldConstants = std::move( constants );
            String oldStart;
            String oldCurrent;
            oldStart.swap( start );
            oldCurrent.swap( curString );
            level = 0;
            ILSystem::unload( data );
        }

        void LSystem::reset()
        {
            String candidate( start );
            ++m_generation;
            curString.swap( candidate );
            level = 0;
        }

        void LSystem::addVariable( const String &var )
        {
            variables.push_back( var );
            ++m_generation;
        }

        void LSystem::removeVariable( const String &var )
        {
            if( removeValues( variables, var ) ) ++m_generation;
        }

        void LSystem::printVariables()
        {
            std::cout << "LSys variables:" << std::endl;
            for( const auto &value : variables ) std::cout << "\t\t\t\t" << value << std::endl;
        }

        void LSystem::addConstant( const String &cons )
        {
            constants.push_back( cons );
            ++m_generation;
        }

        void LSystem::removeConstant( const String &cons )
        {
            if( removeValues( constants, cons ) ) ++m_generation;
        }

        void LSystem::printConstants()
        {
            std::cout << "LSys constants:" << std::endl;
            for( const auto &value : constants ) std::cout << "\t\t\t\t" << value << std::endl;
        }

        void LSystem::addRule( SmartPtr<ILSystemRule> rule )
        {
            rules.push_back( rule );
            ++m_generation;
        }

        void LSystem::removeRule( SmartPtr<ILSystemRule> rule )
        {
            if( removeValues( rules, rule ) ) ++m_generation;
        }

        void LSystem::printRules()
        {
            if( m_rewriting )
            {
                ++m_generation;
                throw std::logic_error( "LSystem rule callbacks are already active" );
            }
            m_rewriting = true;
            RewriteGuard guard{ m_rewriting };
            const auto generation = m_generation;
            const auto denialEpoch = sourceBudgetDenialCount();
            auto owners = makeSourceBudgetSnapshot( rules );
            std::cout << "LSys rules:" << std::endl;
            for( auto rule : owners )
            {
                // Null entries retain their legacy inventory identity but do
                // not participate in rewriting or debugging callbacks.
                if( rule ) rule->print();
                checkEvaluation( generation, denialEpoch );
            }
        }

        void LSystem::setStart( const String &value )
        {
            String nextStart( value );
            String nextCurrent( value );
            ++m_generation;
            start.swap( nextStart );
            curString.swap( nextCurrent );
            level = 0;
        }

        void LSystem::printStart()
        {
            std::cout << "LSys start:" << std::endl;
            std::cout << "\t\t\t\t" << start << std::endl;
        }

        void LSystem::checkEvaluation( u64 generation, u64 denialEpoch ) const
        {
            if( generation != m_generation )
                throw std::logic_error( "LSystem changed during a rule callback" );
            if( sourceBudgetDenialCount() != denialEpoch ) throw std::bad_alloc();
        }

        String LSystem::rewrite( const String &input, const Array<SmartPtr<ILSystemRule>> &owners,
                                 u64 generation, u64 denialEpoch ) const
        {
            auto pieces = makeSourceBudgetSnapshot<String>();
            pieces.reserve( input.size() );
            String result;
            size_t resultLength = 0;
            for( size_t i = 0; i < input.size(); ++i )
            {
                String replacement( input.data() + i, 1 );
                for( const auto &rule : owners )
                {
                    if( !rule ) continue;
                    auto predecessor = rule->getPredecessor();
                    checkEvaluation( generation, denialEpoch );
                    // This is the existing byte-per-symbol dialect: a rule
                    // with a multi-byte predecessor does not match one symbol.
                    if( predecessor.size() == 1 && predecessor[0] == input[i] )
                    {
                        replacement = rule->getSuccessor();
                        checkEvaluation( generation, denialEpoch );
                        break;
                    }
                }
                if( replacement.size() > result.max_size() - resultLength )
                    throw std::length_error( "LSystem rewritten string exceeds max_size" );
                resultLength += replacement.size();
                pieces.push_back( std::move( replacement ) );
            }
            result.reserve( resultLength );
            for( const auto &piece : pieces ) result.append( piece.data(), piece.size() );
            return result;
        }

        String LSystem::evaluate( s32 targetLevel, bool fromStart )
        {
            if( m_rewriting )
            {
                // Invalidate the outer run even if a custom rule catches this
                // rejection and returns an apparently usable replacement.
                ++m_generation;
                throw std::logic_error( "LSystem evaluation is already active" );
            }
            m_rewriting = true;
            RewriteGuard guard{ m_rewriting };
            const auto generation = m_generation;
            const auto denialEpoch = sourceBudgetDenialCount();
            String candidate( fromStart ? start : curString );
            const s32 firstLevel = fromStart ? 0 : level;
            auto owners = makeSourceBudgetSnapshot<SmartPtr<ILSystemRule>>();
            if( firstLevel < targetLevel && !candidate.empty() ) owners = makeSourceBudgetSnapshot( rules );
            for( s32 current = firstLevel; current < targetLevel; ++current )
            {
                candidate = rewrite( candidate, owners, generation, denialEpoch );
                checkEvaluation( generation, denialEpoch );
            }
            // Admit the caller's independently owned return value before any
            // publication; a failed return copy cannot advance the live state.
            String result( candidate );
            checkEvaluation( generation, denialEpoch );
            ++m_generation;
            curString.swap( candidate );
            level = targetLevel;
            return result;
        }

        String LSystem::getNextLevel()
        {
            if( level < 0 || level == std::numeric_limits<s32>::max() )
                throw std::overflow_error( "LSystem level cannot advance beyond s32 range" );
            return evaluate( level + 1, false );
        }

        String LSystem::getLevel( s32 targetLevel )
        {
            if( targetLevel < 0 ) throw std::invalid_argument( "LSystem level must be non-negative" );
            return evaluate( targetLevel, true );
        }
    }  // namespace procedural
}  // namespace workphone
