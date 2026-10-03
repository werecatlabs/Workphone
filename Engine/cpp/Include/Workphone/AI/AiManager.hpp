#ifndef AiManager_h__
#define AiManager_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Ai/IAiManager.hpp>

namespace workphone
{

    /**
     * @class AiManager
     * @brief Manages interactions with the AI subsystem, including pathfinding and query processing via
     * Ollama API.
     */
    class WPCore_API AiManager : public IAiManager
    {
    public:
        /**
         * @brief Constructor for AiManager.
         */
        AiManager();

        /**
         * @brief Destructor for AiManager.
         */
        ~AiManager() override;

        /**
         * @brief Gets the current pathfinder object.
         * @return A smart pointer to the IPathfinder2 instance.
         */
        SmartPtr<IPathfinder2> getPathfinder2() const override;

        /**
         * @brief Sets the pathfinder2 object.
         * @param pathfinder The smart pointer to the IPathfinder2 object to set.
         */
        void setPathfinder2( SmartPtr<IPathfinder2> pathfinder ) override;

        /**
         * @brief Queries the AI manager for a response based on a prompt.
         * @param prompt The user prompt.
         * @return The query result string.
         */
        String query( const String &prompt ) const override;

        /**
         * @brief Processes the response received from the AI.
         * @param response The response string to process.
         * @return True if the response was successfully processed, false otherwise.
         */
        bool processResponse( const String &response ) const override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Queries the Ollama API for a response using the first query method.
         * @param prompt The prompt to send to Ollama.
         * @return The response string from Ollama.
         */
        String query_ollama( const String &prompt ) const;

        /**
         * @brief Queries the Ollama API for a response using the second query method.
         * @param prompt The prompt to send to Ollama.
         * @return The response string from Ollama.
         */
        String query_ollama2( const String &prompt ) const;

        /**
         * @brief Pathfinding object managed by AiManager.
         * @return Smart pointer to the IPathfinder2 instance.
         */
        SmartPtr<IPathfinder2> m_pathfinder2;
    };

}  // namespace workphone

#endif  // AiManager_h__
