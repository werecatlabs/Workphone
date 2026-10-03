#ifndef IOSystem_h__
#define IOSystem_h__

#include <WPAssimp/WPAssimpPrerequisites.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <assimp/IOSystem.hpp>

namespace workphone
{
    class IOSystem : public Assimp::IOSystem
    {
    public:
        IOSystem();
        IOSystem( const SmartPtr<IStream> &src, String grp );

        bool Exists( const char *pFile ) const override;
        char getOsSeparator() const override;

        Assimp::IOStream *Open( const char *pFile, const char *pMode ) override;
        void Close( Assimp::IOStream *pFile ) override;

    protected:
        SmartPtr<IStream> source;
        std::vector<IOStream *> streams;
        String group;
    };
}  // namespace workphone

#endif  // IOSystem_h__
