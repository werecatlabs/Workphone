#pragma once
#include "Workphone/WorkphonePrerequisites.hpp"
#include "Workphone/Interface/Procedural/ProceduralTextureData.hpp"
#include "Workphone/Interface/Procedural/VehicleGeometryTypes.hpp"
#include <Workphone/System/SourceBudgetStdString.hpp>
#include <Workphone/System/SourceBudgetVector.hpp>
#include <array>
#include <string>
#include <vector>

namespace workphone
{
    namespace procedural
    {
        enum class VehicleAppearanceQuality : u8
        {
            Preview = 0,
            Standard,
            High,
            Cinematic
        };

        enum class VehicleMaterialSlot : u8
        {
            BodyPaint = 0,
            SecondaryPaint,
            CarbonGloss,
            CarbonMatte,
            AnodizedAccent,
            BareMetal,
            DarkMetal,
            Titanium,
            Exhaust,
            Rubber,
            Glass,
            CockpitTrim,
            BrakeDisc,
            BrakeDuct,
            Caliper,
            Headlight,
            TailLight,
            RainLight,
            Decal,
            Count
        };

        /// Linear working-space colour. Values are reflectance, not display RGB.
        struct WPCore_API VehicleLinearColor
        {
            real_Num r = 1.0f;
            real_Num g = 1.0f;
            real_Num b = 1.0f;
            real_Num a = 1.0f;
        };

        /// Renderer-neutral physical material metadata. Unsupported lobes may be ignored by a backend.
        struct WPCore_API VehicleMaterialDescriptor : SourceBudgetObject
        {
            SourceBudgetStdString name;
            VehicleLinearColor baseColor;
            /// Physically plausible values to use when the material's texture set is unavailable.
            VehicleLinearColor fallbackBaseColor;
            VehicleLinearColor emissiveColor{ 0.0f, 0.0f, 0.0f, 1.0f };
            real_Num roughness = 0.5f;
            real_Num fallbackRoughness = 0.5f;
            real_Num metallic = 0.0f;
            real_Num clearcoat = 0.0f;
            real_Num clearcoatRoughness = 0.1f;
            real_Num anisotropy = 0.0f;
            real_Num sheen = 0.0f;
            real_Num sheenRoughness = 0.5f;
            real_Num transmission = 0.0f;
            real_Num opacity = 1.0f;
            real_Num indexOfRefraction = 1.5f;
            real_Num iridescence = 0.0f;
            real_Num iridescenceIOR = 1.3f;
            real_Num iridescenceThicknessMinNm = 0.0f;
            real_Num iridescenceThicknessMaxNm = 0.0f;
            real_Num emissiveIntensity = 0.0f;
            real_Num normalScale = 1.0f;
            real_Num clearcoatNormalScale = 0.0f;
            bool transparent = false;
            bool doubleSided = false;
            bool depthWrite = true;

            VehicleMaterialDescriptor() = default;
            VehicleMaterialDescriptor( const VehicleMaterialDescriptor & ) = default;
            VehicleMaterialDescriptor( VehicleMaterialDescriptor && ) noexcept = default;
            VehicleMaterialDescriptor &operator=( VehicleMaterialDescriptor && ) noexcept = default;

            VehicleMaterialDescriptor &operator=( const VehicleMaterialDescriptor &other )
            {
                if( this != &other )
                {
                    VehicleMaterialDescriptor candidate( other );
                    swap( candidate );
                }
                return *this;
            }

            void swap( VehicleMaterialDescriptor &other ) noexcept
            {
                std::swap( *this, other );
            }
        };

        struct WPCore_API VehicleAppearanceQualityProfile
        {
            u32 liveryWidth = 512;
            u32 liveryHeight = 256;
            u32 bodySurfaceWidth = 512;
            u32 bodySurfaceHeight = 256;
            u32 microTextureSize = 256;
            u32 supersample = 1;
            u32 anisotropy = 8;
        };

        struct WPCore_API VehicleAppearanceConfig
        {
            u32 seed = 0x7f4a7c15u;
            VehicleAppearanceQuality quality = VehicleAppearanceQuality::High;
            VehicleLinearColor primary{ 0.36f, 0.018f, 0.030f, 1.0f };
            VehicleLinearColor secondary{ 0.012f, 0.016f, 0.025f, 1.0f };
            VehicleLinearColor accent{ 0.78f, 0.48f, 0.025f, 1.0f };
            u32 vehicleNumber = 7;
            real_Num wear = 0.12f;              ///< 0 pristine .. 1 heavily raced
            real_Num wetness = 0.0f;            ///< 0 dry .. 1 wet
            real_Num metallicBasecoat = 0.48f;  ///< aluminium-flake fraction
            u32 customMaxResolution = 0;        ///< 0 uses the quality profile
        };

