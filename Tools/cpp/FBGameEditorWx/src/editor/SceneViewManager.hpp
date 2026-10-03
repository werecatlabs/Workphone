#ifndef MeshViewManager_h__
#define MeshViewManager_h__



#include <GameEditorPrerequisites.hpp>
#include <FBCore/Memory/CSharedObject.hpp>
#include <FBCore/Math/AABB3.hpp>
#include <FBCore/Interface/System/IStateListener.hpp>



namespace fb
{	
	namespace editor
	{



		//-------------------------------------------------
		class SceneViewManager : public CSharedObject<ISharedObject> 
		{
		public:
			SceneViewManager();
			~SceneViewManager();

			void update(time_interval t, time_interval dt);

			void initCamera( const AABB3F &aabb );

			SmartPtr<MeshTemplate> getMeshTemplate() const;
			void setMeshTemplate(SmartPtr<MeshTemplate> meshTemplate);

			SmartPtr<ParticleSystemTemplate> getParticleTemplate() const;
			void setParticleTemplate(SmartPtr<ParticleSystemTemplate> val);

			void createDestructible(const String& fileName);
			SmartPtr<render::ISceneNode> createMeshSceneNode(const String& fileName);

			void setScene(const String& filePath);
			void setMesh(const String& filePath);
			void addOverlay(const String& filePath);

			void refresh();

		protected:
			class SceneViewManagerStateListener : public CSharedObject<IStateListener>
			{
			public:
				SceneViewManagerStateListener(SceneViewManager* sceneViewManager);

				void handleStateChanged( const SmartPtr<IStateMessage>& message );
				void handleStateChanged( const SmartPtr<IState>& state );
				void handleQuery( SmartPtr<IStateQuery>& query );

				SceneViewManager* m_sceneViewManager;
			};

			SmartPtr<IStateListener> m_stateListener;
			SmartPtr<IStateObject> m_stateObject;

			SmartPtr<ParticleSystemTemplate> m_particleTemplate;

			SmartPtr<MeshTemplate> m_meshTemplate;
			//StateQueryAABB3FPtr m_aabbQuery;

			//SmartPtr<IMap> m_map;
			
			SmartPtr<render::ISceneNode> m_rootNode;
			Array<SmartPtr<render::ISceneNode>> m_nodes;
			Array<SmartPtr<IActor>> m_entities;
		};



	} // end namespace editor	
} // end namespace fb	



#endif // MeshViewManager_h__


