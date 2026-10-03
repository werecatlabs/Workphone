#ifndef FileSelection_h__
#define FileSelection_h__



#include <GameEditorPrerequisites.hpp>
#include <FBCore/Memory/CSharedObject.hpp>



namespace fb
{
	namespace editor
	{



		class FileSelection : public CSharedObject<ISharedObject>
		{
		public:
			FileSelection();
			~FileSelection();

			String getFilePath() const;
			void setFilePath(const String& val);

			FB_CLASS_REGISTER_DECL;

		protected:
			String m_filePath;
		};



	} // end namespace editor	
} // end namespace fb



#endif // FileSelection_h__