        /// Upload-ready textures. Albedo and livery are sRGB RGBA8; every other map is linear RGBA8.
        struct WPCore_API VehicleTextureAssets : SourceBudgetObject
        {
            TextureBuffer bodyLivery;
            TextureBuffer bodyNormal;
            TextureBuffer bodyORM;        ///< R AO, G roughness, B metalness
            TextureBuffer bodyClearcoat;  ///< R amount, G roughness
            TextureBuffer paintCoatNormal;
            TextureBuffer carbonAlbedo;
            TextureBuffer carbonNormal;
            TextureBuffer carbonORM;
            TextureBuffer carbonAnisotropy;  ///< RG tangent direction, B strength
            TextureBuffer rubberAlbedo;
            TextureBuffer rubberNormal;
            TextureBuffer rubberORM;
            TextureBuffer brushedMetalNormal;
            TextureBuffer brushedMetalORM;
            TextureBuffer lightEmission;

            VehicleTextureAssets() = default;
            VehicleTextureAssets( const VehicleTextureAssets & ) = default;
            VehicleTextureAssets( VehicleTextureAssets && ) noexcept = default;
            VehicleTextureAssets &operator=( VehicleTextureAssets && ) noexcept = default;

            VehicleTextureAssets &operator=( const VehicleTextureAssets &other )
            {
                if( this != &other )
                {
                    VehicleTextureAssets candidate( other );
                    swap( candidate );
                }
                return *this;
            }

            void swap( VehicleTextureAssets &other ) noexcept
            {
                std::swap( *this, other );
            }
        };

        enum class VehicleAppearanceIssueSeverity : u8
        {
            Warning = 0,
            Error
        };

        struct WPCore_API VehicleAppearanceIssue : SourceBudgetObject
        {
            VehicleAppearanceIssueSeverity severity = VehicleAppearanceIssueSeverity::Error;
            SourceBudgetStdString code;
            SourceBudgetStdString message;

            VehicleAppearanceIssue() = default;

            VehicleAppearanceIssue( VehicleAppearanceIssueSeverity level,
                                    SourceBudgetStdString codeValue,
                                    SourceBudgetStdString messageValue ) noexcept :
                severity( level ),
                code( std::move( codeValue ) ),
                message( std::move( messageValue ) )
            {
            }

            VehicleAppearanceIssue( const VehicleAppearanceIssue & ) = default;
            VehicleAppearanceIssue( VehicleAppearanceIssue && ) noexcept = default;
            VehicleAppearanceIssue &operator=( VehicleAppearanceIssue && ) noexcept = default;

            VehicleAppearanceIssue &operator=( const VehicleAppearanceIssue &other )
            {
                if( this != &other )
                {
                    VehicleAppearanceIssue candidate( other );
                    swap( candidate );
                }
                return *this;
            }

            void swap( VehicleAppearanceIssue &other ) noexcept
            {
                std::swap( *this, other );
            }
        };

        struct WPCore_API VehicleAppearanceValidation : SourceBudgetObject
        {
            bool valid = true;
            SourceBudgetVector<VehicleAppearanceIssue> issues;

            VehicleAppearanceValidation() = default;
            VehicleAppearanceValidation( const VehicleAppearanceValidation & ) = default;
            VehicleAppearanceValidation( VehicleAppearanceValidation && ) noexcept = default;
            VehicleAppearanceValidation &operator=( VehicleAppearanceValidation && ) noexcept = default;

            VehicleAppearanceValidation &operator=( const VehicleAppearanceValidation &other )
            {
                if( this != &other )
                {
                    VehicleAppearanceValidation candidate( other );
                    swap( candidate );
                }
                return *this;
            }

            void swap( VehicleAppearanceValidation &other ) noexcept
            {
                std::swap( *this, other );
            }
        };

        struct WPCore_API VehicleAppearanceBundle : SourceBudgetObject
        {
            VehicleAppearanceQuality quality = VehicleAppearanceQuality::High;
            u32 seed = 0;
            std::array<VehicleMaterialDescriptor, static_cast<size_t>( VehicleMaterialSlot::Count )>
                materials;
            VehicleTextureAssets textures;
            u64 contentHash = 0;  ///< FNV-1a over descriptors and generated pixels.

            VehicleAppearanceBundle() = default;
            VehicleAppearanceBundle( const VehicleAppearanceBundle & ) = default;
            VehicleAppearanceBundle( VehicleAppearanceBundle && ) noexcept = default;
            VehicleAppearanceBundle &operator=( VehicleAppearanceBundle && ) noexcept = default;

            VehicleAppearanceBundle &operator=( const VehicleAppearanceBundle &other )
            {
                if( this != &other )
                {
                    VehicleAppearanceBundle candidate( other );
                    swap( candidate );
                }
                return *this;
            }

            void swap( VehicleAppearanceBundle &other ) noexcept
            {
                std::swap( *this, other );
            }

            const VehicleMaterialDescriptor &material( VehicleMaterialSlot slot ) const;
            VehicleMaterialDescriptor &material( VehicleMaterialSlot slot );
        };

    }  // namespace procedural
}  // namespace workphone
