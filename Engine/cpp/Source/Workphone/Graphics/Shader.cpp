#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/Shader.hpp>

#include <algorithm>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, Shader, IShader );

    namespace
    {
        void addDiagnostic( Array<IShader::Diagnostic> &diagnostics,
                            IShader::DiagnosticSeverity severity, const String &message )
        {
            IShader::Diagnostic diagnostic;
            diagnostic.severity = severity;
            diagnostic.message = message;
            diagnostics.push_back( diagnostic );
        }

        bool isKnownLanguage( IShader::Language language )
        {
            return language != IShader::Language::Unknown;
        }

        bool isValidStage( IShader::Stage stage )
        {
            return static_cast<u8>( stage ) <= static_cast<u8>( IShader::Stage::Intersection );
        }
    }  // namespace

    Shader::Shader() = default;

    Shader::~Shader() = default;

    IShader::Stage Shader::getStage() const
    {
        return m_stage;
    }

    void Shader::setStage( Stage stage )
    {
        if( isValidStage( stage ) )
            m_stage = stage;
        else
            addDiagnostic( m_diagnostics, DiagnosticSeverity::Error, "Invalid shader stage." );
        if( m_status == Status::Compiled )
            m_status = m_source.empty() && m_binary.empty() ? Status::Empty : Status::SourceReady;
    }

    IShader::Language Shader::getLanguage() const
    {
        return m_language;
    }

    void Shader::setLanguage( Language language )
    {
        m_language = language;
        if( m_status == Status::Compiled )
            m_status = m_source.empty() && m_binary.empty() ? Status::Empty : Status::SourceReady;
    }

    String Shader::getSource() const
    {
        return m_source;
    }

    void Shader::setSource( const String &source )
    {
        m_source = source;
        m_status = source.empty() && m_binary.empty() ? Status::Empty : Status::SourceReady;
    }

    String Shader::getEntryPoint() const
    {
        return m_entryPoint;
    }

    void Shader::setEntryPoint( const String &entryPoint )
    {
        m_entryPoint = entryPoint;
        if( m_status == Status::Compiled )
            m_status = m_source.empty() && m_binary.empty() ? Status::Empty : Status::SourceReady;
    }

    String Shader::getProfile() const
    {
        return m_profile;
    }

    void Shader::setProfile( const String &profile )
    {
        m_profile = profile;
        if( m_status == Status::Compiled )
            m_status = m_source.empty() && m_binary.empty() ? Status::Empty : Status::SourceReady;
    }

    Array<IShader::Define> Shader::getDefines() const
    {
        return m_defines;
    }

    void Shader::setDefines( const Array<Define> &defines )
    {
        m_defines = defines;
        if( m_status == Status::Compiled )
            m_status = m_source.empty() && m_binary.empty() ? Status::Empty : Status::SourceReady;
    }

    void Shader::setDefine( const String &name, const String &value )
    {
        if( name.empty() )
            return;
        auto it = std::find_if( m_defines.begin(), m_defines.end(),
                                [&name]( const Define &define ) { return define.name == name; } );
        if( it == m_defines.end() )
        {
            Define define;
            define.name = name;
            define.value = value;
            m_defines.push_back( define );
        }
        else
            it->value = value;
        if( m_status == Status::Compiled )
            m_status = m_source.empty() && m_binary.empty() ? Status::Empty : Status::SourceReady;
    }

    void Shader::removeDefine( const String &name )
    {
        m_defines.erase(
            std::remove_if( m_defines.begin(), m_defines.end(),
                            [&name]( const Define &define ) { return define.name == name; } ),
            m_defines.end() );
        if( m_status == Status::Compiled )
            m_status = m_source.empty() && m_binary.empty() ? Status::Empty : Status::SourceReady;
    }

    void Shader::clearDefines()
    {
        m_defines.clear();
        if( m_status == Status::Compiled )
            m_status = m_source.empty() && m_binary.empty() ? Status::Empty : Status::SourceReady;
    }

    bool Shader::compile()
    {
        m_status = Status::Compiling;
        m_diagnostics.clear();

        if( !isValidStage( m_stage ) )
            addDiagnostic( m_diagnostics, DiagnosticSeverity::Error, "Invalid shader stage." );
        if( !isKnownLanguage( m_language ) )
            addDiagnostic( m_diagnostics, DiagnosticSeverity::Error,
                           "A source or binary language must be specified." );
        if( m_entryPoint.empty() )
            addDiagnostic( m_diagnostics, DiagnosticSeverity::Error,
                           "Shader entry point cannot be empty." );
        if( m_source.empty() && m_binary.empty() )
            addDiagnostic( m_diagnostics, DiagnosticSeverity::Error,
                           "Shader has neither source nor binary data." );

        if( !m_diagnostics.empty() )
        {
            m_status = Status::Failed;
            return false;
        }

        // This class owns portable shader state and performs contract validation.
        // Render-system implementations should override compile() to invoke their
        // compiler and populate backend binary/reflection data.
        m_status = Status::Compiled;
        return true;
    }

    bool Shader::reload()
    {
        return compile();
    }

    void Shader::invalidate()
    {
        m_binary.clear();
        m_reflection = Reflection();
        m_status = m_source.empty() ? Status::Empty : Status::SourceReady;
    }

    IShader::Status Shader::getStatus() const
    {
        return m_status;
    }

    bool Shader::isCompiled() const
    {
        return m_status == Status::Compiled;
    }

    Array<IShader::Diagnostic> Shader::getDiagnostics() const
    {
        return m_diagnostics;
    }

    void Shader::clearDiagnostics()
    {
        m_diagnostics.clear();
    }

    IShader::Reflection Shader::getReflection() const
    {
        return m_reflection;
    }

    bool Shader::hasResource( const String &name ) const
    {
        return std::find_if( m_reflection.resources.begin(), m_reflection.resources.end(),
                             [&name]( const ResourceBinding &resource ) {
                                 return resource.name == name;
                             } ) != m_reflection.resources.end();
    }

    Array<u8> Shader::getBinary() const
    {
        return m_binary;
    }

    bool Shader::setBinary( const Array<u8> &binary, Language language )
    {
        if( binary.empty() || !isKnownLanguage( language ) )
            return false;
        m_binary = binary;
        m_source.clear();
        m_language = language;
        m_status = Status::Compiled;
        m_diagnostics.clear();
        return true;
    }

    bool Shader::setSpecializationConstant( const String &name, u32 value )
    {
        if( name.empty() )
            return false;
        auto it =
            std::find_if( m_specializationValues.begin(), m_specializationValues.end(),
                          [&name]( const SpecializationValue &entry ) { return entry.name == name; } );
        if( it == m_specializationValues.end() )
        {
            SpecializationValue entry;
            entry.name = name;
            entry.value = value;
            m_specializationValues.push_back( entry );
        }
        else
            it->value = value;
        if( m_status == Status::Compiled )
            m_status = Status::SourceReady;
        return true;
    }

    bool Shader::getSpecializationConstant( const String &name, u32 &value ) const
    {
        auto it =
            std::find_if( m_specializationValues.begin(), m_specializationValues.end(),
                          [&name]( const SpecializationValue &entry ) { return entry.name == name; } );
        if( it == m_specializationValues.end() )
            return false;
        value = it->value;
        return true;
    }

    void Shader::getNativeHandle( void **handle ) const
    {
        if( handle )
            *handle = nullptr;
    }

}  // namespace workphone::render
