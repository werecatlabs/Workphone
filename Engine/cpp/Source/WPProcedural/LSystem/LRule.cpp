#include "WPProcedural/WPProceduralPCH.hpp"
#include <WPProcedural/LSystem/LRule.hpp>
#include <iostream>

namespace workphone
{
    namespace procedural
    {
        LRule::LRule() = default;

        LRule::LRule( const String &pre, const String &succ ) :
            m_predecessor( pre ), m_successor( succ )
        {
        }

        LRule::~LRule()
        {
            unload( nullptr );
        }

        void LRule::unload( SmartPtr<ISharedObject> data )
        {
            String predecessor;
            String successor;
            predecessor.swap( m_predecessor );
            successor.swap( m_successor );
            ILSystemRule::unload( data );
        }

        void LRule::print() const
        {
            std::cout << "\t\t\t\t" << m_predecessor << "->" << m_successor << std::endl;
        }

        String LRule::getPredecessor() const
        {
            return m_predecessor;
        }

        void LRule::setPredecessor( const String &predecessor )
        {
            String candidate( predecessor );
            m_predecessor.swap( candidate );
        }

        String LRule::getSuccessor() const
        {
            return m_successor;
        }

        void LRule::setSuccessor( const String &successor )
        {
            String candidate( successor );
            m_successor.swap( candidate );
        }

        bool operator==( LRule &rule1, LRule &rule2 )
        {
            return ( rule1.m_predecessor == rule2.m_predecessor &&
                     rule1.m_successor == rule2.m_successor );
        }
    }  // namespace procedural
}  // namespace workphone
