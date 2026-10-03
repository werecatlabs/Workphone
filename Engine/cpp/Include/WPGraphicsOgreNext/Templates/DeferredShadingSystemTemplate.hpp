#ifndef DeferredShadingSystemTemplate_h__
#define DeferredShadingSystemTemplate_h__



#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrequisites.hpp>
#include <Workphone/Interface/Scene/IDirector.hpp>
#include <Workphone/Memory/CSharedObject.hpp>



namespace fb
{


	class DeferredShadingSystemTemplate : public CSharedObject<IObjectDirector>
	{
	public:
		DeferredShadingSystemTemplate();
		~DeferredShadingSystemTemplate();

		String getGBufferCompositorName() const { return m_sGBufferCompositorName; }
		void setGBufferCompositorName(const String& gBufferCompositorName) { m_sGBufferCompositorName = gBufferCompositorName; }

		String getShowLightingCompositorName() const { return m_sShowLightingCompositorName; }
		void setShowLightingCompositorName(const String& showLightingCompositorName) { m_sShowLightingCompositorName = showLightingCompositorName; }

		String getShowNormalsCompositorName() const { return m_sShowNormalsCompositorName; }
		void setShowNormalsCompositorName(const String& showNormalsCompositorName) { m_sShowNormalsCompositorName = showNormalsCompositorName; }

		String getShowDepthSpecularCompositorName() const { return m_sShowDepthSpecularCompositorName; }
		void setShowDepthSpecularCompositorName(const String& showDepthSpecularCompositorName) { m_sShowDepthSpecularCompositorName = showDepthSpecularCompositorName; }

		String getShowColourCompositorName() const { return m_sShowColourCompositorName; }
		void setShowColourCompositorName(const String& showColourCompositorName) { m_sShowColourCompositorName = showColourCompositorName; }

		String getLightCompositionPassName() const { return m_sLightCompositionPassName; }
		void setLightCompositionPassName(const String& lightCompositionPassName) { m_sLightCompositionPassName = lightCompositionPassName; }

		String getRenderTargetName() const { return m_renderTargetName; }
		void setRenderTargetName(const String& renderTargetName) { m_renderTargetName = renderTargetName; }

		bool isDirty() const;
		void setDirty( bool dirty );

	protected:
		String m_sGBufferCompositorName;
		String m_sShowLightingCompositorName;
		String m_sShowNormalsCompositorName;
		String m_sShowDepthSpecularCompositorName;
		String m_sShowColourCompositorName;

		String m_sLightCompositionPassName;

		String m_renderTargetName;
	};



} // end namespace fb



#endif // DeferredShadingSystemTemplate_h__


