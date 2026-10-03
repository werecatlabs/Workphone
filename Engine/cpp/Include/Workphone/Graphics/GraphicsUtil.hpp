#ifndef GraphicsUtil_h__
#define GraphicsUtil_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @class GraphicsUtil
         * @brief Utility class providing static methods for graphics-related string conversions and
         * operations.
         *
         * The GraphicsUtil class serves as a central utility for converting various graphics enumeration
         * types to their corresponding string representations. This is particularly useful for
         * serialization, debugging, logging, and user interface display purposes.
         *
         * All methods in this class are static, making it a pure utility class that doesn't require
         * instantiation. The class handles conversions for terrain textures, PBS (Physically Based
         * Shading) textures, skybox textures, and material types.
         *
         * @note This class is designed to be stateless and thread-safe.
         * @since Engine Version 1.0
         * @author Engine Team
         */
        class WPCore_API GraphicsUtil
        {
        public:
            /**
             * @brief Converts a terrain texture type enumeration to its string representation.
             *
             * This method maps TerrainTextureTypes enumeration values to their corresponding
             * string names, which can be used for configuration files, debugging output,
             * or user interface display.
             *
             * @param terrainTextureType The terrain texture type to convert
             * @return String representation of the terrain texture type (e.g., "Base", "Splat",
             * "Diffuse1")
             *
             * @see TerrainTextureTypes
             *
             * @par Example:
             * @code
             * String textureTypeName =
             * GraphicsUtil::getTerrainTextureType(TerrainTextureTypes::Diffuse1);
             * // textureTypeName will be "Diffuse1"
             * @endcode
             */
            static String getTerrainTextureType( TerrainTextureTypes terrainTextureType );

            /**
             * @brief Converts a PBS (Physically Based Shading) texture type enumeration to its string
             * representation.
             *
             * This method provides string representations for PBS texture types used in
             * physically based rendering pipelines. These include various material properties
             * like diffuse, normal, metallic, roughness, and detail textures.
             *
             * @param pbsTextureType The PBS texture type to convert
             * @return String representation of the PBS texture type (e.g., "PBSM_DIFFUSE",
             * "PBSM_NORMAL", "PBSM_METALLIC")
             *
             * @see PbsTextureTypes
             *
             * @par Example:
             * @code
             * String pbsTypeName = GraphicsUtil::getPbsTextureType(PbsTextureTypes::PBSM_METALLIC);
             * // pbsTypeName will be "PBSM_METALLIC"
             * @endcode
             */
            static String getPbsTextureType( PbsTextureTypes pbsTextureType );

            /**
             * @brief Converts a skybox texture type enumeration to its string representation.
             *
             * This method maps skybox texture types to their string equivalents, typically
             * representing the six faces of a skybox cube (Front, Back, Left, Right, Up, Down).
             *
             * @param skyboxTextureType The skybox texture type to convert
             * @return String representation of the skybox texture type (e.g., "Front", "Back", "Left")
             *
             * @see SkyboxTextureTypes
             *
             * @par Example:
             * @code
             * String skyboxFace = GraphicsUtil::getSkyboxTextureType(SkyboxTextureTypes::Front);
             * // skyboxFace will be "Front"
             * @endcode
             */
            static String getSkyboxTextureType( SkyboxTextureTypes skyboxTextureType );

            /**
             * @brief Converts a skybox cube texture type enumeration to its string representation.
             *
             * This method handles the conversion of skybox cube texture types, which are used
             * for cube texture skyboxes as opposed to traditional six-face skyboxes.
             *
             * @param skyboxTextureType The skybox cube texture type to convert
             * @return String representation of the skybox cube texture type (e.g., "Cube")
             *
             * @see SkyboxCubeTextureTypes
             *
             * @par Example:
             * @code
             * String cubeType = GraphicsUtil::getSkyboxCubeTextureType(SkyboxCubeTextureTypes::Cube);
             * // cubeType will be "Cube"
             * @endcode
             */
            static String getSkyboxCubeTextureType( SkyboxCubeTextureTypes skyboxTextureType );

            /**
             * @brief Converts a material type enumeration to its string representation.
             *
             * This method provides string representations for different material types
             * supported by the graphics engine, including standard materials, terrain materials,
             * skybox materials, and UI materials.
             *
             * @param materialType The material type to convert
             * @return String representation of the material type (e.g., "Standard", "TerrainDiffuse",
             * "Skybox")
             *
             * @see MaterialType
             *
             * @par Example:
             * @code
             * String matType = GraphicsUtil::getMaterialType(MaterialType::TerrainStandard);
             * // matType will be "TerrainStandard"
             * @endcode
             */
            static String getMaterialType( MaterialType materialType );

            /**
             * @brief Retrieves an array of all available material type strings.
             *
             * This method returns a complete list of all material type names supported
             * by the graphics system. This is useful for populating UI dropdowns,
             * configuration validation, or iteration over all available material types.
             *
             * @return Array<String> containing all material type names
             *
             * @par Example:
             * @code
             * Array<String> allMaterialTypes = GraphicsUtil::getMaterialTypes();
             * for(const auto& matType : allMaterialTypes) {
             *     // Process each material type name
             *     WP_LOG_MESSAGE("Available material type: " + matType);
             * }
             * @endcode
             *
             * @note The returned array contains string representations of all values in the MaterialType
             * enumeration.
             */
            static Array<String> getMaterialTypes();

            /**
             * @brief Retrieves a concatenated string of all material types.
             *
             * This method returns all material type names as a single string,
             * typically separated by delimiters. This is useful for logging,
             * debugging output, or creating summary information.
             *
             * @return String containing all material type names, typically comma or space separated
             *
             * @par Example:
             * @code
             * String allTypes = GraphicsUtil::getMaterialTypesString();
             * // allTypes might be "Standard,StandardSpecular,TerrainStandard,Skybox,..."
             * WP_LOG_MESSAGE("Supported material types: " + allTypes);
             * @endcode
             *
             * @note The exact formatting and separator used in the returned string is
             * implementation-dependent.
             */
            static String getMaterialTypesString();
        };

    }  // end namespace render
}  // namespace workphone

#endif  // GraphicsUtil_h__
