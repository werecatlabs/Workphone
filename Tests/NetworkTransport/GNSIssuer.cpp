#include "GNSIssuerWire.pb.h"

#include <openssl/evp.h>
#include <openssl/pem.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <limits>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#include <fcntl.h>
#include <sys/stat.h>
#ifdef _WIN32
#    include <io.h>
#    include <share.h>
#else
#    include <unistd.h>
#endif

namespace
{
    namespace wire = workphone::network::gns_issuer;
    using Path = std::filesystem::path;
    using Key = std::unique_ptr<EVP_PKEY, decltype( &EVP_PKEY_free )>;
    using PublicKey = std::array<unsigned char, 32>;
    using Arguments = std::map<std::string, std::string>;

    constexpr unsigned DefaultLifetime = 3600;
    constexpr unsigned MaxLifetime = 24 * 60 * 60;
    constexpr std::size_t MaxRequestBytes = 8192;

    void require( bool condition, const char *error )
    {
        if( !condition )
        {
            throw std::runtime_error( error );
        }
    }

    template <typename Write>
    void writeNew( const Path &path, Write write )
    {
        // Exclusive creation prevents an operator from overwriting a key or certificate.
#ifdef _WIN32
        int fd = -1;
        require( _wsopen_s( &fd, path.c_str(), _O_WRONLY | _O_CREAT | _O_EXCL | _O_BINARY,
                            _SH_DENYRW, _S_IREAD | _S_IWRITE ) == 0,
                 "Cannot create output. Use a new file in an existing directory." );
        FILE *output = _fdopen( fd, "wb" );
        if( !output )
        {
            _close( fd );
        }
#else
        const int fd = open( path.c_str(), O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR );
        require( fd >= 0, "Cannot create output. Use a new file in an existing directory." );
        FILE *output = fdopen( fd, "wb" );
        if( !output )
        {
            close( fd );
        }
#endif
        if( output )
        {
            const bool written = write( output );
            const bool closed = std::fclose( output ) == 0;
            if( written && closed )
            {
                return;
            }
        }
        std::error_code ignored;
        std::filesystem::remove( path, ignored );
        throw std::runtime_error( "Cannot write output file" );
    }

    void writeBytes( const Path &path, std::string_view bytes )
    {
        writeNew( path, [&]( FILE *file ) {
            return std::fwrite( bytes.data(), 1, bytes.size(), file ) == bytes.size();
        } );
    }

    Key readPrivateKey( const Path &path )
    {
        FILE *file = nullptr;
#ifdef _WIN32
        _wfopen_s( &file, path.c_str(), L"rb" );
#else
        file = std::fopen( path.c_str(), "rb" );
#endif
        require( file != nullptr, "Cannot open CA key file" );
        // A tool used in scripted issuance must never stop to prompt for a password.
        const auto noPassword = []( char *, int, int, void * ) { return -1; };
        Key key( PEM_read_PrivateKey( file, nullptr, noPassword, nullptr ), EVP_PKEY_free );
        std::fclose( file );
        require( key && EVP_PKEY_is_a( key.get(), "ED25519" ) == 1,
                 "CA key must be an unencrypted Ed25519 PEM private key" );
        return key;
    }

    PublicKey publicKey( EVP_PKEY *key )
    {
        PublicKey bytes{};
        std::size_t size = bytes.size();
        require( EVP_PKEY_get_raw_public_key( key, bytes.data(), &size ) == 1 && size == bytes.size(),
                 "Cannot extract Ed25519 public key" );
        return bytes;
    }

    std::uint64_t keyId( const PublicKey &key )
    {
        std::array<unsigned char, 32> hash{};
        unsigned int size = 0;
        require( EVP_Digest( key.data(), key.size(), hash.data(), &size, EVP_sha256(), nullptr ) == 1 &&
                     size == hash.size(),
                 "Cannot hash public key" );
        // GNS interprets the first eight bytes of SHA-256 as a little-endian integer.
        std::uint64_t value = 0;
        for( unsigned i = 0; i < sizeof( value ); ++i )
        {
            value |= std::uint64_t{ hash[i] } << ( i * 8 );
        }
        require( value != 0, "CA key has a reserved GNS key ID" );
        return value;
    }

