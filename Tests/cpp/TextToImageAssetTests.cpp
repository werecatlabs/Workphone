#include <ui/TextToImageAssets.hpp>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    namespace fs = std::filesystem;
    namespace assets = workphone::editor::image_assets;

    void require( bool value, const char *message )
    {
        if( !value ) throw std::runtime_error( message );
    }

    struct TemporaryDirectory
    {
        fs::path path;
        TemporaryDirectory()
        {
            const auto nonce = std::to_string( std::chrono::steady_clock::now().time_since_epoch().count() );
            const auto random = std::to_string( std::random_device{}() );
            path = fs::temp_directory_path() / ( "workphone_text_to_image_" + nonce + "_" + random );
            require( fs::create_directory( path ), "Could not create unique test directory" );
        }
        ~TemporaryDirectory()
        {
            std::error_code ignored;
            if( path.parent_path() == fs::temp_directory_path() &&
                path.filename().string().find( "workphone_text_to_image_" ) == 0 )
                fs::remove_all( path, ignored );
        }
    };

    std::vector<unsigned char> read( const fs::path &path )
    {
        std::ifstream input( path, std::ios::binary );
        require( input.good(), "Generated PNG could not be reopened" );
        return { std::istreambuf_iterator<char>( input ), std::istreambuf_iterator<char>() };
    }

    const unsigned char png[] = {
        0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a,
        0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
        0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
        0x08, 0x04, 0x00, 0x00, 0x00, 0xb5, 0x1c, 0x0c, 0x02,
        0x00, 0x00, 0x00, 0x0b, 0x49, 0x44, 0x41, 0x54,
        0x78, 0xda, 0x63, 0xfc, 0xff, 0x1f, 0x00, 0x03, 0x03,
        0x02, 0x00, 0xef, 0x97, 0xda, 0x1d,
        0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44,
        0xae, 0x42, 0x60, 0x82
    };

    void contracts()
    {
        TemporaryDirectory temporary;
        const auto project = temporary.path / "Project with spaces";
        fs::create_directories( project );
        fs::path output;
        std::string error;
        auto saved = assets::savePng( project, "test-image", png, sizeof( png ), output, error );
        require( saved, error.c_str() );
        require( error.empty(), "Successful save retained a previous error" );
        require( output.parent_path() == fs::canonical( project / "Assets/Generated" ) &&
                 output.extension() == ".png", "Image was not saved in project Assets/Generated" );
        const auto expected = std::vector<unsigned char>( std::begin( png ), std::end( png ) );
        require( read( output ) == expected, "Saved PNG bytes changed" );
        const auto first = output;
        saved = assets::savePng( project, "test-image", png, sizeof( png ), output, error );
        require( saved, error.c_str() );
        require( output != first && read( first ) == expected && read( output ) == expected,
                 "Repeated generation overwrote the previous asset" );

        for( const auto name : { "../../outside", "C:\\outside\\image", "a/b\\c:d*e?f\"g<h>i|j",
                                "", "...", " . ", "CON", "con.png", "AUX", "NUL", "COM1", "LPT9" } )
        {
            const auto safe = assets::sanitizeBasename( name );
            require( !safe.empty() && safe != "." && safe != "..",
                     "Sanitizer returned an empty or traversal name" );
            require( safe.find_first_of( "/\\:*?\"<>|" ) == std::string::npos &&
                     safe.back() != '.' && safe.back() != ' ', "Filename contains unsafe characters" );
            saved = assets::savePng( project, name, png, sizeof( png ), output, error );
            require( saved, error.c_str() );
            require( output.parent_path() == fs::canonical( project / "Assets/Generated" ),
                     "Filename escaped the generated assets directory" );
        }
        const auto longName = assets::sanitizeBasename( std::string( 1000, 'a' ) );
        require( longName.size() <= 200, "Generated filename was not bounded" );
        const auto filesBefore = std::distance( fs::directory_iterator( project / "Assets/Generated" ),
                                               fs::directory_iterator() );
        require( !assets::savePng( project, "invalid", nullptr, 0, output, error ) && !error.empty(),
                 "Empty image was saved" );
        const unsigned char invalid[] = { 'n', 'o', 't', 'a', 'p', 'n', 'g', '!' };
        require( !assets::savePng( project, "invalid", invalid, sizeof( invalid ), output, error ),
                 "Non-PNG payload was saved" );
        const auto filesAfter = std::distance( fs::directory_iterator( project / "Assets/Generated" ),
                                              fs::directory_iterator() );
        require( filesBefore == filesAfter, "Invalid image left an asset file" );

        const auto blocked = temporary.path / "BlockedProject";
        fs::create_directory( blocked );
        std::ofstream( blocked / "Assets" ) << "existing file";
        require( !assets::savePng( blocked, "image", png, sizeof( png ), output, error ) &&
                 !error.empty(), "Directory creation error was not reported" );

        // Creating symlinks may require a Windows developer-mode setting. Exercise
        // confinement when available; absence of that privilege does not hide other tests.
        const auto linked = temporary.path / "LinkedProject";
        const auto outside = temporary.path / "Outside";
        fs::create_directories( linked / "Assets" );
        fs::create_directory( outside );
        std::error_code linkError;
        fs::create_directory_symlink( outside, linked / "Assets/Generated", linkError );
        if( !linkError )
        {
            require( !assets::savePng( linked, "escape", png, sizeof( png ), output, error ),
                     "Generated directory symlink escaped the project" );
            require( fs::is_empty( outside ), "Symlink target was modified" );
        }
        else
        {
            std::cout << "INFO: symlink fixture unavailable: " << linkError.message() << '\n';
        }
    }
}

int main()
{
    try
    {
        contracts();
        std::cout << "PASS: generated PNG names, project confinement and no-overwrite saves\n";
        return 0;
    }
    catch( const std::exception &error )
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
