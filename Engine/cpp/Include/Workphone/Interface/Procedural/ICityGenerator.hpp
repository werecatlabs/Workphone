#ifndef ICityGenerator_h__
#define ICityGenerator_h__

#include <Workphone/Interface/Procedural/IProceduralGenerator.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace procedural
    {

        /**
         * @class ICityGenerator
         * @brief Interface for procedural city layout generation.
         *
         * This interface defines the contract for generating city layouts using procedural generation
         * techniques. It extends the IProceduralGenerator interface and provides methods for
         * configuring, controlling, and retrieving results from the city generation process.
         * Implementations of this interface allow for modular and extensible city generation, supporting
         * various generator modules such as terrain, block, mesh, and road generators, as well as
         * integration with a procedural world.
         */
        class WPCore_API ICityGenerator : public IProceduralGenerator
        {
        public:
            /**
             * @brief Virtual destructor for safe cleanup of derived classes.
             */
            ~ICityGenerator() override;

            /**
             * @brief Starts the city generation process.
             *
             * This pure virtual method triggers the procedural generation of the city layout using the
             * configured generator modules and settings. The process may be asynchronous or
             * time-consuming, depending on the implementation. Use isFinished() to check for completion.
             */
            void generate() override = 0;

            /**
             * @brief Checks if the city generation process has completed.
             *
             * @return True if the generation process is finished and all cities have been generated;
             * false otherwise.
             */
            bool isFinished() const override = 0;

            /**
             * @brief Gets the terrain generator module used for terrain mesh generation.
             *
             * @return Smart pointer to the ITerrainGenerator module, or nullptr if not set.
             */
            virtual SmartPtr<ITerrainGenerator> getTerrainGenerator() const = 0;

            /**
             * @brief Sets the terrain generator module.
             *
             * The terrain generator is responsible for creating the base terrain mesh, which other
             * modules can build upon.
             *
             * @param terrainGenerator Smart pointer to the ITerrainGenerator module to use.
             */
            virtual void setTerrainGenerator( SmartPtr<ITerrainGenerator> terrainGenerator ) = 0;

            /**
             * @brief Gets the block generator module used for city block generation.
             *
             * @return Smart pointer to the IBlockGenerator module, or nullptr if not set.
             */
            virtual SmartPtr<IBlockGenerator> getBlockGenerator() const = 0;

            /**
             * @brief Sets the block generator module.
             *
             * The block generator is responsible for creating the structure and layout of city blocks.
             *
             * @param blockGenerator Smart pointer to the IBlockGenerator module to use.
             */
            virtual void setBlockGenerator( SmartPtr<IBlockGenerator> blockGenerator ) = 0;

            /**
             * @brief Gets the procedural world module used for building the city world.
             *
             * @return Smart pointer to the IProceduralWorld module, or nullptr if not set.
             */
            virtual SmartPtr<IProceduralWorld> getProceduralWorld() const = 0;

            /**
             * @brief Sets the procedural world module.
             *
             * The procedural world module manages the overall world context in which the city is
             * generated.
             *
             * @param proceduralWorld Smart pointer to the IProceduralWorld module to use.
             */
            virtual void setProceduralWorld( SmartPtr<IProceduralWorld> proceduralWorld ) = 0;

            /**
             * @brief Gets the mesh generator module used for mesh generation.
             *
             * @return Smart pointer to the IMeshGenerator module, or nullptr if not set.
             */
            virtual SmartPtr<IMeshGenerator> getMeshGenerator() const = 0;

            /**
             * @brief Sets the mesh generator module.
             *
             * The mesh generator is responsible for creating meshes for city elements.
             *
             * @param meshGenerator Smart pointer to the IMeshGenerator module to use.
             */
            virtual void setMeshGenerator( SmartPtr<IMeshGenerator> meshGenerator ) = 0;

            /**
             * @brief Gets the road generator module used for road network generation.
             *
             * @return Smart pointer to the IRoadGenerator module, or nullptr if not set.
             */
            virtual SmartPtr<IRoadGenerator> getRoadGenerator() const = 0;

            /**
             * @brief Sets the road generator module.
             *
             * The road generator is responsible for creating the road network within the city.
             *
             * @param roadGenerator Smart pointer to the IRoadGenerator module to use.
             */
            virtual void setRoadGenerator( SmartPtr<IRoadGenerator> roadGenerator ) = 0;

            /**
             * @brief Gets the array of generated cities.
             *
             * @return Array of smart pointers to IProceduralCity objects representing the generated
             * cities.
             */
            virtual const Array<SmartPtr<IProceduralCity>> getCities() const = 0;

            /**
             * @brief Removes a city from the generated cities list.
             *
             * @param city Smart pointer to the IProceduralCity to remove.
             */
            virtual void removeCity( SmartPtr<IProceduralCity> city ) = 0;

            /**
             * @brief Adds a city to the generated cities list.
             *
             * @param city Smart pointer to the IProceduralCity to add.
             */
            virtual void addCity( SmartPtr<IProceduralCity> city ) = 0;

            String getFilePath() const override = 0;

            void setFilePath( const String &filePath ) override = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // ICityGenerator_h__
