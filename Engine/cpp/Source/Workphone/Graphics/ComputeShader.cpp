#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/ComputeShader.hpp>

#include <algorithm>
#include <limits>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone::render, ComputeShader, IComputeShader );

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
        }  // namespace

        ComputeShader::ComputeShader() = default;

        ComputeShader::~ComputeShader() = default;

        IComputeShader::WorkgroupSize ComputeShader::getWorkgroupSize() const
        {
            return {};
        }

        void ComputeShader::setWorkgroupSize( const WorkgroupSize &size )
        {
        }

        IComputeShader::DispatchSize ComputeShader::getDispatchSize() const
        {
            return {};
        }

        void ComputeShader::setDispatchSize( const DispatchSize &size )
        {
        }

        bool ComputeShader::validateDispatch( const DispatchSize &size, String *errorMessage ) const
        {
            const u64 workgroupCount = static_cast<u64>( size.x ) * size.y * size.z;
            const u64 localCount =
                static_cast<u64>( m_workgroupSize.x ) * m_workgroupSize.y * m_workgroupSize.z;

            if( size.x == 0 || size.y == 0 || size.z == 0 )
            {
                if( errorMessage )
                    *errorMessage = "Dispatch dimensions must be greater than zero.";
                return false;
            }

            // These are conservative portable defaults. Backends may override this
            // method when their device exposes different limits.
            if( size.x > 65535 || size.y > 65535 || size.z > 65535 )
            {
                if( errorMessage )
                    *errorMessage = "Dispatch dimensions exceed the portable 65535 limit.";
                return false;
            }

            if( m_workgroupSize.x == 0 || m_workgroupSize.y == 0 || m_workgroupSize.z == 0 ||
                localCount > 1024 )
            {
                if( errorMessage )
                    *errorMessage = "The workgroup size is invalid or exceeds 1024 invocations.";
                return false;
            }

            if( workgroupCount > std::numeric_limits<u32>::max() )
            {
                if( errorMessage )
                    *errorMessage = "Dispatch contains too many workgroups.";
                return false;
            }

            return true;
        }

        Array<IComputeShader::ComputeBinding> ComputeShader::getComputeBindings() const
        {
            return m_computeBindings;
        }

        bool ComputeShader::getComputeBinding( const String &name, ComputeBinding &binding ) const
        {
            auto it = std::find_if(
                m_computeBindings.begin(), m_computeBindings.end(),
                [&name]( const ComputeBinding &candidate ) { return candidate.name == name; } );
            if( it == m_computeBindings.end() )
                return false;
            binding = *it;
            return true;
        }

        bool ComputeShader::hasComputeBinding( const String &name ) const
        {
            ComputeBinding binding;
            return getComputeBinding( name, binding );
        }

        bool ComputeShader::setBindingOverride( const String &name, u32 set, u32 binding )
        {
            if( name.empty() )
                return false;

            auto it =
                std::find_if( m_bindingOverrides.begin(), m_bindingOverrides.end(),
                              [&name]( const BindingOverride &value ) { return value.name == name; } );
            if( it == m_bindingOverrides.end() )
            {
                BindingOverride value;
                value.name = name;
                value.set = set;
                value.binding = binding;
                m_bindingOverrides.push_back( value );
            }
            else
            {
                it->set = set;
                it->binding = binding;
            }
            m_pipelineCreated = false;
            return hasComputeBinding( name );
        }

        void ComputeShader::clearBindingOverrides()
        {
            m_bindingOverrides.clear();
            m_pipelineCreated = false;
        }

        u32 ComputeShader::getSharedMemorySize() const
        {
            return m_sharedMemorySize;
        }

        void ComputeShader::setSharedMemorySize( u32 bytes )
        {
            m_sharedMemorySize = bytes;
            m_pipelineCreated = false;
        }

        bool ComputeShader::validateBindings( Array<Diagnostic> *diagnostics ) const
        {
            Array<Diagnostic> localDiagnostics;
            for( const auto &binding : m_computeBindings )
            {
                if( binding.name.empty() )
                    addDiagnostic( localDiagnostics, DiagnosticSeverity::Error,
                                   "A compute binding has no name." );
                if( binding.arraySize == 0 )
                    addDiagnostic(
                        localDiagnostics, DiagnosticSeverity::Error,
                        String( "Compute binding '" ) + binding.name + "' has an empty array." );
            }

            for( size_Num i = 0; i < m_computeBindings.size(); ++i )
            {
                for( size_Num j = i + 1; j < m_computeBindings.size(); ++j )
                {
                    const auto &a = m_computeBindings[i];
                    const auto &b = m_computeBindings[j];
                    if( a.set == b.set && a.binding == b.binding )
                        addDiagnostic( localDiagnostics, DiagnosticSeverity::Error,
                                       String( "Compute bindings '" ) + a.name + "' and '" + b.name +
                                           "' use the same set and binding." );
                }
            }

            if( diagnostics )
                *diagnostics = localDiagnostics;
            return localDiagnostics.empty();
        }

        bool ComputeShader::createPipeline()
        {
            if( !isCompiled() || !validateBindings() )
                return false;
            m_pipelineCreated = true;
            return true;
        }

        void ComputeShader::destroyPipeline()
        {
            m_pipelineCreated = false;
            m_pipelineHandle = nullptr;
        }

        bool ComputeShader::hasPipeline() const
        {
            return m_pipelineCreated;
        }

        void ComputeShader::getPipelineHandle( void **handle ) const
        {
            if( handle )
                *handle = m_pipelineHandle;
        }

        bool ComputeShader::setIndirectDispatchSource( void *buffer, u32 offset )
        {
            if( !buffer )
                return false;
            m_indirectDispatchBuffer = buffer;
            m_indirectDispatchOffset = offset;
            return true;
        }

        void ComputeShader::clearIndirectDispatchSource()
        {
            m_indirectDispatchBuffer = nullptr;
            m_indirectDispatchOffset = 0;
        }

        bool ComputeShader::hasIndirectDispatchSource() const
        {
            return m_indirectDispatchBuffer != nullptr;
        }

        IShader::Stage ComputeShader::getStage() const
        {
            return m_stage;
        }

        void ComputeShader::setStage( Stage stage )
        {
            m_stage = Stage::Compute;
            if( stage != Stage::Compute )
                addDiagnostic( m_diagnostics, DiagnosticSeverity::Error,
                               "A compute shader must use the Compute stage." );
        }

        IShader::Language ComputeShader::getLanguage() const
        {
            return m_language;
        }

        void ComputeShader::setLanguage( Language language )
        {
            m_language = language;
            m_pipelineCreated = false;
            if( m_status == Status::Compiled )
                m_status = Status::SourceReady;
        }

        String ComputeShader::getSource() const
        {
            return m_source;
        }

        void ComputeShader::setSource( const String &source )
        {
            m_source = source;
            m_binary.clear();
            m_pipelineCreated = false;
            m_status = source.empty() ? Status::Empty : Status::SourceReady;
        }

        String ComputeShader::getEntryPoint() const
        {
            return m_entryPoint;
        }

        void ComputeShader::setEntryPoint( const String &entryPoint )
        {
            m_entryPoint = entryPoint;
            m_pipelineCreated = false;
        }

        String ComputeShader::getProfile() const
        {
            return m_profile;
        }

        void ComputeShader::setProfile( const String &profile )
        {
            m_profile = profile;
            m_pipelineCreated = false;
        }

        Array<IShader::Define> ComputeShader::getDefines() const
        {
            return m_defines;
        }

        void ComputeShader::setDefines( const Array<Define> &defines )
        {
            m_defines = defines;
            m_pipelineCreated = false;
        }

        void ComputeShader::setDefine( const String &name, const String &value )
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
            m_pipelineCreated = false;
        }

        void ComputeShader::removeDefine( const String &name )
        {
            m_defines.erase(
                std::remove_if( m_defines.begin(), m_defines.end(),
                                [&name]( const Define &define ) { return define.name == name; } ),
                m_defines.end() );
            m_pipelineCreated = false;
        }

        void ComputeShader::clearDefines()
        {
            m_defines.clear();
            m_pipelineCreated = false;
        }

        bool ComputeShader::compile()
        {
            m_diagnostics.clear();
            m_pipelineCreated = false;
            if( m_stage != Stage::Compute )
                addDiagnostic( m_diagnostics, DiagnosticSeverity::Error,
                               "A compute shader must use the Compute stage." );
            if( m_entryPoint.empty() )
                addDiagnostic( m_diagnostics, DiagnosticSeverity::Error,
                               "Compute shader entry point cannot be empty." );
            if( !isKnownLanguage( m_language ) )
                addDiagnostic( m_diagnostics, DiagnosticSeverity::Error,
                               "A source or binary language must be specified." );
            if( m_source.empty() && m_binary.empty() )
                addDiagnostic( m_diagnostics, DiagnosticSeverity::Error,
                               "Compute shader has neither source nor binary data." );
            if( !validateDispatch( m_dispatchSize ) )
                addDiagnostic( m_diagnostics, DiagnosticSeverity::Error,
                               "Compute shader dispatch configuration is invalid." );
            if( !validateBindings( &m_diagnostics ) )
            {
                m_status = Status::Failed;
                return false;
            }
            if( !m_diagnostics.empty() )
            {
                m_status = Status::Failed;
                return false;
            }

            // This base class validates and owns shader data. Backend subclasses
            // should override compile() to invoke their compiler and populate binary
            // and reflection data.
            m_status = Status::Compiled;
            return true;
        }

        bool ComputeShader::reload()
        {
            return compile();
        }

        void ComputeShader::invalidate()
        {
            destroyPipeline();
            m_binary.clear();
            m_reflection = Reflection();
            m_status = m_source.empty() ? Status::Empty : Status::SourceReady;
        }

        IShader::Status ComputeShader::getStatus() const
        {
            return m_status;
        }

        bool ComputeShader::isCompiled() const
        {
            return m_status == Status::Compiled;
        }

        Array<IShader::Diagnostic> ComputeShader::getDiagnostics() const
        {
            return m_diagnostics;
        }

        void ComputeShader::clearDiagnostics()
        {
            m_diagnostics.clear();
        }

        IShader::Reflection ComputeShader::getReflection() const
        {
            return m_reflection;
        }

        bool ComputeShader::hasResource( const String &name ) const
        {
            return hasComputeBinding( name );
        }

        Array<u8> ComputeShader::getBinary() const
        {
            return m_binary;
        }

        bool ComputeShader::setBinary( const Array<u8> &binary, Language language )
        {
            if( binary.empty() || !isKnownLanguage( language ) )
                return false;
            m_binary = binary;
            m_source.clear();
            m_language = language;
            m_status = Status::Compiled;
            m_diagnostics.clear();
            m_pipelineCreated = false;
            return true;
        }

        bool ComputeShader::setSpecializationConstant( const String &name, u32 value )
        {
            if( name.empty() )
                return false;
            auto it = std::find_if(
                m_specializationValues.begin(), m_specializationValues.end(),
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
            m_pipelineCreated = false;
            return true;
        }

        bool ComputeShader::getSpecializationConstant( const String &name, u32 &value ) const
        {
            auto it = std::find_if(
                m_specializationValues.begin(), m_specializationValues.end(),
                [&name]( const SpecializationValue &entry ) { return entry.name == name; } );
            if( it == m_specializationValues.end() )
                return false;
            value = it->value;
            return true;
        }

        void ComputeShader::getNativeHandle( void **handle ) const
        {
            if( handle )
                *handle = nullptr;
        }

    }  // namespace render
}  // namespace workphone
