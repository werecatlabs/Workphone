#pragma once

#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace workphone
{
    struct BakeVertex
    {
        Vector3F position;
        Vector3F normal;
        Vector2F uv1;
    };

    struct BakeTriangle
    {
        BakeVertex v0;
        BakeVertex v1;
        BakeVertex v2;
    };

    enum class BakeLightType
    {
        Directional,
        Point
    };

    struct BakeLight
    {
        BakeLightType type = BakeLightType::Directional;

        // Directional: direction points from light toward scene.
        Vector3F direction = Vector3F( 0.0f, -1.0f, 0.0f );

        // Point light.
        Vector3F position = Vector3F::ZERO;
        float range = 10.0f;

        // RGBA channel: 0, 1, 2, 3.
        int channel = 0;
    };

    struct Ray
    {
        Vector3F origin;
        Vector3F dir;
        float maxT = std::numeric_limits<float>::max();
    };

    struct ShadowMaskBakeSettings
    {
        int width = 1024;
        int height = 1024;

        // 1 = single sample. 2 = 2x2. 4 = 4x4.
        int samplesPerAxis = 2;

        // Offset along interpolated normal to avoid self-intersection.
        float normalBias = 0.02f;

        // Max directional light ray length.
        float directionalRayDistance = 100000.0f;

        // Reject lighting when normal faces away from light.
        bool rejectBackfaces = true;

        // Expand valid texels into empty neighbours.
        int dilationIterations = 8;
    };

    class ShadowMaskBaker
    {
    public:
        static bool bake( const std::string &outputTga, const ShadowMaskBakeSettings &settings,
                          const std::vector<BakeTriangle> &meshTriangles,
                          const std::vector<BakeTriangle> &sceneTriangles,
                          const std::vector<BakeLight> &lights );

    private:
        struct Aabb
        {
            Vector3F min;
            Vector3F max;
        };

        struct BvhNode
        {
            Aabb bounds;
            int left = -1;
            int right = -1;
            int firstTri = 0;
            int triCount = 0;

            bool isLeaf() const
            {
                return triCount > 0;
            }
        };

        class TriangleBvh
        {
        public:
            explicit TriangleBvh( const std::vector<BakeTriangle> &triangles );

            bool occluded( const Ray &ray ) const;

        private:
            int buildRecursive( int first, int count );
            bool occludedRecursive( int nodeIndex, const Ray &ray ) const;

            static Aabb makeTriangleBounds( const BakeTriangle &tri );
            static Aabb merge( const Aabb &a, const Aabb &b );
            static Vector3F centroid( const BakeTriangle &tri );

            std::vector<BakeTriangle> m_triangles;
            std::vector<int> m_indices;
            std::vector<BvhNode> m_nodes;
        };

        static bool intersectRayTriangle( const Ray &ray, const Vector3F &a, const Vector3F &b,
                                          const Vector3F &c, float &outT );

        static bool intersectRayAabb( const Ray &ray, const Aabb &aabb );

        static bool pointInUvTriangle( const Vector2F &p, const Vector2F &a, const Vector2F &b,
                                       const Vector2F &c, float &w0, float &w1, float &w2 );

        static Vector3F baryPosition( const BakeTriangle &tri, float w0, float w1, float w2 );
        static Vector3F baryNormal( const BakeTriangle &tri, float w0, float w1, float w2 );

        static float bakeLightVisibility( const Vector3F &position, const Vector3F &normal,
                                          const BakeLight &light, const TriangleBvh &bvh,
                                          const ShadowMaskBakeSettings &settings );

        static void dilate( int width, int height, std::vector<std::array<uint8_t, 4>> &pixels,
                            std::vector<uint8_t> &coverage, int iterations );

        static bool writeRGBA8Tga( const std::string &path, int width, int height,
                                   const std::vector<std::array<uint8_t, 4>> &pixels );
    };
}  // namespace workphone
