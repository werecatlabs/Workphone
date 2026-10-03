#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CComputeShaderOgreNext.hpp>

#include <OgreException.h>
#include <OgreHighLevelGpuProgramManager.h>

#include <atomic>
#include <string>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, CComputeShaderOgreNext, ComputeShader );

    namespace
    {
        std::atomic<u64> s_computeShaderId = 0;

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

    CComputeShaderOgreNext::CComputeShaderOgreNext() = default;

    CComputeShaderOgreNext::~CComputeShaderOgreNext()
    {
        releaseProgram();
    }

    String CComputeShaderOgreNext::getOgreLanguage() const
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

    String CComputeShaderOgreNext::getOgreResourceName() const
    {
        const String objectName = getName();
        const String id = String( std::to_string( s_computeShaderId.fetch_add( 1 ) + 1 ).c_str() );
        if( !objectName.empty() )
            return String( "Workphone/ComputeShader/" ) + objectName + "/" + id;
        return String( "Workphone/ComputeShader/" ) + id;
    }

    bool CComputeShaderOgreNext::compile()
    {
        if( !ComputeShader::compile() )
            return false;

        if( getStage() != Stage::Compute )
        {
            m_diagnostics.clear();
            addDiagnostic( m_diagnostics, DiagnosticSeverity::Error,
                           "OgreNext compute programs require the Compute shader stage." );
            m_status = Status::Failed;
            return false;
        }

        if( getSource().empty() )
        {
            m_diagnostics.clear();
            addDiagnostic( m_diagnostics, DiagnosticSeverity::Error,
                           "OgreNext high-level compute programs require source text." );
            m_status = Status::Failed;
            return false;
        }

        const String language = getOgreLanguage();
        if( language.empty() )
        {
            m_diagnostics.clear();
            addDiagnostic( m_diagnostics, DiagnosticSeverity::Error,
                           "The selected compute shader language is not supported by OgreNext." );
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
            candidate = manager->createProgram( candidateName, group, toOgreString( language ),
                                                Ogre::GPT_COMPUTE_PROGRAM );
            if( candidate.isNull() )
                throw Ogre::Exception( Ogre::Exception::ERR_INTERNAL_ERROR,
                                       "OgreNext returned a null compute GPU program.",
                                       "CComputeShaderOgreNext" );

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
            m_pipelineCreated = false;
            m_pipelineHandle = nullptr;
            m_diagnostics.clear();
            return true;
        }
        catch( const Ogre::Exception &exception )
        {
            m_diagnostics.clear();
            addDiagnostic( m_diagnostics, DiagnosticSeverity::Error,
                           String( "OgreNext compute shader compilation failed: " ) +
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
                           "Unknown OgreNext compute shader compilation failure." );
            m_status = Status::Failed;
            candidate.reset();
            if( manager && !candidateName.empty() && !manager->getByName( candidateName ).isNull() )
                manager->remove( candidateName );
            return false;
        }
    }

    bool CComputeShaderOgreNext::reload()
    {
        return compile();
    }

    void CComputeShaderOgreNext::invalidate()
    {
        releaseProgram();
        ComputeShader::invalidate();
    }

    bool CComputeShaderOgreNext::createPipeline()
    {
        if( !isCompiled() || m_program.isNull() || !validateBindings() )
            return false;

        m_pipelineHandle = m_program.get();
        m_pipelineCreated = true;
        return true;
    }

    void CComputeShaderOgreNext::destroyPipeline()
    {
        m_pipelineCreated = false;
        m_pipelineHandle = nullptr;
    }

    void CComputeShaderOgreNext::getPipelineHandle( void **handle ) const
    {
        if( handle )
            *handle = m_pipelineHandle;
    }

    void CComputeShaderOgreNext::getNativeHandle( void **handle ) const
    {
        if( handle )
            *handle = m_program.get();
    }

    void CComputeShaderOgreNext::releaseProgram()
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
