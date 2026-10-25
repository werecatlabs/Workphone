#ifndef WP_PROPERTY_PATH_HPP
#define WP_PROPERTY_PATH_HPP

#include <Workphone/Core/Properties.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/System/SourceBudgetContainers.hpp>
#include <stdexcept>

namespace workphone
{
    struct PropertyGroupStep
    {
        String name;
        u32 occurrence = 0;
    };
    using PropertyGroupPath = SourceBudgetArray<PropertyGroupStep>;

    namespace property_path_detail
    {
        inline bool find( SmartPtr<Properties> root, SmartPtr<Properties> group,
                          PropertyGroupPath &path )
        {
            if( root == group ) return true;
            if( path.size() >= 256 ) return false;
            const auto children = root->getChildren();
            for( size_t i = 0; i < children.size(); ++i )
            {
                auto child = children[i];
                if( !child ) continue;
                u32 occurrence = 0;
                const auto name = child->getName();
                for( size_t j = 0; j < i; ++j )
                    if( children[j] && children[j]->getName() == name ) ++occurrence;
                path.push_back( { name, occurrence } );
                if( find( child, group, path ) ) return true;
                path.pop_back();
            }
            return false;
        }

        inline SmartPtr<Properties> clone( SmartPtr<Properties> source, u32 depth )
        {
            if( !source ) return nullptr;
            if( depth > 256 ) throw std::length_error( "Property hierarchy exceeds its depth limit" );
            auto copy = make_ptr<Properties>();
            copy->setName( source->getName() );
            copy->setPropertiesAsArray( source->getPropertiesAsArray() );
            for( auto child : source->getChildren() )
                if( child ) copy->addChild( clone( child, depth + 1 ) );
            return copy;
        }
    }

    /** Identify the group by its position in the displayed hierarchy. Repeated
     * names are distinguished by their occurrence among same-named siblings. */
    inline bool findPropertyGroupPath( SmartPtr<Properties> root, SmartPtr<Properties> group,
                                       PropertyGroupPath &path )
    {
        path.clear();
        return root && group && property_path_detail::find( root, group, path );
    }

    /** Replay a displayed group path against a target's fresh property snapshot. */
    inline SmartPtr<Properties> resolvePropertyGroup( SmartPtr<Properties> root,
                                                      const PropertyGroupPath &path )
    {
        for( const auto &step : path )
        {
            if( !root ) return nullptr;
            SmartPtr<Properties> found;
            u32 occurrence = 0;
            for( auto child : root->getChildren() )
                if( child && child->getName() == step.name && occurrence++ == step.occurrence )
                {
                    found = child;
                    break;
                }
            root = found;
        }
        return root;
    }

    /** Properties' copy constructor shares children. Edits need independent
     * descendants so a setter failure cannot alter the original snapshot. */
    inline SmartPtr<Properties> clonePropertyTree( SmartPtr<Properties> source )
    {
        return property_path_detail::clone( source, 0 );
    }
}

#endif
