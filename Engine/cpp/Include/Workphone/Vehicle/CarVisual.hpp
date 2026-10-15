#pragma once
#include <Workphone/WorkphonePrerequisites.hpp>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace workphone::vehiclevisual
{
    // C++17 port of the user-supplied Claude-for-speed CarVisual.js. All geometry
    // is car-local (+X right, +Y up, -Z forward); no Three.js/DOM dependency.
    struct Vec2
    {
        float x = 0, y = 0;
    };

    struct Vec3
    {
        float x = 0, y = 0, z = 0;
    };

    struct Colour
    {
        float r = 1, g = 1, b = 1, a = 1;
    };

    struct WPCore_API Bounds
    {
        Vec3 min{}, max{};
        bool valid = false;
        void include( Vec3 p );
        Vec3 size() const;
        Vec3 centre() const;
        bool contains( const Bounds &other ) const;
    };

    struct Vertex
    {
        Vec3 position{}, normal{ 0, 1, 0 }, tangent{ 1, 0, 0 };
        float tangentSign = 1;
        Vec2 uv0{}, uv1{};
    };

    struct Mesh
    {
        std::string name, sourceMaterial;
        std::vector<Vertex> vertices;
        std::vector<std::uint32_t> indices;
        Colour colour{};
        float roughness = .5f, metalness = 0, opacity = 1;
        bool transparent = false;
    };

    struct Models
    {
        std::vector<Mesh> chassis, wheel;
    };

    struct Spec
    {
        float length = 4.5f, wheelbase = 2.7f, frontTrack = 1.6f, rearTrack = 1.6f;
        std::array<float, 4> wheelRadius{ .35f, .35f, .35f, .35f };
        float axleOffsetZ = 0, yawFix = 0;
        bool sourceForwardPlusZ = true, player = true, police = false, rivalShadows = true;
        std::uint32_t seed = 1;
    };

    struct Frame
    {
        Vec3 position{};
        float yaw = 0, pitch = 0, roll = 0, rideY = 0, speed = 0, brake = 0, backfire = 0, nitrous = 0;
        std::array<float, 4> steering{}, rotation{}, suspension{};
        bool headlights = true;
    };
    enum class Kind
    {
        Group,
        Body,
        Wheel,
        WheelBlur,
        Headlight,
        Beam,
        Pool,
        Tail,
        TailGlow,
        Underglow,
        Flame,
        PoliceLamp,
        PoliceHalo,
        ContactShadow
    };
    enum class Texture
    {
        None,
        Glow,
        Cone,
        Flare,
        Shadow
    };

    struct Node
    {
        std::string name;
        int parent = -1, corner = -1;
        Kind kind = Kind::Group;
        std::shared_ptr<const Mesh> mesh;
        Vec3 position{}, euler{}, scale{ 1, 1, 1 };
        Colour colour{};
        float opacity = 1, roughness = .5f, metalness = 0, clearcoat = 0, clearcoatRoughness = .06f;
        bool visible = true, castShadow = false, transparent = false, additive = false,
             depthWrite = true;
        bool paint = false, glass = false;
        Texture texture = Texture::None;
    };

    struct Image
    {
        unsigned width = 0, height = 0;
        std::vector<std::uint8_t> rgba;
    };

    struct Spot
    {
        Vec3 position{}, target{};
        Colour colour{};
        float intensity = 0, range = 0, angle = 0, penumbra = 0;
        bool shadows = false;
    };

    class WPCore_API CarVisual
    {
    public:
        // Pure inputs: never mutates a shared imported chassis or wheel prototype.
        void configure( const Spec &spec, const Models &models, Colour paint );
        void setPaint( Colour paint );
        void setLivery( std::string texturePath );
        void sync( const Frame &frame, double seconds, const Vec3 *cameraPosition = nullptr );
        void setVisible( bool visible );

        const std::vector<Node> &nodes() const
        {
            return m_nodes;
        }

        const std::array<Spot, 2> &spots() const
        {
            return m_spots;
        }

        const std::string &livery() const
        {
            return m_livery;
        }

        const Spec &spec() const
        {
            return m_spec;
        }

        static Bounds robustBox( const std::vector<Mesh> &meshes );
        static Bounds percentileBox( const std::vector<Mesh> &meshes, float low = .02f,
                                     float high = .98f );
        static std::shared_ptr<const Image> texture( Texture type );

    private:
        Spec m_spec;
        Colour m_paint;
        std::string m_livery;
        std::vector<Node> m_nodes;
        std::array<Spot, 2> m_spots{};
        std::array<int, 4> m_pivots{}, m_spins{};
        bool m_visible = true;
        int add( Node node );
    };
}  // namespace workphone::vehiclevisual
