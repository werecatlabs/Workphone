#ifdef WP_ASSIMP_TEXTURE_TEST_STANDALONE
#    define BOOST_TEST_MODULE AssimpTexturePathTests
#    include <boost/test/included/unit_test.hpp>
#else
#    include <boost/test/unit_test.hpp>
#endif
#include <WPAssimp/TexturePathResolver.hpp>
#include <WPAssimp/EmbeddedTextureCache.hpp>
#include <WPAssimp/MaterialImportDefaults.hpp>
#include <limits>
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

BOOST_AUTO_TEST_CASE( legacy_obj_zero_opacity_recovery_is_scoped_and_optional )
{
    BOOST_CHECK( workphone::detail::recoverObjMaterialOpacity( 0.0f, true, true ) );
    BOOST_CHECK( !workphone::detail::recoverObjMaterialOpacity( 0.0f, true, false ) );
    BOOST_CHECK( !workphone::detail::recoverObjMaterialOpacity( 0.0f, false, true ) );
    BOOST_CHECK( !workphone::detail::recoverObjMaterialOpacity( 0.4f, true, true ) );
    BOOST_CHECK( !workphone::detail::recoverObjMaterialOpacity( 1.0f, true, true ) );
}

BOOST_AUTO_TEST_CASE( imported_material_factors_preserve_authored_values_and_sanitize_invalid_values )
{
    using workphone::detail::materialUnitFactor;
    BOOST_CHECK_EQUAL( materialUnitFactor( 0.0f, 1.0f ), 0.0f );
    BOOST_CHECK_EQUAL( materialUnitFactor( 0.4f, 1.0f ), 0.4f );
    BOOST_CHECK_EQUAL( materialUnitFactor( -2.0f, 0.5f ), 0.0f );
    BOOST_CHECK_EQUAL( materialUnitFactor( 2.0f, 0.5f ), 1.0f );
    BOOST_CHECK_EQUAL( materialUnitFactor( std::numeric_limits<float>::quiet_NaN(), 1.0f ), 1.0f );
    BOOST_CHECK_EQUAL( materialUnitFactor( std::numeric_limits<float>::infinity(), 0.5f ), 0.5f );
}

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

BOOST_AUTO_TEST_CASE( embedded_image_cache_preserves_bytes_and_reuses_unchanged_images )
{
    TextureFolders folders;
    const std::string image = "RIFFembedded image bytesWEBP";
    std::string error;
    const auto path = workphone::detail::cacheEmbeddedTexture(
        folders.project / "Cache", "Assets/cars/body.glb", 0, "webp", image.data(), image.size(), error );
    BOOST_REQUIRE_MESSAGE( !path.empty(), error );
    BOOST_CHECK( path.parent_path() == folders.project / "Cache/EmbeddedTextures" );
    BOOST_CHECK_EQUAL( path.extension().string(), ".webp" );
    std::ifstream input( path, std::ios::binary );
    BOOST_CHECK_EQUAL( std::string( std::istreambuf_iterator<char>( input ), {} ), image );
    input.close();
    const auto savedTime = fs::last_write_time( path ) - std::chrono::hours( 1 );
    fs::last_write_time( path, savedTime );
    const auto same = workphone::detail::cacheEmbeddedTexture(
        folders.project / "Cache", "Assets/cars/body.glb", 0, "webp", image.data(), image.size(), error );
    BOOST_CHECK( same == path );
    BOOST_CHECK( fs::last_write_time( path ) == savedTime );
}

BOOST_AUTO_TEST_CASE( embedded_image_reimport_gets_new_identity_when_content_changes )
{
    TextureFolders folders;
    std::string error;
    const std::string original = "original image";
    const std::string replacement = "changed image";
    const auto first = workphone::detail::cacheEmbeddedTexture(
        folders.project / "Cache", "Assets/cars/body.glb", 0, "webp",
        original.data(), original.size(), error );
    const auto next = workphone::detail::cacheEmbeddedTexture(
        folders.project / "Cache", "Assets/cars/body.glb", 0, "webp",
        replacement.data(), replacement.size(), error );
    BOOST_REQUIRE_MESSAGE( !first.empty() && !next.empty(), error );
    BOOST_CHECK( first != next );
    BOOST_CHECK( fs::exists( first ) );
    const auto otherModel = workphone::detail::cacheEmbeddedTexture(
        folders.project / "Cache", "Assets/cars/wheel.glb", 0, "webp",
        original.data(), original.size(), error );
    BOOST_CHECK( otherModel != first );
}

BOOST_AUTO_TEST_CASE( embedded_image_cache_repairs_incomplete_file_and_rejects_path_hints )
{
    TextureFolders folders;
    std::string error;
    const std::string image = "complete image";
    const auto path = workphone::detail::cacheEmbeddedTexture(
        folders.project / "Cache", "Assets/cars/body.glb", 0, "webp", image.data(), image.size(), error );
    BOOST_REQUIRE_MESSAGE( !path.empty(), error );
    std::ofstream( path, std::ios::binary | std::ios::trunc ) << "bad";
    const auto repaired = workphone::detail::cacheEmbeddedTexture(
        folders.project / "Cache", "Assets/cars/body.glb", 0, "webp", image.data(), image.size(), error );
    BOOST_REQUIRE( repaired == path );
    std::ifstream input( path, std::ios::binary );
    BOOST_CHECK_EQUAL( std::string( std::istreambuf_iterator<char>( input ), {} ), image );
    BOOST_CHECK( workphone::detail::cacheEmbeddedTexture(
        folders.project / "Cache", "Assets/cars/body.glb", 0, "../webp",
        image.data(), image.size(), error ).empty() );
    BOOST_CHECK( !error.empty() );
}

BOOST_AUTO_TEST_SUITE_END()
