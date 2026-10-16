#include <WPLuabind/WPLuabindPCH.hpp>
#include <WPLuabind/Helpers/TerrainHelper.hpp>

namespace workphone
{
    String TerrainHelper::brush( scene::TerrainSystem *terrain, const String &mode, f32 x, f32 z,
                                 f32 radius, f32 strength, f32 target )
    {
        scene::TerrainBrush brush;
        if( mode == "raise" )
        {
            brush.mode = scene::TerrainBrushMode::Raise;
        }
        else if( mode == "lower" )
        {
            brush.mode = scene::TerrainBrushMode::Lower;
        }
        else if( mode == "smooth" )
        {
            brush.mode = scene::TerrainBrushMode::Smooth;
        }
        else if( mode == "flatten" )
        {
            brush.mode = scene::TerrainBrushMode::Flatten;
        }
        else
        {
            return "This terrain brush is unavailable.";
        }
        brush.centre = Vector2F( x, z );
        brush.radius = radius;
        brush.strength = strength;
        brush.targetHeight = target;
        auto application = core::IApplicationManager::instancePtr();
        String error;
        scene::applyTerrainBrush( terrain, brush,
                                  application ? application->getCommandManager() : nullptr, error );
        return error;
    }

    String TerrainHelper::save( scene::TerrainSystem *terrain, const String &path )
    {
        String error;
        scene::saveTerrainDataFile( terrain, path, error );
        return error;
    }

    String TerrainHelper::load( scene::TerrainSystem *terrain, const String &path )
    {
        String error;
        scene::loadTerrainDataFile( terrain, path, error );
        return error;
    }

    String TerrainHelper::importData( scene::TerrainSystem *terrain, const String &data )
    {
        String error;
        if( terrain )
        {
            terrain->importTerrainData( data, error );
        }
        else
        {
            error = "No terrain selected.";
        }
        return error;
    }
}  // namespace workphone
