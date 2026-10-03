#ifndef CityGeneratorDefault_h__
#define CityGeneratorDefault_h__

#include <Workphone/Interface/Procedural/ICityGenerator.hpp>
#include <WPProcedural/CProceduralGenerator.hpp>
#include <Workphone/Thread/RecursiveMutex.hpp>

namespace workphone
{
    namespace procedural
    {
        /**
         * @brief Default implementation of a procedural city generator.
         *
         * This class provides a concrete implementation of the ICityGenerator interface,
         * responsible for generating procedural cities with terrain, buildings, roads,
         * and other urban elements. It coordinates various generators including terrain,
         * blocks, meshes, and roads to create complete city environments.
         *
         * The generator supports loading configuration from files and can manage multiple
         * cities within a procedural world. It is thread-safe and can be used in
         * multi-threaded environments.
         *
         * @see ICityGenerator
         * @see CProceduralGenerator
         * @see ITerrainGenerator
         * @see IBlockGenerator
         * @see IRoadGenerator
         */
        class WPProcedural_API CityGeneratorDefault : public CProceduralGenerator<ICityGenerator>
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Initializes a new instance of the CityGeneratorDefault class with
             * default values for all member variables.
             */
            CityGeneratorDefault();

            /**
             * @brief Destructor.
             *
             * Cleans up resources and ensures proper cleanup of all managed objects.
             */
            ~CityGeneratorDefault() override;

            /**
             * @brief Loads city generation configuration from a file.
             *
             * @param filePath The path to the configuration file containing city generation parameters.
             *                 The file should contain settings for terrain, buildings, roads, etc.
             *
             * @note This method should be called before generate() to configure the city generation process.
             */
            void loadFromFile( const String &filePath );

            /**
             * @brief Loads the city generator with the specified shared object data.
             *
             * @param data Shared object containing initialization data for the city generator.
             *             This may include configuration parameters, references to other generators,
             * etc.
             *
             * @see ISharedObject
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the city generator and releases associated resources.
             *
             * @param data Shared object containing cleanup data (may be null).
             *
             * @see ISharedObject
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Generates the procedural city.
             *
             * This method orchestrates the city generation process by coordinating
             * all sub-generators (terrain, blocks, roads, meshes) to create a complete
             * city environment. The generation process may be asynchronous.
             *
             * @note Ensure all required generators are set before calling this method.
             * @see isFinished()
             */
            void generate() override;

            /**
             * @brief Checks if the city generation process has completed.
             *
             * @return true if generation is finished, false if still in progress.
             *
             * @see generate()
             */
            bool isFinished() const override;

            /**
             * @brief Gets the terrain generator associated with this city generator.
             *
             * @return Smart pointer to the terrain generator, or null if not set.
             *
             * @see setTerrainGenerator()
             */
            SmartPtr<ITerrainGenerator> getTerrainGenerator() const override;

            /**
             * @brief Sets the terrain generator for this city generator.
             *
             * @param terrainGenerator Smart pointer to the terrain generator to use.
             *
             * @see getTerrainGenerator()
             */
            void setTerrainGenerator( SmartPtr<ITerrainGenerator> terrainGenerator ) override;

            /**
             * @brief Gets the block generator associated with this city generator.
             *
             * @return Smart pointer to the block generator, or null if not set.
             *
             * @see setBlockGenerator()
             */
            SmartPtr<IBlockGenerator> getBlockGenerator() const override;

            /**
             * @brief Sets the block generator for this city generator.
             *
             * @param blockGenerator Smart pointer to the block generator to use.
             *
             * @see getBlockGenerator()
             */
            void setBlockGenerator( SmartPtr<IBlockGenerator> blockGenerator ) override;

            /**
             * @brief Gets the procedural world associated with this city generator.
             *
             * @return Smart pointer to the procedural world, or null if not set.
             *
             * @see setProceduralWorld()
             */
            SmartPtr<IProceduralWorld> getProceduralWorld() const override;

