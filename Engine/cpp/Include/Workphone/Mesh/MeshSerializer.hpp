#ifndef __WP_MeshSerializer_H__
#define __WP_MeshSerializer_H__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/Serializer.hpp>
#include <Workphone/Mesh/MeshSerializerImpl.hpp>

namespace workphone
{

    /// Mesh compatibility versions
    enum MeshVersion
    {
        /// Latest version available
        MESH_VERSION_LATEST,

        /// OGRE version v1.8+
        MESH_VERSION_1_8,
        /// OGRE version v1.7+
        MESH_VERSION_1_7,
        /// OGRE version v1.4+
        MESH_VERSION_1_4,
        /// OGRE version v1.0+
        MESH_VERSION_1_0,

        /// Legacy versions, DO NOT USE for writing
        MESH_VERSION_LEGACY
    };

    /** Class for serialising mesh data to/from an OGRE .mesh file.
    @remarks
        This class allows exporters to write OGRE .mesh files easily, and allows the
        OGRE engine to import .mesh files into instantiated OGRE Meshes.
        Note that a .mesh file can include not only the Mesh, but also definitions of
        any Materials it uses (although this is optional, the .mesh can rely on the
        Material being loaded from another source, especially useful if you want to
        take advantage of OGRE's advanced Material properties which may not be available
        in your modeller).
    @par
        To export a Mesh:<OL>
        <LI>Use the MaterialManager methods to create any dependent Material objects, if you want
            to export them with the Mesh.</LI>
        <LI>Create a Mesh object and populate it using it's methods.</LI>
        <LI>Call the exportMesh method</LI>
        </OL>
    @par
        It's important to realise that this exporter uses OGRE terminology. In this context,
        'Mesh' means a top-level mesh structure which can actually contain many SubMeshes, each
        of which has only one Material. Modelling packages may refer to these differently, for
        example in Milkshape, it says 'Model' instead of 'Mesh' and 'Mesh' instead of 'SubMesh',
        but the theory is the same.
    */
    class WPCore_API MeshSerializer : public Serializer
    {
    public:
        MeshSerializer();
        ~MeshSerializer() override;

        /** Exports a mesh to the file specified, in the latest format
        @remarks
            This method takes an externally created Mesh object, and exports it
            to a .mesh file in the latest format version available.
        @param pMesh Pointer to the Mesh to export
        @param filename The destination filename
        @param endianMode The endian mode of the written file
        */
        void exportMesh( const Mesh *pMesh, const String &filename,
                         Endian endianMode = Endian::ENDIAN_NATIVE );

        /** Exports a mesh to the file specified, in a specific version format.
         @remarks
         This method takes an externally created Mesh object, and exports it
         to a .mesh file in the specified format version. Note that picking a
         format version other that the latest will cause some information to be
         lost.
         @param pMesh Pointer to the Mesh to export
         @param filename The destination filename
         @param version Mesh version to write
         @param endianMode The endian mode of the written file
         */
        void exportMesh( const Mesh *pMesh, const String &filename, MeshVersion version,
                         Endian endianMode = Endian::ENDIAN_NATIVE );

        /** Exports a mesh to the stream specified, in the latest format.
        @remarks
         This method takes an externally created Mesh object, and exports it
         to a .mesh file in the latest format version.
        @param pMesh Pointer to the Mesh to export
        @param stream Writeable stream
        @param endianMode The endian mode of the written file
        */
        void exportMesh( const Mesh *pMesh, SmartPtr<IStream> stream,
                         Endian endianMode = Endian::ENDIAN_NATIVE );

        /** Exports a mesh to the stream specified, in a specific version format.
         @remarks
         This method takes an externally created Mesh object, and exports it
         to a .mesh file in the specified format version. Note that picking a
         format version other that the latest will cause some information to be
         lost.
         @param pMesh Pointer to the Mesh to export
         @param stream Writeable stream
         @param version Mesh version to write
         @param endianMode The endian mode of the written file
         */
        void exportMesh( const Mesh *pMesh, SmartPtr<IStream> stream, MeshVersion version,
                         Endian endianMode = Endian::ENDIAN_NATIVE );

        /** Imports Mesh and (optionally) Material data from a .mesh file DataStream.
        @remarks
            This method imports data from a DataStream opened from a .mesh file and places it's
            contents into the Mesh object which is passed in.
        @param stream The DataStream holding the .mesh data. Must be initialised (pos at the start of the
        buffer).
        @param pDest Pointer to the Mesh object which will receive the data. Should be blank already.
        */
        void importMesh( SmartPtr<IStream> &stream, Mesh *pDest );

        SmartPtr<IMesh> loadMesh( SmartPtr<IStream> &stream );

        /// Sets the listener for this serializer
        void setListener( MeshSerializerListener *listener );

        /// Returns the current listener
        MeshSerializerListener *getListener();

    protected:
        class MeshVersionData
        {
        public:
            MeshVersion version;
            String versionString;
            MeshSerializerImpl *impl;

            MeshVersionData( MeshVersion _ver, const String &_string, MeshSerializerImpl *_impl );

            ~MeshVersionData();
        };

        using MeshVersionDataList = Array<MeshVersionData *>;
        MeshVersionDataList mVersionData;

        MeshSerializerListener *mListener;
    };

    /** @} */
    /** @} */
}  // namespace workphone

#endif
