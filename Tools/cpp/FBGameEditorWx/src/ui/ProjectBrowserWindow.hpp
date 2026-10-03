//
// Created by Zane Desir on 25/11/2021.
//

#ifndef FB_PROJECTBROWSERWINDOW_H
#define FB_PROJECTBROWSERWINDOW_H

#include <GameEditorPrerequisites.hpp>
#include <FBWxWidgets/FBWxApplicationWindow.hpp>
#include <core/IMessageListener.hpp>
#include <FBCore/Interface/System/IStateListener.hpp>

namespace fb
{
	namespace editor
	{

		class ProjectBrowserWindow : public ui::wxApplicationWindow
		{
		public:
			ProjectBrowserWindow();
			~ProjectBrowserWindow();
		};

	} // end namespace editor
} // end namespace fb



#endif //FB_PROJECTBROWSERWINDOW_H
