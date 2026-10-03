//
// Created by Zane Desir on 11/11/2021.
//
#include <GameEditorPCH.hpp>
#include "ui/MaterialWindow.hpp"
#include <wx/wx.hpp>



namespace fb
{
	namespace editor
	{

		MaterialWindow::MaterialWindow()
		{

		}

		MaterialWindow::MaterialWindow(wxWindow* parent, wxWindowID id, const wxPoint& pos,
				const wxSize& size, long style, const wxValidator& validator, const wxString& name)
		{
			try
			{
				auto window = new wxScrolledWindow(parent, id, pos, size, style);
				setWindow(window);

				auto baseSizer = new wxBoxSizer(wxVERTICAL);
				window->SetSizer(baseSizer);
			}
			catch (std::exception& e)
			{
				wxMessageBox(e.what());
			}
		}

		MaterialWindow::~MaterialWindow()
		{

		}

	} // end namespace editor
} // end namespace fb

