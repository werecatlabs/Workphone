#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CShaderOgreNext.hpp>

#include <OgreException.h>
#include <OgreHighLevelGpuProgramManager.h>
#include <OgreRoot.h>

#include <atomic>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, CShaderOgreNext, Shader );

    namespace
    {
        std::atomic<u64> s_shaderId = 0;

        void addDiagnostic( Array<IShader::Diagnostic> &diagnostics,
                            IShader::DiagnosticSeverity severity, const String &message )
        {
            IShader::Diagnostic diagnostic;
            diagnostic.severity = severity;
            diagnostic.message = message;
            diagnostics.push_back( diagnostic );
        }

        Ogre::String toOgreString( const String &value )
        {
            return Ogre::String( value.c_str() );
        }
    }  // namespace

    CShaderOgreNext::CShaderOgreNext() = default;

    CShaderOgreNext::~CShaderOgreNext()
    {
        releaseProgram();
    }

    String CShaderOgreNext::getOgreLanguage() const
    {
        switch( getLanguage() )
        {
            case Language::GLSL:
                return "glsl";
            case Language::HLSL:
                return "hlsl";
            case Language::Metal:
                return "metal";
            default:
                return String();
        }
    }

    Ogre::GpuProgramType CShaderOgreNext::getOgreProgramType() const
    {
        switch( getStage() )
        {
            case Stage::Vertex:
                return Ogre::GPT_VERTEX_PROGRAM;
            case Stage::Fragment:
                return Ogre::GPT_FRAGMENT_PROGRAM;
            case Stage::Geometry:
                return Ogre::GPT_GEOMETRY_PROGRAM;
            case Stage::TessellationControl:
                return Ogre::GPT_HULL_PROGRAM;
            case Stage::TessellationEvaluation:
                return Ogre::GPT_DOMAIN_PROGRAM;
            case Stage::Compute:
                return Ogre::GPT_COMPUTE_PROGRAM;
            default:
                return Ogre::GPT_VERTEX_PROGRAM;
        }
    }

    bool CShaderOgreNext::isSupportedByOgre() const
    {
        return !getOgreLanguage().empty() &&
               ( getStage() == Stage::Vertex || getStage() == Stage::Fragment ||
                 getStage() == Stage::Geometry || getStage() == Stage::TessellationControl ||
                 getStage() == Stage::TessellationEvaluation || getStage() == Stage::Compute );
    }

    String CShaderOgreNext::getOgreResourceName() const
    {
        const String objectName = getName();
        const String id = String( std::to_string( s_shaderId.fetch_add( 1 ) + 1 ).c_str() );
        if( !objectName.empty() )
            return String( "Workphone/Shader/" ) + objectName + "/" + id;
        return String( "Workphone/Shader/" ) + id;
    }

    bool CShaderOgreNext::compile()
    {
        // Validate the portable contract before touching the live Ogre resource.
        if( !Shader::compile() )
            return false;

        if( !isSupportedByOgre() )
        {
            m_diagnostics.clear();
            addDiagnostic( m_diagnostics, DiagnosticSeverity::Error,
                           "The selected shader stage or language is not supported by OgreNext." );
            m_status = Status::Failed;
            return false;
        }

        if( getSource().empty() )
        {
            m_diagnostics.clear();
            addDiagnostic( m_diagnostics, DiagnosticSeverity::Error,
                           "OgreNext high-level programs require source text; binary-only shaders "
                           "must be handled by a backend-specific implementation." );
            m_status = Status::Failed;
            return false;
        }

        auto *manager = Ogre::HighLevelGpuProgramManager::getSingletonPtr();
        if( !manager )
        {
            m_diagnostics.clear();
            addDiagnostic( m_diagnostics, DiagnosticSeverity::Error,
                           "OgreNext high-level GPU program manager is unavailable." );
            m_status = Status::Failed;
            return false;
        }

        Ogre::HighLevelGpuProgramPtr candidate;
        Ogre::String candidateName;
        try
        {
            candidateName = toOgreString( getOgreResourceName() );
            const Ogre::String group = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;
            candidate = manager->createProgram( candidateName, group, toOgreString( getOgreLanguage() ),
                                                getOgreProgramType() );
            if( candidate.isNull() )
                throw Ogre::Exception( Ogre::Exception::ERR_INTERNAL_ERROR,
                                       "OgreNext returned a null GPU program.", "CShaderOgreNext" );

            if( !getEntryPoint().empty() )
                candidate->setParameter( "entry_point", toOgreString( getEntryPoint() ) );
            if( !getProfile().empty() )
                candidate->setParameter( "target", toOgreString( getProfile() ) );

            String defines;
            for( const auto &define : getDefines() )
            {
                if( define.name.empty() )
                    continue;
                if( !defines.empty() )
                    defines += ";";
                defines += define.name;
                if( !define.value.empty() )
                {
                    defines += "=";
                    defines += define.value;
                }
            }
            if( !defines.empty() )
                candidate->setParameter( "preprocessor_defines", toOgreString( defines ) );

            candidate->setSource( toOgreString( getSource() ) );
            candidate->load();
            releaseProgram();
            m_program = candidate;
            m_status = Status::Compiled;
            m_diagnostics.clear();
            return true;
        }
        catch( const Ogre::Exception &exception )
        {
            m_diagnostics.clear();
            addDiagnostic( m_diagnostics, DiagnosticSeverity::Error,
                           String( "OgreNext shader compilation failed: " ) +
                               String( exception.getDescription().c_str() ) );
            m_status = Status::Failed;
            candidate.reset();
            if( manager && !candidateName.empty() && !manager->getByName( candidateName ).isNull() )
                manager->remove( candidateName );
            return false;
        }
        catch( ... )
        {
            m_diagnostics.clear();
            addDiagnostic( m_diagnostics, DiagnosticSeverity::Error,
                           "Unknown OgreNext shader compilation failure." );
            m_status = Status::Failed;
            candidate.reset();
            if( manager && !candidateName.empty() && !manager->getByName( candidateName ).isNull() )
                manager->remove( candidateName );
            return false;
        }
    }

    bool CShaderOgreNext::reload()
    {
        releaseProgram();
        return compile();
    }

    void CShaderOgreNext::invalidate()
    {
        releaseProgram();
        Shader::invalidate();
    }

    void CShaderOgreNext::getNativeHandle( void **handle ) const
    {
        if( handle )
            *handle = m_program.get();
    }

    void CShaderOgreNext::releaseProgram()
    {
        if( m_program.isNull() )
            return;

        const Ogre::String name = m_program->getName();
        m_program.reset();

        auto *manager = Ogre::HighLevelGpuProgramManager::getSingletonPtr();
        if( manager && !name.empty() && !manager->getByName( name ).isNull() )
            manager->remove( name );
    }

}  // namespace workphone::render
