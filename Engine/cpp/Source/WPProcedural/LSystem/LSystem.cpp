#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/LSystem/LSystem.hpp"
#include <Workphone/Workphone.hpp>
#include <iostream>

namespace workphone
{
    namespace procedural
    {
        LSystem::LSystem()
        {
            level = 0;
        }

        void LSystem::reset()
        {
            level = 0;
            curString = start;
        }

        void LSystem::addVariable( const String &var )
        {
            variables.push_back( var );
        }

        void LSystem::removeVariable( const String &var )
        {
            for( int i = 0; i < variables.size(); i++ )
            {
                if( variables[i] == var )
                    variables.erase( variables.begin() + i );
            }
        }

        void LSystem::printVariables()
        {
            std::cout << "LSys variables:" << std::endl;
            for( int i = 0; i < variables.size(); i++ )
            {
                std::cout << "\t\t\t\t" << variables[i] << std::endl;
            }
        }

        void LSystem::addConstant( const String &cons )
        {
            constants.push_back( cons.c_str() );
        }

        void LSystem::removeConstant( const String &cons )
        {
            for( size_Num i = 0; i < constants.size(); i++ )
            {
                if( StringUtil::isEqual( constants[i], cons ) )
                {
                    constants.erase( constants.begin() + i );
                }
            }
        }

        void LSystem::printConstants()
        {
            std::cout << "LSys constants:" << std::endl;
            for( int i = 0; i < constants.size(); i++ )
            {
                std::cout << "\t\t\t\t" << constants[i] << std::endl;
            }
        }

        void LSystem::addRule( SmartPtr<ILSystemRule> rule )
        {
            rules.push_back( rule );
        }

        void LSystem::removeRule( SmartPtr<ILSystemRule> rule )
        {
            for( int i = 0; i < rules.size(); i++ )
            {
                if( rules[i] == rule )
                    rules.erase( rules.begin() + i );
            }
        }

        void LSystem::printRules()
        {
            std::cout << "LSys rules:" << std::endl;
            for( int i = 0; i < rules.size(); i++ )
            {
                rules[i]->print();
            }
        }

        void LSystem::setStart( const String &_start )
        {
            start = _start;
            curString = start;
        }

        void LSystem::printStart()
        {
            std::cout << "LSys start:" << std::endl;
            std::cout << "\t\t\t\t" << start << std::endl;
        }

        String LSystem::getNextLevel()
        {
            auto length = curString.length();  // length of the current string

            Array<String> substr;
            substr.resize( length );  // split into 1-char substrings

            for( int i = 0; i < length; i++ )
            {
                substr[i] = curString.substr( i, 1 );
            }

            for( int i = 0; i < length; i++ )
            {
                // apply all rules
                for( int j = 0; j < rules.size(); j++ )
                {
                    if( substr[i] == rules[j]->getPredecessor() )
                    {
                        substr[i] = rules[j]->getSuccessor();
                        j = rules.size();  // if one rule is applied, skip rest of rules
                    }
                }
            }

            String result;  // merge into resulting string
            for( int i = 0; i < length; i++ )
            {
                result.append( substr[i] );
            }
            curString = result;

            level++;
            return curString;  // return current result
        }

        String LSystem::getLevel( s32 _level )
        {
            curString = start;

            String result;

            for( int i = 0; i < _level; i++ )
            {
                result = getNextLevel();
            }

            return result.c_str();
        }
    }  // namespace procedural
}  // namespace workphone
