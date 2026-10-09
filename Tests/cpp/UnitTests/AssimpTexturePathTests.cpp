#ifdef WP_ASSIMP_TEXTURE_TEST_STANDALONE
#    define BOOST_TEST_MODULE AssimpTexturePathTests
#    include <boost/test/included/unit_test.hpp>
#else
#    include <boost/test/unit_test.hpp>
#endif
#include <WPAssimp/TexturePathResolver.hpp>
#include <chrono>
#include <fstream>

namespace
{
    namespace fs = std::filesystem;

    struct TextureFolders
    {
        fs::path project = fs::temp_directory_path() /
                           ( "wp-assimp-textures-" + std::to_string(
                               std::chrono::steady_clock::now().time_since_epoch().count() ) );
        fs::path local = project / "Assets" / "Model";

        TextureFolders()
        {
            fs::create_directories( local );
        }

        ~TextureFolders()
        {
            std::error_code error;
            fs::remove_all( project, error );
        }

        fs::path file( const fs::path &relative )
        {
            const auto path = project / relative;
            fs::create_directories( path.parent_path() );
            std::ofstream( path ).put( 'x' );
            return path;
        }
    };
}

BOOST_AUTO_TEST_SUITE( AssimpTexturePathTests )

BOOST_AUTO_TEST_CASE( model_relative_texture_precedes_project_relative_texture )
{
    TextureFolders folders;
    const auto expected = folders.file( "Assets/Model/Textures/albedo.png" );
    folders.file( "Textures/albedo.png" );
    workphone::detail::TexturePathResolver resolver( folders.local, folders.project );
    BOOST_CHECK( resolver.resolve( "Textures/albedo.png" ) == expected );
}

BOOST_AUTO_TEST_CASE( existing_absolute_reference_is_preserved )
{
    TextureFolders folders;
    const auto expected = folders.file( "Shared/albedo.png" );
    folders.file( "Assets/Model/albedo.png" );
    workphone::detail::TexturePathResolver resolver( folders.local, folders.project );
    BOOST_CHECK( resolver.resolve( expected.string() ) == expected );
}

BOOST_AUTO_TEST_CASE( obsolete_absolute_path_finds_texture_beside_model )
{
    TextureFolders folders;
    const auto expected = folders.file( "Assets/Model/albedo.png" );
    const auto obsolete = folders.project / "DeletedExportFolder" / "albedo.png";
    workphone::detail::TexturePathResolver resolver( folders.local, folders.project );
    BOOST_CHECK( resolver.resolve( obsolete.string() ) == expected );
}

BOOST_AUTO_TEST_CASE( local_recursive_search_handles_case_and_windows_separators )
{
    TextureFolders folders;
    const auto expected = folders.file( "Assets/Model/Images/Textures/Albedo.PNG" );
    folders.file( "Shared/Albedo.PNG" );
    workphone::detail::TexturePathResolver resolver( folders.local, folders.project );
    BOOST_CHECK( resolver.resolve( "OldExport\\textures\\albedo.png" ) == expected );
}

BOOST_AUTO_TEST_CASE( project_recursive_search_finds_shared_texture )
{
    TextureFolders folders;
    const auto expected = folders.file( "Assets/Shared/Images/normal.png" );
    workphone::detail::TexturePathResolver resolver( folders.local, folders.project );
    BOOST_CHECK( resolver.resolve( "MissingExport/normal.png" ) == expected );
}

BOOST_AUTO_TEST_CASE( matching_trailing_directories_disambiguate_duplicate_names )
{
    TextureFolders folders;
    folders.file( "Assets/Shared/Brick/albedo.png" );
    const auto expected = folders.file( "Assets/Shared/Wood/albedo.png" );
    workphone::detail::TexturePathResolver resolver( folders.local, folders.project );
    BOOST_CHECK( resolver.resolve( "OldExport/Wood/albedo.png" ) == expected );
}

BOOST_AUTO_TEST_CASE( ambiguous_filenames_are_not_assigned_arbitrarily )
{
    TextureFolders folders;
    folders.file( "Assets/Model/Brick/albedo.png" );
    folders.file( "Assets/Model/Wood/albedo.png" );
    workphone::detail::TexturePathResolver resolver( folders.local, folders.project );
    BOOST_CHECK( resolver.resolve( "albedo.png" ).empty() );
}

BOOST_AUTO_TEST_CASE( missing_and_embedded_references_are_skipped )
{
    TextureFolders folders;
    workphone::detail::TexturePathResolver resolver( folders.local, folders.project );
    BOOST_CHECK( resolver.resolve( "missing.png" ).empty() );
    BOOST_CHECK( resolver.resolve( "*0" ).empty() );
    BOOST_CHECK( resolver.resolve( "" ).empty() );
    fs::create_directory( folders.local / "directory.png" );
    BOOST_CHECK( resolver.resolve( "directory.png" ).empty() );
}

BOOST_AUTO_TEST_CASE( next_import_discovers_previously_missing_textures )
{
    TextureFolders folders;
    workphone::detail::TexturePathResolver firstImport( folders.local, folders.project );
    BOOST_CHECK( firstImport.resolve( "new.png" ).empty() );
    const auto expected = folders.file( "Assets/Shared/new.png" );
    workphone::detail::TexturePathResolver nextImport( folders.local, folders.project );
    BOOST_CHECK( nextImport.resolve( "new.png" ) == expected );
}

BOOST_AUTO_TEST_SUITE_END()
