//
// Created by Zane Desir on 11/11/2021.
//

#ifndef FB_MATERIALWINDOW_H
#define FB_MATERIALWINDOW_H



#include <GameEditorPrerequisites.hpp>
#include <FBWxWidgets/FBWxApplicationWindow.hpp>
#include "core/IMessageListener.hpp"
#include <FBCore/Interface/System/IStateListener.hpp>
#include <wx/treectrl.hpp>



namespace fb
{
	namespace editor
	{

		class MaterialWindow : public ui::wxApplicationWindow
		{
		public:
			MaterialWindow();

			MaterialWindow(wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize, long style = wxVSCROLL | wxHSCROLL,
				const wxValidator& validator = wxDefaultValidator, const wxString& name = "SceneWindow");
			~MaterialWindow();
		};



	} // end namespace editor
} // end namespace fb


#endif //FB_MATERIALWINDOW_H