            /**
             * @brief Sets the procedural world for this city generator.
             *
             * @param proceduralWorld Smart pointer to the procedural world to use.
             *
             * @see getProceduralWorld()
             */
            void setProceduralWorld( SmartPtr<IProceduralWorld> proceduralWorld ) override;

            /**
             * @brief Gets the mesh generator associated with this city generator.
             *
             * @return Smart pointer to the mesh generator, or null if not set.
             *
             * @see setMeshGenerator()
             */
            SmartPtr<IMeshGenerator> getMeshGenerator() const override;

            /**
             * @brief Sets the mesh generator for this city generator.
             *
             * @param meshGenerator Smart pointer to the mesh generator to use.
             *
             * @see getMeshGenerator()
             */
            void setMeshGenerator( SmartPtr<IMeshGenerator> meshGenerator ) override;

            /**
             * @brief Gets the road generator associated with this city generator.
             *
             * @return Smart pointer to the road generator, or null if not set.
             *
             * @see setRoadGenerator()
             */
            SmartPtr<IRoadGenerator> getRoadGenerator() const override;

            /**
             * @brief Sets the road generator for this city generator.
             *
             * @param roadGenerator Smart pointer to the road generator to use.
             *
             * @see getRoadGenerator()
             */
            void setRoadGenerator( SmartPtr<IRoadGenerator> roadGenerator ) override;

            /**
             * @brief Gets the list of cities managed by this generator.
             *
             * @return Array of smart pointers to procedural cities.
             *
             * @see addCity()
             * @see removeCity()
             */
            const Array<SmartPtr<IProceduralCity>> getCities() const override;

            /**
             * @brief Removes a city from the generator's management.
             *
             * @param city Smart pointer to the city to remove.
             *
             * @see addCity()
             * @see getCities()
             */
            void removeCity( SmartPtr<IProceduralCity> city ) override;

            /**
             * @brief Adds a city to the generator's management.
             *
             * @param city Smart pointer to the city to add.
             *
             * @see removeCity()
             * @see getCities()
             */
            void addCity( SmartPtr<IProceduralCity> city ) override;

            /**
             * @brief Gets the file path used for loading configuration.
             *
             * @return The file path string, or empty string if not set.
             *
             * @see setFilePath()
             * @see loadFromFile()
             */
            String getFilePath() const;

            /**
             * @brief Sets the file path for configuration loading.
             *
             * @param filePath The file path to set.
             *
             * @see getFilePath()
             * @see loadFromFile()
             */
            void setFilePath( const String &filePath );

            /**
             * @brief Converts the OSM data to properties for serialization.
             *
             * This method extracts relevant properties from the OSM data
             * and returns them as a Properties object for further processing.
             *
             * @return Smart pointer to the Properties object containing OSM data.
             */
            SmartPtr<Properties> osmDataToProperties( const String &filePath ) const;

            /**
             * @brief Class registration declaration for the reflection system.
             *
             * This macro enables runtime type information and serialization support
             * for the CityGeneratorDefault class.
             */
            WP_CLASS_REGISTER_DECL;

        protected:
            /** @brief Path to the configuration file for city generation. */
            String m_filePath;

            /** @brief Terrain generator responsible for creating the city's terrain. */
            SmartPtr<ITerrainGenerator> m_terrainGenerator;

            /** @brief Procedural world that contains the generated city. */
            SmartPtr<IProceduralWorld> m_proceduralWorld;

            /** @brief Block generator responsible for creating building blocks. */
            SmartPtr<IBlockGenerator> m_blockGenerator;

            /** @brief Mesh generator responsible for creating geometric meshes. */
            SmartPtr<IMeshGenerator> m_meshGenerator;

            /** @brief Road generator responsible for creating road networks. */
            SmartPtr<IRoadGenerator> m_roadGenerator;

            /** @brief Array of procedural scenes managed by this generator. */
            Array<SmartPtr<IProceduralScene>> m_scenes;

            /** @brief Array of procedural cities managed by this generator. */
            Array<SmartPtr<IProceduralCity>> m_cities;

            /** @brief Mutex for thread-safe access to generator data. */
            mutable RecursiveMutex m_mutex;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // CityGeneratorDefault_h__
