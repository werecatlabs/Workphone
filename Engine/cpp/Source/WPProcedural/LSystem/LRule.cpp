#include "WPProcedural/WPProceduralPCH.hpp"
#include <WPProcedural/LSystem/LRule.hpp>
#include <iostream>

namespace workphone
{
    namespace procedural
    {
        LRule::LRule() = default;

        LRule::LRule( const String &pre, const String &succ )
        {
            m_predecessor = pre;
            m_successor = succ;
        }

        void LRule::print()
        {
            std::cout << "\t\t\t\t" << m_predecessor << "->" << m_successor << std::endl;
        }

        String LRule::getPredecessor() const
        {
            return m_predecessor;
        }

        void LRule::setPredecessor( const String &predecessor )
        {
            m_predecessor = predecessor;
        }

        String LRule::getSuccessor() const
        {
            return m_successor;
        }

        void LRule::setSuccessor( const String &successor )
        {
            m_successor = successor;
        }

        bool operator==( LRule &rule1, LRule &rule2 )
        {
            return ( rule1.m_predecessor == rule2.m_predecessor &&
                     rule1.m_successor == rule2.m_successor );
        }
    }  // namespace procedural
}  // namespace workphone