    void generateKey( const Path &out )
    {
        Key key( EVP_PKEY_Q_keygen( nullptr, nullptr, "ED25519" ), EVP_PKEY_free );
        require( key != nullptr, "Ed25519 key generation failed" );
        writeNew( out, [&]( FILE *file ) {
            return PEM_write_PrivateKey( file, key.get(), nullptr, nullptr, 0, nullptr, nullptr ) == 1;
        } );
    }

    void exportRoot( const Path &keyFile, const Path &out )
    {
        const auto key = readPrivateKey( keyFile );
        const auto raw = publicKey( key.get() );
        // OpenSSH's blob is two length-prefixed strings, in network byte order.
        std::array<unsigned char, 51> blob{ 0, 0, 0, 11, 's', 's', 'h', '-', 'e', 'd', '2', '5', '5',
                                            '1', '9', 0, 0, 0, 32 };
        std::copy( raw.begin(), raw.end(), blob.begin() + 19 );
        std::array<unsigned char, 69> base64{};
        const int size = EVP_EncodeBlock( base64.data(), blob.data(), static_cast<int>( blob.size() ) );
        require( size == 68, "Cannot encode OpenSSH public key" );
        const std::string encoded( reinterpret_cast<const char *>( base64.data() ), size );
        writeBytes( out, "ssh-ed25519 " + encoded + " ID" + std::to_string( keyId( raw ) ) + "\n" );
    }

    std::string requestedPublicKey( const Path &file )
    {
        std::ifstream input( file, std::ios::binary );
        require( input.is_open(), "Cannot open certificate request" );
        std::array<char, MaxRequestBytes + 1> bytes{};
        input.read( bytes.data(), bytes.size() );
        const auto size = input.gcount();
        require( !input.bad() && size > 0 && size <= static_cast<std::streamsize>( MaxRequestBytes ),
                 "Certificate request is empty, too large, or unreadable" );
        wire::Request request;
        require( request.ParseFromArray( bytes.data(), static_cast<int>( size ) ) && request.has_leaf() &&
                     request.leaf().key_type() == wire::Leaf::ED25519 &&
                     request.leaf().key_data().size() == PublicKey{}.size(),
                 "Expected a GNS request with an Ed25519 public key" );
        return request.leaf().key_data();
    }

