#ifndef ActorSelection_h__
#define ActorSelection_h__



#include <GameEditorPrerequisites.hpp>
#include <FBCore/Memory/CSharedObject.hpp>



namespace fb
{
	namespace editor
	{



		class ActorSelection : public CSharedObject<ISharedObject>
		{
		public:
			ActorSelection();
			~ActorSelection();

			String getFilePath() const;
			void setFilePath(const String& val);

			FB_CLASS_REGISTER_DECL;

		protected:
			String m_filePath;
		};



	} // end namespace editor	
} // end namespace fb



#endif // ActorSelection_h__


