#ifndef ProceduralManagerDefault_h__
#define ProceduralManagerDefault_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Interface/Procedural/IProceduralManager.hpp>
#include <Workphone/Interface/Procedural/IProceduralCollision.hpp>

namespace workphone
{
    namespace procedural
    {
        /**
         * @class CProceduralManager
         * @brief Implementation of the procedural generation manager.
         *
         * This class coordinates the various generators (City, Terrain) and manages the
         * procedural objects, AI integration, and collision detection within the procedural system.
         */
        class WPProcedural_API CProceduralManager : public IProceduralManager
        {
        public:
            CProceduralManager();
            ~CProceduralManager() override;

            /**
             * @brief Triggers the procedural generation process.
             */
            void generate() override;

            /**
             * @brief Adds a shared object to the procedural manager's tracking.
             * @param object The object to be added.
             */
            void addObject( SmartPtr<ISharedObject> object );

            /**
             * @brief Removes a shared object from the procedural manager.
             * @param object The object to be removed.
             */
            void removeObject( SmartPtr<ISharedObject> object );

            /**
             * @brief Finds a tracked object by its handle.
             * @param handle The unique handle of the object.
             * @return A smart pointer to the found object, or nullptr if not found.
             */
            SmartPtr<ISharedObject> findObject( Handle handle );

            /**
             * @brief Gets a reference to the current AI instance.
             * @return A reference to the smart pointer of the AI object.
             */
            SmartPtr<IAi> &getAi();

            /**
             * @brief Gets a constant reference to the current AI instance.
             * @return A constant reference to the smart pointer of the AI object.
             */
            const SmartPtr<IAi> &getAi() const;

            /**
             * @brief Sets the AI instance for the procedural manager.
             * @param value The AI object to assign.
             */
            void setAi( SmartPtr<IAi> value );

            /**
             * @brief Gets the AI manager responsible for handling AI logic.
             * @return A smart pointer to the AI manager.
             */
            SmartPtr<IAiManager> getAiManager() const;

            /**
             * @brief Sets the AI manager for the procedural system.
             * @param aiManager The AI manager to assign.
             */
            void setAiManager( SmartPtr<IAiManager> aiManager );

            /**
             * @brief Gets the current city generator.
             * @return A smart pointer to the city generator.
             */
            SmartPtr<ICityGenerator> getCityGenerator() const override;

            /**
             * @brief Sets the city generator to be used for procedural city generation.
             * @param value The city generator to assign.
             */
            void setCityGenerator( SmartPtr<ICityGenerator> value ) override;

            /**
             * @brief Gets the current terrain generator.
             * @return A smart pointer to the terrain generator.
             */
            SmartPtr<ITerrainGenerator> getTerrainGenerator() const override;

            /**
             * @brief Sets the terrain generator to be used for procedural terrain generation.
             * @param value The terrain generator to assign.
             */
            void setTerrainGenerator( SmartPtr<ITerrainGenerator> value ) override;

            /**
             * @brief Gets the collision manager for procedural elements.
             * @return A smart pointer to the collision manager.
             */
            SmartPtr<IProceduralCollision> getCollisionManager() const override;

            /**
             * @brief Sets the collision manager for the procedural system.
             * @param value The collision manager to assign.
             */
            void setCollisionManager( SmartPtr<IProceduralCollision> value ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            SmartPtr<IAiManager> m_aiManager;                ///< Manager for AI logic and entities
            SmartPtr<ICityGenerator> m_cityGenerator;        ///< Generator used to create cities
            SmartPtr<ITerrainGenerator> m_terrainGenerator;  ///< Generator used to create terrain
            SmartPtr<IProceduralCollision>
                m_collisionManager;  ///< Handles collision for procedural objects
            SmartPtr<IAi> m_ai;      ///< Current AI instance
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // ProceduralManagerDefault_h__