    bool validIdentity( std::string_view identity )
    {
        constexpr std::string_view prefix = "str:";
        if( identity.size() <= prefix.size() || identity.size() > prefix.size() + 31 ||
            identity.substr( 0, prefix.size() ) != prefix )
        {
            return false;
        }
        return std::all_of( identity.begin() + prefix.size(), identity.end(), []( char c ) {
            return ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c >= '0' && c <= '9' ) ||
                   c == '-' || c == '_' || c == '.';
        } );
    }

    unsigned lifetimeSeconds( std::string_view value )
    {
        unsigned lifetime = 0;
        const auto end = value.data() + value.size();
        const auto parsed = std::from_chars( value.data(), end, lifetime );
        require( parsed.ec == std::errc{} && parsed.ptr == end && lifetime >= 1 && lifetime <= MaxLifetime,
                 "Certificate lifetime must be 1 through 86400 seconds" );
        return lifetime;
    }

    void issue( const Path &requestFile, const std::string &identity, const Path &keyFile,
                unsigned lifetime, const Path &out )
    {
        require( validIdentity( identity ),
                 "Identity must be str: followed by 1 to 31 ASCII letters, digits, periods, hyphens or underscores" );
        const std::time_t now = std::time( nullptr );
        require( now > 0 && std::uint64_t( now ) <= std::numeric_limits<std::uint32_t>::max() - lifetime,
                 "System clock is outside the certificate time range" );

        // A request has no authority. Rebuild the leaf from just its public key and the
        // operator-selected identity and lifetime. Ignore all requested claims and extensions.
        wire::Leaf leaf;
        leaf.set_key_type( wire::Leaf::ED25519 );
        leaf.set_key_data( requestedPublicKey( requestFile ) );
        leaf.set_identity( identity );
        leaf.set_time_created( static_cast<std::uint32_t>( now ) );
        leaf.set_time_expiry( static_cast<std::uint32_t>( now ) + lifetime );
        // Standalone open-source GNS uses AppID 0. Its peer verifier requires a
        // nonempty AppID list for a signed certificate with a named identity.
        leaf.add_app_ids( 0 );

        wire::SignedLeaf signedLeaf;
        require( leaf.SerializeToString( signedLeaf.mutable_leaf() ), "Cannot encode certificate" );
        const auto key = readPrivateKey( keyFile );
        signedLeaf.set_ca_key_id( keyId( publicKey( key.get() ) ) );
        std::unique_ptr<EVP_MD_CTX, decltype( &EVP_MD_CTX_free )> signing( EVP_MD_CTX_new(),
                                                                        EVP_MD_CTX_free );
        std::array<unsigned char, 64> signature{};
        std::size_t size = signature.size();
        const auto &body = signedLeaf.leaf();
        require( signing && EVP_DigestSignInit( signing.get(), nullptr, nullptr, nullptr, key.get() ) == 1 &&
                     EVP_DigestSign( signing.get(), signature.data(), &size,
                                     reinterpret_cast<const unsigned char *>( body.data() ), body.size() ) == 1 &&
                     size == signature.size(),
                 "Ed25519 signing failed" );
        signedLeaf.set_ca_signature( signature.data(), signature.size() );
        std::string bytes;
        require( signedLeaf.SerializeToString( &bytes ), "Cannot encode signed certificate" );
        writeBytes( out, bytes );
    }

    Arguments parse( int argc, char **argv, std::initializer_list<std::string_view> allowed )
    {
        Arguments options;
        for( int i = 2; i < argc; i += 2 )
        {
            require( i + 1 < argc && argv[i + 1][0] != '\0', "Every option needs a value" );
            const std::string_view name( argv[i] );
            require( std::find( allowed.begin(), allowed.end(), name ) != allowed.end(), "Unknown option" );
            require( options.emplace( name, argv[i + 1] ).second, "Duplicate option" );
        }
        return options;
    }

    const std::string &required( const Arguments &options, const char *name )
    {
        const auto value = options.find( name );
        require( value != options.end(), "Required option missing. See --help." );
        return value->second;
    }

    void help()
    {
        std::puts( "WPNetworkGNSIssuer generate-key --out <new PEM file>\n"
                   "WPNetworkGNSIssuer root --key <CA PEM> --out <new public-root file>\n"
                   "WPNetworkGNSIssuer issue --request <GNS request> --identity <str:name> --key <CA PEM>\n"
                   "                        --out <new certificate file> [--lifetime-seconds <1-86400>]\n"
                   "Use generate-key in a directory restricted to the issuer. Approve identity ownership\n"
                   "before calling issue. The request provides a public key, with no identity proof." );
    }
}  // namespace

int main( int argc, char **argv )
{
    try
    {
        const std::string_view command = argc >= 2 ? argv[1] : "";
        if( command == "generate-key" )
        {
            const auto options = parse( argc, argv, { "--out" } );
            generateKey( required( options, "--out" ) );
        }
        else if( command == "root" )
        {
            const auto options = parse( argc, argv, { "--key", "--out" } );
            exportRoot( required( options, "--key" ), required( options, "--out" ) );
        }
        else if( command == "issue" )
        {
            const auto options = parse( argc, argv, { "--request", "--identity", "--key", "--out", "--lifetime-seconds" } );
            const auto lifetime = options.find( "--lifetime-seconds" );
            issue( required( options, "--request" ), required( options, "--identity" ),
                   required( options, "--key" ),
                   lifetime == options.end() ? DefaultLifetime : lifetimeSeconds( lifetime->second ),
                   required( options, "--out" ) );
        }
        else if( command == "--help" && argc == 2 )
        {
            help();
        }
        else
        {
            throw std::runtime_error( "Expected generate-key, root or issue. See --help." );
        }
        return 0;
    }
    catch( const std::exception &error )
    {
        std::fprintf( stderr, "WPNetworkGNSIssuer: %s\n", error.what() );
        return 1;
    }
}
