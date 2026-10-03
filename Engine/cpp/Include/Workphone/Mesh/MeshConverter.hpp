#ifndef _MeshCoverter_H
#define _MeshCoverter_H

#include <Workphone/Interface/Graphics/IMeshConverter.hpp>

namespace workphone
{

    /**
     * @class MeshConverter
     * @brief Provides functionality to convert and write mesh data from game actors.
     *
     * The MeshConverter class implements the IMeshConverter interface to handle
     * the process of extracting mesh information from a scene actor and writing it
     * to a compatible format.
     */
    class WPCore_API MeshConverter : public render::IMeshConverter
    {
    public:
        /**
         * @brief Constructs a new MeshConverter object.
         */
        MeshConverter();

        /**
         * @brief Destroys the MeshConverter object.
         */
        ~MeshConverter() override;

        /**
         * @brief Writes the mesh data of the specified actor to a file or buffer.
         *
         * @param actor A smart pointer to the game actor whose mesh is to be written.
         */
        void writeMesh( const SmartPtr<scene::IGameActor> actor ) override;
    };

}  // namespace workphone

#endif
