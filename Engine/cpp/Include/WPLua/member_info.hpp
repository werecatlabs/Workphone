#ifndef member_info_h__
#define member_info_h__

#include <luabind/luabind.hpp>
#include <luabind/adopt_policy.hpp>
#include <luabind/detail/object_rep.hpp>
#include <luabind/detail/stack_utils.hpp>

namespace luabind
{
    using namespace detail;

    class member_info
    {
    public:
        member_info()
        {
        }

        member_info( lua_State *L, const object &obj, const object_rep *rep, int index ) :
            m_L( L ),
            m_obj( obj ),
            m_rep( rep ),
            m_index( index )
        {
        }

        const char *name() const
        {
            if( !m_rep )
            {
                throw std::runtime_error( "member_info is null" );
            }

            const char *name = "";  //lua_getupvalue( m_L, m_rep->m_index, m_index + 1 );
            if( !name )
            {
                throw std::runtime_error( "invalid member_info index" );
            }

            return name;
        }

        int type() const
        {
            if( !m_rep )
            {
                throw std::runtime_error( "member_info is null" );
            }

            lua_pushnumber( m_L, m_index + 1 );
            //lua_gettable( m_L, m_rep->m_index );
            int type = lua_type( m_L, -1 );
            lua_pop( m_L, 1 );
            return type;
        }

        bool is_function() const
        {
            return type() == LUA_TFUNCTION;
        }

    private:
        lua_State *m_L;
        object m_obj;
        const object_rep *m_rep;
        int m_index;
    };

}  // namespace luabind

namespace luabind
{

    class iterator_range
    {
    public:
        typedef member_info value_type;
        typedef value_type *pointer;
        typedef value_type &reference;
        typedef std::ptrdiff_t difference_type;
        typedef std::input_iterator_tag iterator_category;

        iterator_range()
        {
        }

        iterator_range( const member_info &first, const member_info &last ) :
            m_first( first ),
            m_last( last )
        {
        }

        pointer begin() const
        {
            return const_cast<pointer>( &m_first );
        }

        pointer end() const
        {
            return const_cast<pointer>( &m_last ) + 1;
        }

    private:
        member_info m_first;
        member_info m_last;
    };

}  // namespace luabind

#endif  // member_info_h__
