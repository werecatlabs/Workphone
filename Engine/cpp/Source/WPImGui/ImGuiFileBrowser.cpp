#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiFileBrowser.hpp>

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#    define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_internal.h"

#include <iostream>
#include <functional>
#include <climits>
#include <cstring>
#include <sstream>
#include <cwchar>
#include <cctype>
#include <algorithm>
#include <cmath>
#include <utility>

#include <sys/stat.h>

#if defined( WIN32 ) || defined( _WIN32 ) || defined( __WIN32 )
#    define OSWIN
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#    include <WPImGui/Extern/Dirent/dirent.h>
#    include <windows.h>
#else
#    include <dirent.h>
#endif  // defined (WIN32) || defined (_WIN32)

namespace workphone::ui
{
    ImGuiFileBrowser::ImGuiFileBrowser()
    {
        filter_mode = FilterMode_Files | FilterMode_Dirs;

        show_inputbar_combobox = false;
        validate_file = false;
        show_hidden = false;
        is_dir = false;
        filter_dirty = true;
        is_appearing = true;
        show_all_valid_files = false;

        col_items_limit = 12;
        selected_idx = -1;
        selected_ext_idx = 0;
        ext_box_width = -1.0f;
        col_width = 280.0f;
        min_size = ImVec2( 500, 300 );

        invfile_modal_id = "Invalid File!";
        repfile_modal_id = "Replace File?";
        selected_fn = "";
        selected_path = "";
        input_fn[0] = '\0';

#ifdef OSWIN
        current_path = "./";
#else
        initCurrentPath();
#endif
    }

    ImGuiFileBrowser::~ImGuiFileBrowser() = default;

    void ImGuiFileBrowser::update()
    {
        if( triggerOpen )
        {
            ImGui::OpenPopup( "FileBrowser" );
            triggerOpen = false;
        }

        auto fileExtension = getFileExtension();
        auto mode = getDialogMode();

        showFileDialog( "FileBrowser", mode, ImVec2( 0, 0 ), fileExtension.c_str() );
    }

    bool ImGuiFileBrowser::show()
    {
        triggerOpen = true;
        return true;
    }

    void ImGuiFileBrowser::clearFileList()
    {
        // Clear pointer references to subdirs and subfiles
        filtered_dirs.clear();
        filtered_files.clear();
        inputcb_filter_files.clear();

        // Now clear subdirs and subfiles
        subdirs.clear();
        subfiles.clear();
        filter_dirty = true;
        selected_idx = -1;
    }

    void ImGuiFileBrowser::closeDialog()
    {
        valid_types = "";
        valid_exts.clear();
        selected_ext_idx = 0;
        selected_idx = -1;

        input_fn[0] = '\0';  // Hide any text in Input bar for the next time save dialog is opened.
        filter.Clear();      // Clear Filter for the next time open dialog is called.

        show_inputbar_combobox = false;
        validate_file = false;
        show_hidden = false;
        is_dir = false;
        filter_dirty = true;
        is_appearing = true;

        // Clear pointer references to subdirs and subfiles
        filtered_dirs.clear();
        filtered_files.clear();
        inputcb_filter_files.clear();

        // Now clear subdirs and subfiles
        subdirs.clear();
        subfiles.clear();

        ImGui::CloseCurrentPopup();

        //auto listener = getListener();
        //if(listener)
        //{
        //    listener->setElement( this );

        //    if(!StringUtil::isNullOrEmpty( selected_fn ))
        //    {
        //        listener->handleSelection();
        //    }
        //    else
        //    {
        //        listener->handleClose();
        //    }
        //}
    }

    String ImGuiFileBrowser::getFilePath() const
    {
        return selected_path.c_str();
    }

    void ImGuiFileBrowser::setFilePath( const String &filePath )
    {
        selected_path = StringUtil::str( filePath );
    }

    String ImGuiFileBrowser::getFileExtension() const
    {
        return extension.c_str();
    }

    void ImGuiFileBrowser::setFileExtension( const String &fileExtension )
    {
        extension = StringUtil::str( fileExtension );
    }

    IUIFileBrowser::DialogMode ImGuiFileBrowser::getDialogMode() const
    {
        return dialog_mode;
    }

    void ImGuiFileBrowser::setDialogMode( DialogMode mode )
    {
        dialog_mode = mode;
    }

    IUIFileBrowser::FilterMode ImGuiFileBrowser::getFilterMode() const
    {
        return static_cast<IUIFileBrowser::FilterMode>( filter_mode );
    }

    void ImGuiFileBrowser::setFilterMode( IUIFileBrowser::FilterMode mode )
    {
        filter_mode = static_cast<int>( mode );
    }

    bool ImGuiFileBrowser::showFileDialog( const std::string &label, const DialogMode mode,
                                           const ImVec2 &sz_xy, const std::string &valid_types )
    {
        dialog_mode = mode;

        ImGuiIO &io = ImGui::GetIO();
        max_size.x = io.DisplaySize.x;
        max_size.y = io.DisplaySize.y;
        ImGui::SetNextWindowSizeConstraints( min_size, max_size );
        ImGui::SetNextWindowPos( io.DisplaySize * 0.5f, ImGuiCond_Appearing, ImVec2( 0.5f, 0.5f ) );
        ImGui::SetNextWindowSize(
            ImVec2( std::max<float>( sz_xy.x, min_size.x ), std::max<float>( sz_xy.y, min_size.y ) ),
            ImGuiCond_Appearing );

        // Set Proper Filter Mode.
        if( mode == DialogMode::Select )
        {
            filter_mode = FilterMode_Dirs;
        }
        else
        {
            filter_mode = FilterMode_Files | FilterMode_Dirs;
        }

        bool showPopup = true;
        if( ImGui::BeginPopupModal( label.c_str(), &showPopup,
                                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse ) )
        {
            bool show_error = false;

            // If this is the initial run, read current directory and load data once.
            if( is_appearing )
            {
                selected_fn.clear();
                selected_path.clear();
                if( mode != DialogMode::Select )
                {
                    this->valid_types = valid_types;
                    setValidExtTypes( valid_types );
                }

                /* If current path is empty (can happen on Windows if user closes dialog while inside
                 * MyComputer. Since this is a virtual folder, path would be empty) load the drives
                 * on Windows else initialize the current path on Unix.
                 */
                if( current_path.empty() )
                {
#ifdef OSWIN
                    show_error |= !( loadWindowsDrives() );
#else
                    initCurrentPath();
                    show_error |= !( readDIR( current_path ) );
#endif  // OSWIN
                }
                else
                {
                    show_error |= !( readDIR( current_path ) );
                }
                is_appearing = false;
            }

            show_error |= renderNavAndSearchBarRegion();
            show_error |= renderFileListRegion();
            show_error |= renderInputTextAndExtRegion();
            show_error |= renderButtonsAndCheckboxRegion();

            if( validate_file )
            {
                validate_file = false;
                bool check = validateFile();

                if( !check && dialog_mode == DialogMode::Open )
                {
                    ImGui::OpenPopup( invfile_modal_id.c_str() );
                    selected_fn.clear();
                    selected_path.clear();
                }

                else if( !check && dialog_mode == DialogMode::Save )
                {
                    ImGui::OpenPopup( repfile_modal_id.c_str() );
                }
                else if( !check && dialog_mode == DialogMode::Select )
                {
                    selected_fn.clear();
                    selected_path.clear();
                    show_error = true;
                    error_title = "Invalid Directory!";
                    error_msg = "Invalid Directory Selected. Please make sure the directory exists.";
                }

                // If selected file passes through validation check, set path to the file and close
                // file dialog
                if( check )
                {
                    selected_path = current_path + selected_fn;

                    // Add a trailing "/" to emphasize its a directory not a file. If you want just
                    // the dir name it's accessible through "selected_fn"
                    if( dialog_mode == DialogMode::Select )
                    {
                        selected_path += "/";
                    }

                    closeDialog();
                }
            }

            // We don't need to check as the modals will only be shown if OpenPopup is called
            showInvalidFileModal();
            if( showReplaceFileModal() )
            {
                closeDialog();
            }

            // Show Error Modal if there was an error opening any directory
            if( show_error )
            {
                ImGui::OpenPopup( error_title.c_str() );
            }
            showErrorModal();

            ImGui::EndPopup();
            return ( !selected_fn.empty() && !selected_path.empty() );
        }

        return false;
    }

    bool ImGuiFileBrowser::renderNavAndSearchBarRegion()
    {
        ImGuiStyle &style = ImGui::GetStyle();
        bool show_error = false;
        float frame_height = ImGui::GetFrameHeight();
        float list_item_height = GImGui->FontSize + style.ItemSpacing.y;

        ImVec2 pw_content_size = ImGui::GetWindowSize() - style.WindowPadding * 2.0;
        auto sw_size = ImVec2( ImGui::CalcTextSize( "Random" ).x + 140,
                               style.WindowPadding.y * 2.0f + frame_height );
        ImVec2 sw_content_size = sw_size - style.WindowPadding * 2.0;
        auto nw_size = ImVec2( pw_content_size.x - style.ItemSpacing.x - sw_size.x, sw_size.y );

        ImGui::BeginChild( "##NavigationWindow", nw_size, true,
                           ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar );

        ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 0.882f, 0.745f, 0.078f, 1.0f ) );
        for( Array<std::string>::size_type i = 0; i < current_dirlist.size(); i++ )
        {
            if( ImGui::Button( current_dirlist[i].c_str() ) )
            {
                // If last button clicked, nothing happens
                if( i != current_dirlist.size() - 1 )
                {
                    show_error |= !( onNavigationButtonClick( static_cast<int>( i ) ) );
                }
            }

            // Draw Arrow Buttons
            if( i != current_dirlist.size() - 1 )
            {
                ImGui::SameLine( 0, 0 );
                float next_label_width = ImGui::CalcTextSize( current_dirlist[i + 1].c_str() ).x;

                if( i + 1 < current_dirlist.size() - 1 )
                {
                    next_label_width += frame_height + ImGui::CalcTextSize( ">>" ).x;
                }

                if( ImGui::GetCursorPosX() + next_label_width >=
                    ( nw_size.x - style.WindowPadding.x * 3.0 ) )
                {
                    ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 1.0f, 1.0f, 1.0f, 0.01f ) );
                    ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 1.0f, 1.0f, 1.0f, 1.0f ) );

                    // Render a drop down of navigation items on button press
                    if( ImGui::Button( ">>" ) )
                    {
                        ImGui::OpenPopup( "##NavBarDropboxPopup" );
                    }
                    if( ImGui::BeginPopup( "##NavBarDropboxPopup" ) )
                    {
                        ImGui::PushStyleColor( ImGuiCol_FrameBg,
                                               ImVec4( 0.125f, 0.125f, 0.125f, 1.0f ) );
                        if( ImGui::BeginListBox( "##NavBarDropBox", ImVec2( 0, list_item_height * 5 ) ) )
                        {
                            ImGui::PushStyleColor( ImGuiCol_Text,
                                                   ImVec4( 0.882f, 0.745f, 0.078f, 1.0f ) );
                            for( Array<std::string>::size_type j = i + 1; j < current_dirlist.size();
                                 j++ )
                            {
                                if( ImGui::Selectable( current_dirlist[j].c_str(), false ) &&
                                    j != current_dirlist.size() - 1 )
                                {
                                    show_error |= !( onNavigationButtonClick( static_cast<int>( j ) ) );
                                    ImGui::CloseCurrentPopup();
                                }
                            }
                            ImGui::PopStyleColor();
                            ImGui::EndListBox();
                        }
                        ImGui::PopStyleColor();
                        ImGui::EndPopup();
                    }
                    ImGui::PopStyleColor( 2 );
                    break;
                }
                ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 1.0f, 1.0f, 1.0f, 0.01f ) );
                ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 1.0f, 1.0f, 1.0f, 1.0f ) );
                ImGui::ArrowButtonEx( "##Right", ImGuiDir_Right, ImVec2( frame_height, frame_height ),
                                      ImGuiItemFlags_Disabled );
                ImGui::SameLine( 0, 0 );
                ImGui::PopStyleColor( 2 );
            }
        }
        ImGui::PopStyleColor();
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild( "##SearchWindow", sw_size, true,
                           ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar );

        // Render Search/Filter bar
        float marker_width = ImGui::CalcTextSize( "(?)" ).x + style.ItemSpacing.x;
        if( filter.Draw( "##SearchBar", sw_content_size.x - marker_width ) || filter_dirty )
        {
            filterFiles( filter_mode );
        }

        // If filter bar was focused clear selection
        if( ImGui::GetFocusID() == ImGui::GetID( "##SearchBar" ) )
        {
            selected_idx = -1;
        }

        ImGui::SameLine();
        showHelpMarker( "Filter (inc, -exc)" );

        ImGui::EndChild();
        return show_error;
    }

    bool ImGuiFileBrowser::renderFileListRegion()
    {
        ImGuiStyle &style = ImGui::GetStyle();
        ImVec2 pw_size = ImGui::GetWindowSize();
        bool show_error = false;
        float list_item_height = ImGui::CalcTextSize( "" ).y + style.ItemSpacing.y;
        float input_bar_ypos =
            pw_size.y - ImGui::GetFrameHeightWithSpacing() * 2.5f - style.WindowPadding.y;
        float window_height = input_bar_ypos - ImGui::GetCursorPosY() - style.ItemSpacing.y;
        float window_content_height = window_height - style.WindowPadding.y * 2.0f;
        float min_content_size = pw_size.x - style.WindowPadding.x * 4.0f;

        if( window_content_height <= 0.0f )
        {
            return show_error;
        }

        // Reinitialize the limit on number of selectables in one column based on height
        col_items_limit =
            static_cast<int>( std::max<float>( 1.0f, window_content_height / list_item_height ) );
        int num_cols = static_cast<int>( std::max<float>(
            1.0f, std::ceil( static_cast<float>( filtered_dirs.size() + filtered_files.size() ) /
                             col_items_limit ) ) );

        // Limitation by ImGUI in 1.75. If columns are greater than 64 readjust the limit on items
        // per column and recalculate number of columns
        if( num_cols > 64 )
        {
            int exceed_items_amount = ( num_cols - 64 ) * col_items_limit;
            col_items_limit += static_cast<int>( std::ceil( exceed_items_amount / 64.0 ) );
            num_cols = static_cast<int>( std::max<float>(
                1.0f, std::ceil( static_cast<float>( filtered_dirs.size() + filtered_files.size() ) /
                                 col_items_limit ) ) );
        }

        float content_width = num_cols * col_width;
        if( content_width < min_content_size )
        {
            content_width = 0;
        }

        ImGui::SetNextWindowContentSize( ImVec2( content_width, 0 ) );
        ImGui::BeginChild( "##ScrollingRegion", ImVec2( 0, window_height ), true,
                           ImGuiWindowFlags_HorizontalScrollbar );
        ImGui::Columns( num_cols );

        // Output directories in yellow
        ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 0.882f, 0.745f, 0.078f, 1.0f ) );
        int items = 0;
        for( Array<const Info *>::size_type i = 0; i < filtered_dirs.size(); i++ )
        {
            if( !filtered_dirs[i]->is_hidden || show_hidden )
            {
                items++;
                if( ImGui::Selectable( filtered_dirs[i]->name.c_str(),
                                       selected_idx == static_cast<int>( i ) && is_dir,
                                       ImGuiSelectableFlags_AllowDoubleClick ) )
                {
                    selected_idx = static_cast<int>( i );
                    is_dir = true;

                    // If dialog mode is SELECT then copy the selected dir name to the input text bar
                    if( dialog_mode == DialogMode::Select )
                    {
                        strcpy( input_fn, filtered_dirs[i]->name.c_str() );
                    }

                    if( ImGui::IsMouseDoubleClicked( 0 ) )
                    {
                        show_error |= !( onDirClick( static_cast<int>( i ) ) );
                        break;
                    }
                }
                if( ( items ) % col_items_limit == 0 )
                {
                    ImGui::NextColumn();
                }
            }
        }
        ImGui::PopStyleColor( 1 );

        // Output files
        for( Array<const Info *>::size_type i = 0; i < filtered_files.size(); i++ )
        {
            if( !filtered_files[i]->is_hidden || show_hidden )
            {
                items++;
                if( ImGui::Selectable( filtered_files[i]->name.c_str(),
                                       selected_idx == static_cast<int>( i ) && !is_dir,
                                       ImGuiSelectableFlags_AllowDoubleClick ) )
                {
                    // int len = filtered_files[i]->name.length();
                    selected_idx = static_cast<int>( i );
                    is_dir = false;

                    // If dialog mode is OPEN/SAVE then copy the selected file name to the input text
                    // bar
                    strcpy( input_fn, filtered_files[i]->name.c_str() );

                    if( ImGui::IsMouseDoubleClicked( 0 ) )
                    {
                        selected_fn = filtered_files[i]->name;
                        validate_file = true;
                    }
                }
                if( ( items ) % col_items_limit == 0 )
                {
                    ImGui::NextColumn();
                }
            }
        }
        ImGui::Columns( 1 );
        ImGui::EndChild();

        return show_error;
    }

    bool ImGuiFileBrowser::renderInputTextAndExtRegion()
    {
        std::string label = ( dialog_mode == DialogMode::Save ) ? "Save As:" : "Open:";
        ImGuiStyle &style = ImGui::GetStyle();

        ImVec2 pw_pos = ImGui::GetWindowPos();
        ImVec2 pw_content_sz = ImGui::GetWindowSize() - style.WindowPadding * 2.0;
        ImVec2 cursor_pos = ImGui::GetCursorPos();

        float label_width = ImGui::CalcTextSize( label.c_str() ).x + style.ItemSpacing.x;
        float frame_height_spacing = ImGui::GetFrameHeightWithSpacing();
        float input_bar_width = pw_content_sz.x - label_width;

        if( ext_box_width < 0.0 )
        {
            ext_box_width = ImGui::CalcTextSize( "All Valid Files" ).x + style.ItemSpacing.x +
                            ImGui::GetFrameHeightWithSpacing() + 10;
        }

        if( dialog_mode != DialogMode::Select )
        {
            input_bar_width -= ( ext_box_width + style.ItemSpacing.x );
        }

        bool show_error = false;
        ImGui::SetCursorPosY( pw_content_sz.y - frame_height_spacing * 2.0f );

        // Render Input Text Bar label
        ImGui::Text( "%s", label.c_str() );
        ImGui::SameLine();

        // Render Input Text Bar
        input_combobox_pos = ( pw_pos + ImGui::GetCursorPos() );
        input_combobox_sz = ImVec2( input_bar_width, 0 );
        ImGui::PushItemWidth( input_bar_width );
        if( ImGui::InputTextWithHint(
                "##FileNameInput", "Type a name...", &input_fn[0], 256,
                ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll ) )
        {
            if( strlen( input_fn ) > 0 )
            {
                struct stat s;
                stat( input_fn, &s );

                // If input is a directory...
                if( S_ISDIR( s.st_mode ) )
                {
                    current_path = input_fn;
                    std::replace( current_path.begin(), current_path.end(), '\\', '/' );

                    // Browse there
                    readDIR( current_path );

                    // Reset nav tabs
                    current_dirlist.clear();
                    parsePathTabs( current_path );

                    // Clean out inputbox
                    input_fn[0] = 0;
                }
                else
                {
                    selected_fn = std::string( input_fn );
                    validate_file = true;
                }
            }
        }
        ImGui::PopItemWidth();

        // If Input Bar is edited show a list of files or dirs matching the input text.
        if( ImGui::IsItemEdited() || ImGui::IsItemActivated() )
        {
            // If input bar was focused clear selection
            selected_idx = -1;
            // If dialog_mode is OPEN/SAVE then filter from list of files..
            if( dialog_mode == DialogMode::Open || dialog_mode == DialogMode::Save )
            {
                inputcb_filter_files.clear();
                for( auto &subfile : subfiles )
                {
                    if( ImStristr( subfile.name.c_str(), nullptr, input_fn, nullptr ) != nullptr )
                    {
                        inputcb_filter_files.push_back( std::ref( subfile.name ) );
                    }
                }
            }

            // If dialog_mode == SELECT then filter from list of directories
            else if( dialog_mode == DialogMode::Select )
            {
                inputcb_filter_files.clear();
                for( auto &subdir : subdirs )
                {
                    if( ImStristr( subdir.name.c_str(), nullptr, input_fn, nullptr ) != nullptr )
                    {
                        inputcb_filter_files.push_back( std::ref( subdir.name ) );
                    }
                }
            }

            // If filtered list has any items show dropdown
            if( inputcb_filter_files.size() > 0 )
            {
                show_inputbar_combobox = true;
            }
            else
            {
                show_inputbar_combobox = false;
            }
        }

        // Render Extensions and File Types DropDown
        if( dialog_mode != DialogMode::Select )
        {
            ImGui::SameLine();
            renderExtBox();
        }

        // Render a Drop Down of files/dirs (depending on mode) that have matching characters as the
        // input text only.
        show_error |= renderInputComboBox();

        ImGui::SetCursorPos( cursor_pos );
        return show_error;
    }

    bool ImGuiFileBrowser::renderButtonsAndCheckboxRegion()
    {
        ImVec2 pw_size = ImGui::GetWindowSize();
        ImGuiStyle &style = ImGui::GetStyle();
        bool show_error = false;
        float frame_height = ImGui::GetFrameHeight();
        float frame_height_spacing = ImGui::GetFrameHeightWithSpacing();
        float button_width = ( ext_box_width - style.ItemSpacing.x ) / 2.0f;
        float buttons_xpos =
            pw_size.x - button_width * 2.0f - style.ItemSpacing.x - style.WindowPadding.x;

        ImGui::SetCursorPosY( pw_size.y - frame_height_spacing - style.WindowPadding.y );

        // Render Checkbox
        float label_width = ImGui::CalcTextSize( "Show Hidden Files and Folders" ).x +
                            ImGui::GetCursorPosX() + frame_height;
        bool show_marker = ( label_width >= buttons_xpos );
        ImGui::Checkbox( ( show_marker ) ? "##showHiddenFiles" : "Show Hidden Files and Folders",
                         &show_hidden );
        if( show_marker )
        {
            ImGui::SameLine();
            showHelpMarker( "Show Hidden Files and Folders" );
        }

        // Render an Open Button (in OPEN/SELECT dialog_mode) or Open/Save depending on what's
        // selected in SAVE dialog_mode
        ImGui::SameLine();
        ImGui::SetCursorPosX( buttons_xpos );
        if( dialog_mode == DialogMode::Save )
        {
            // If directory selected and Input Text Bar doesn't have focus, render Open Button
            if( selected_idx != -1 && is_dir &&
                ImGui::GetFocusID() != ImGui::GetID( "##FileNameInput" ) )
            {
                if( ImGui::Button( "Open", ImVec2( button_width, 0 ) ) )
                {
                    show_error |= !( onDirClick( selected_idx ) );
                }
            }
            else if( ImGui::Button( "Save", ImVec2( button_width, 0 ) ) && strlen( input_fn ) > 0 )
            {
                selected_fn = std::string( input_fn );
                validate_file = true;
            }
        }
        else
        {
            if( ImGui::Button( "Open", ImVec2( button_width, 0 ) ) )
            {
                // It's possible for both to be true at once (user selected directory but input bar
                // has some text. In this case we chose to open the directory instead of opening the
                // file. Also note that we don't need to access the selected file through
                // "selected_idx" since the if a file is selected, input bar will get populated with
                // that name.
                if( selected_idx >= 0 && is_dir )
                {
                    show_error |= !( onDirClick( selected_idx ) );
                }
                else if( strlen( input_fn ) > 0 )
                {
                    selected_fn = std::string( input_fn );
                    validate_file = true;
                }
            }

            // Render Select Button if in SELECT Mode
            if( dialog_mode == DialogMode::Select )
            {
                // Render Select Button
                ImGui::SameLine();
                if( ImGui::Button( "Select", ImVec2( button_width, 0 ) ) )
                {
                    if( strlen( input_fn ) > 0 )
                    {
                        selected_fn = std::string( input_fn );
                        validate_file = true;
                    }
                }
            }
        }

        // Render Cancel Button
        ImGui::SameLine();
        if( ImGui::Button( "Cancel", ImVec2( button_width, 0 ) ) )
        {
            closeDialog();
        }

        return show_error;
    }

    bool ImGuiFileBrowser::renderInputComboBox()
    {
        bool show_error = false;
        ImGuiStyle &style = ImGui::GetStyle();
        ImGuiID input_id = ImGui::GetID( "##FileNameInput" );
        ImGuiID focus_scope_id = ImGui::GetID( "##InputBarComboBoxListScope" );
        float frame_height = ImGui::GetFrameHeight();

        input_combobox_sz.y = std::min<float>(
            ( inputcb_filter_files.size() + 1 ) * frame_height + style.WindowPadding.y * 2.0f,
            8 * ImGui::GetFrameHeight() + style.WindowPadding.y * 2.0f );

        if( show_inputbar_combobox && ( ImGui::GetFocusedFocusScope() == focus_scope_id ||
                                        ImGui::GetCurrentContext()->ActiveIdIsAlive == input_id ) )
        {
            ImGuiWindowFlags popupFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                          ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoFocusOnAppearing |
                                          ImGuiWindowFlags_NoScrollbar |
                                          ImGuiWindowFlags_NoSavedSettings;

            ImGui::PushStyleColor( ImGuiCol_ChildBg, ImVec4( 0.1f, 0.1f, 0.1f, 1.0f ) );
            ImGui::PushStyleColor( ImGuiCol_FrameBg, ImVec4( 0.125f, 0.125f, 0.125f, 1.0f ) );
            ImGui::SetNextWindowBgAlpha( 1.0 );
            ImGui::SetNextWindowPos( input_combobox_pos +
                                     ImVec2( 0, ImGui::GetFrameHeightWithSpacing() ) );
            ImGui::PushClipRect( ImVec2( 0, 0 ), ImGui::GetIO().DisplaySize, false );

            ImGui::BeginChild( "##InputBarComboBox", input_combobox_sz, true, popupFlags );

            ImVec2 listbox_size = input_combobox_sz - ImGui::GetStyle().WindowPadding * 2.0f;
            if( ImGui::BeginListBox( "##InputBarComboBoxList", listbox_size ) )
            {
                ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 1.0f, 1.0f, 1.0f, 1.0f ) );
                ImGui::PushFocusScope( focus_scope_id );
                for( auto &element : inputcb_filter_files )
                {
                    if( ImGui::Selectable( element.get().c_str(), false,
                                           ImGuiSelectableFlags_NoHoldingActiveID |
                                               ImGuiSelectableFlags_SelectOnClick ) )
                    {
                        if( element.get().size() > 256 )
                        {
                            error_title = "Error!";
                            error_msg = "Selected File Name is longer than 256 characters.";
                            show_error = true;
                        }
                        else
                        {
                            strcpy( input_fn, element.get().c_str() );
                            show_inputbar_combobox = false;
                        }
                    }
                }
                ImGui::PopFocusScope();
                ImGui::PopStyleColor( 1 );
                ImGui::EndListBox();
            }
            ImGui::EndChild();
            ImGui::PopStyleColor( 2 );
            ImGui::PopClipRect();
        }
        return show_error;
    }

    void ImGuiFileBrowser::renderExtBox()
    {
        if( !valid_exts.empty() )
        {
            const char *selected_label = valid_exts[selected_ext_idx].c_str();
            ImGui::PushItemWidth( ext_box_width );
            if( ImGui::BeginCombo( "##FileTypes", selected_label ) )
            {
                for( Array<std::string>::size_type i = 0; i < valid_exts.size(); i++ )
                {
                    std::string label_text = valid_exts[i];
                    if( label_text == "*.*" )
                    {
                        label_text = "All Files (*.*)";
                    }

                    if( ImGui::Selectable( label_text.c_str(),
                                           selected_ext_idx == static_cast<int>( i ) ) )
                    {
                        show_all_valid_files = ( label_text == "All valid files" );
                        selected_ext_idx = static_cast<int>( i );
                        // Automatically append extension to input filename when changing extensions
                        // from dropdown
                        if( dialog_mode == DialogMode::Save )
                        {
                            std::string name( input_fn );
                            size_t idx = name.find_last_of( "." );
                            if( idx == std::string::npos )
                            {
                                idx = strlen( input_fn );
                            }
                            for( char j : valid_exts[selected_ext_idx] )
                            {
                                input_fn[idx++] = j;
                            }
                            input_fn[idx++] = '\0';
                        }
                        filterFiles( FilterMode_Files );
                    }
                }

                ImGui::EndCombo();
            }
            extension = valid_exts[selected_ext_idx];
            ImGui::PopItemWidth();
        }
    }

    bool ImGuiFileBrowser::onNavigationButtonClick( int idx )
    {
        std::string new_path = "";

        // First Button corresponds to virtual folder Computer which lists all logical drives (hard
        // disks and removables) and "/" on Unix
        if( idx == 0 )
        {
#ifdef OSWIN
            if( !loadWindowsDrives() )
            {
                return false;
            }
            current_path.clear();
            current_dirlist.clear();
            current_dirlist.emplace_back( "Computer" );
            return true;
#else
            new_path = "/";
#endif  // OSWIN
        }
#ifdef OSWIN
        // Clicked on a drive letter?
        if( idx == 1 )
        {
            new_path = current_path.substr( 0, 3 );
        }
        else
        {
            // Start from i=1 since at 0 lies "MyComputer" which is only virtual and shouldn't be
            // read by readDIR
            for( int i = 1; i <= idx; i++ )
            {
                new_path += current_dirlist[i] + "/";
            }
        }
#else
        // Since UNIX absolute paths start at "/", we handle this separately to avoid adding a
        // double slash at the beginning
        new_path += current_dirlist[0];
        for( int i = 1; i <= idx; i++ )
            new_path += current_dirlist[i] + "/";
#endif

        if( readDIR( new_path ) )
        {
            current_dirlist.erase( current_dirlist.begin() + idx + 1, current_dirlist.end() );
            current_path = new_path;
            return true;
        }
        return false;
    }

    bool ImGuiFileBrowser::onDirClick( int idx )
    {
        std::string name;
        std::string new_path( current_path );
        bool drives_shown = false;

#ifdef OSWIN
        drives_shown = ( current_dirlist.size() == 1 && current_dirlist.back() == "Computer" );
#endif  // OSWIN

        name = filtered_dirs[idx]->name;

        if( name == ".." )
        {
            new_path.pop_back();  // Remove trailing '/'
            new_path =
                new_path.substr( 0, new_path.find_last_of( '/' ) + 1 );  // Also include a trailing '/'
        }
        else
        {
            // Remember we displayed drives on Windows as *Local/Removable Disk: X* hence we need
            // last char only
            if( drives_shown )
            {
                name = std::string( 1, name.back() ) + ":";
            }
            new_path += name + "/";
        }

        if( readDIR( new_path ) )
        {
            if( name == ".." )
            {
                current_dirlist.pop_back();
            }
            else
            {
                current_dirlist.push_back( name );
            }

            current_path = new_path;
            return true;
        }
        return false;
    }

    bool ImGuiFileBrowser::readDIR( std::string pathdir )
    {
        DIR *dir;
        struct dirent *ent;

        /* If the current directory doesn't exist, and we are opening the dialog for the first time,
         * reset to defaults to avoid looping of showing error modal. An example case is when user
         * closes the dialog in a folder. Then deletes the folder outside. On reopening the dialog
         * the current path (previous) would be invalid.
         */
        dir = opendir( pathdir.c_str() );
        if( dir == nullptr && is_appearing )
        {
            current_dirlist.clear();
#ifdef OSWIN
            current_path = pathdir = "./";
#else
            initCurrentPath();
            pathdir = current_path;
#endif  // OSWIN

            dir = opendir( pathdir.c_str() );
        }

        if( dir != nullptr )
        {
#ifdef OSWIN
            // If we are on Windows and current path is relative then get absolute path from dirent
            // structure
            if( current_dirlist.empty() && pathdir == "./" )
            {
                const wchar_t *absolute_path = dir->wdirp->patt;
                std::string current_directory = wStringToString( absolute_path );
                std::replace( current_directory.begin(), current_directory.end(), '\\', '/' );

                // Remove trailing "*" returned by ** dir->wdirp->patt **
                current_directory.pop_back();
                current_path = current_directory;

                // Create a vector of each directory in the file path for the filepath bar. Not
                // Necessary for linux as starting directory is "/"
                parsePathTabs( current_path );
            }
#endif  // OSWIN

            // store all the files and directories within directory and clear previous entries
            clearFileList();
            while( ( ent = readdir( dir ) ) != nullptr )
            {
                bool is_hidden = false;
                std::string name( ent->d_name );

                // Ignore current directory
                if( name == "." )
                {
                    continue;
                }

                // Somehow there is a '..' present in root directory in linux.
#ifndef OSWIN
                if( name == ".." && pathdir == "/" )
                    continue;
#endif  // OSWIN

                if( name != ".." )
                {
#ifdef OSWIN
                    std::string dir = pathdir + std::string( ent->d_name );
                    // IF system file skip it...
                    if( FILE_ATTRIBUTE_SYSTEM & GetFileAttributesA( dir.c_str() ) )
                    {
                        continue;
                    }
                    if( FILE_ATTRIBUTE_HIDDEN & GetFileAttributesA( dir.c_str() ) )
                    {
                        is_hidden = true;
                    }
#else
                    if( name[0] == '.' )
                        is_hidden = true;
#endif  // OSWIN
                }
                // Store directories and files in separate vectors
                if( ent->d_type == DT_DIR )
                {
                    subdirs.emplace_back( name, is_hidden );
                }
                else if( ent->d_type == DT_REG && dialog_mode != DialogMode::Select )
                {
                    subfiles.emplace_back( name, is_hidden );
                }
            }
            closedir( dir );
            std::sort( subdirs.begin(), subdirs.end(), alphaSortComparator );
            std::sort( subfiles.begin(), subfiles.end(), alphaSortComparator );

            // Initialize Filtered dirs and files
            filterFiles( filter_mode );
        }
        else
        {
            error_title = "Error!";
            error_msg =
                "Error opening directory! Make sure the directory exists and you have the proper "
                "rights to access the directory.";
            return false;
        }
        return true;
    }

    void ImGuiFileBrowser::filterFiles( int filter_mode )
    {
        filter_dirty = false;
        if( filter_mode | FilterMode_Dirs )
        {
            filtered_dirs.clear();
            for( auto &subdir : subdirs )
            {
                if( filter.PassFilter( subdir.name.c_str() ) )
                {
                    filtered_dirs.push_back( &subdir );
                }
            }
        }
        if( filter_mode | FilterMode_Files )
        {
            filtered_files.clear();
            for( auto &subfile : subfiles )
            {
                // If the option to show all supported formats is selected, filter all files
                // supported
                if( show_all_valid_files )
                {
                    if( filter.PassFilter( subfile.name.c_str() ) )
                    {
                        std::string ext = subfile.name.find_last_of( '.' ) == std::string::npos
                                              ? ""
                                              : subfile.name.substr( subfile.name.find_last_of( '.' ) );
                        std::transform( ext.begin(), ext.end(), ext.begin(),
                                        []( unsigned char c ) { return std::tolower( c ); } );
                        if( ext.length() > 0 &&
                            find( valid_exts.begin(), valid_exts.end(), ext ) != valid_exts.end() )
                        {
                            filtered_files.push_back( &subfile );
                        }
                    }
                }
                // If the option to show all files is selected, filter all files
                else if( !valid_exts.empty() && valid_exts[selected_ext_idx] == "*.*" )
                {
                    if( filter.PassFilter( subfile.name.c_str() ) )
                    {
                        filtered_files.push_back( &subfile );
                    }
                }
                // If any other extension is selected, filter files having only that extension
                else
                {
                    if( !valid_exts.empty() )
                    {
                        if( filter.PassFilter( subfile.name.c_str() ) &&
                            ( ImStristr( subfile.name.c_str(), nullptr,
                                         valid_exts[selected_ext_idx].c_str(), nullptr ) ) != nullptr )
                        {
                            filtered_files.push_back( &subfile );
                        }
                    }
                }
            }
        }
    }

    void ImGuiFileBrowser::showHelpMarker( std::string desc )
    {
        ImGui::TextDisabled( "(?)" );
        if( ImGui::IsItemHovered() )
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos( ImGui::GetFontSize() * 35.0f );
            ImGui::TextUnformatted( desc.c_str() );
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    void ImGuiFileBrowser::showErrorModal()
    {
        ImVec2 window_size( 260, 0 );
        ImGui::SetNextWindowSize( window_size );

        if( ImGui::BeginPopupModal( error_title.c_str(), nullptr,
                                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize ) )
        {
            ImGui::TextWrapped( "%s", error_msg.c_str() );

            ImGui::Separator();
            ImGui::SetCursorPosX( window_size.x / 2.0f - getButtonSize( "OK" ).x / 2.0f );
            if( ImGui::Button( "OK", getButtonSize( "OK" ) ) )
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    bool ImGuiFileBrowser::showReplaceFileModal()
    {
        ImVec2 window_size( 250, 0 );
        ImGui::SetNextWindowSize( window_size );
        bool ret_val = false;
        if( ImGui::BeginPopupModal( repfile_modal_id.c_str(), nullptr,
                                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize ) )
        {
            std::string text =
                "A file with the following filename already exists. Are you sure you want to "
                "replace the existing file?";
            ImGui::TextWrapped( "%s", text.c_str() );

            ImGui::Separator();

            float buttons_width =
                getButtonSize( "Yes" ).x + getButtonSize( "No" ).x + ImGui::GetStyle().ItemSpacing.x;
            ImGui::SetCursorPosX( ImGui::GetCursorPosX() + ImGui::GetWindowWidth() / 2.0f -
                                  buttons_width / 2.0f - ImGui::GetStyle().WindowPadding.x );

            if( ImGui::Button( "Yes", getButtonSize( "Yes" ) ) )
            {
                selected_path = current_path + selected_fn;
                ImGui::CloseCurrentPopup();
                ret_val = true;
            }

            ImGui::SameLine();
            if( ImGui::Button( "No", getButtonSize( "No" ) ) )
            {
                selected_fn.clear();
                selected_path.clear();
                ImGui::CloseCurrentPopup();
                ret_val = false;
            }
            ImGui::EndPopup();
        }
        return ret_val;
    }

    void ImGuiFileBrowser::showInvalidFileModal()
    {
        ImVec2 window_size( 350, 0 );
        ImGui::SetNextWindowSize( window_size );

        if( ImGui::BeginPopupModal( invfile_modal_id.c_str(), nullptr,
                                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize ) )
        {
            std::string text = "";
            if( valid_exts.back() == "*.*" )
            {
                text =
                    "Selected file doesn't exist. Make sure the file you are trying to open exists "
                    "and the name matches including the extension.";
            }
            else
            {
                text =
                    "Selected file either doesn't exist or is not supported. Please select a file "
                    "with the following extensions...";
            }

            ImVec2 button_size = getButtonSize( "OK" );

            float frame_height = ImGui::GetFrameHeightWithSpacing();
            float cw_content_height = ( valid_exts.size() - 1 ) * frame_height;
            float cw_height = std::min<float>( 4.0f * frame_height, cw_content_height );

            ImGui::TextWrapped( "%s", text.c_str() );
            if( valid_exts.back() != "*.*" )
            {
                ImGui::BeginChild( "##SupportedExts", ImVec2( 0, cw_height ), true );
                for( Array<std::string>::size_type i = 0; i < valid_exts.size() - 1; i++ )
                {
                    ImGui::BulletText( "%s", valid_exts[i].c_str() );
                }
                ImGui::EndChild();
            }

            ImGui::SetCursorPosX( window_size.x / 2.0f - button_size.x / 2.0f );
            if( ImGui::Button( "OK", button_size ) )
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    void ImGuiFileBrowser::setValidExtTypes( const std::string &valid_types_string )
    {
        /* Initialize a list of files extensions that are valid.
         * If the user chooses a file that doesn't match the extensions in the
         * list, we will show an error modal...
         */
        bool all_files = false;
        valid_exts.clear();

        if( valid_types_string == "" )
        {
            return;
        }

        std::string valid_str_lower( valid_types_string );
        std::transform( valid_str_lower.begin(), valid_str_lower.end(), valid_str_lower.begin(),
                        []( unsigned char c ) { return std::tolower( c ); } );

        std::string extension = "";
        std::istringstream iss( valid_str_lower );
        while( std::getline( iss, extension, ',' ) )
        {
            if( !extension.empty() && extension != "*.*" )
            {
                valid_exts.push_back( extension );
            }
            else if( extension == "*.*" )
            {
                all_files = true;
            }
        }

        // Add an option to support all valid extensions
        if( valid_exts.size() > 1 && dialog_mode == DialogMode::Open )
        {
            valid_exts.emplace_back( "All valid files" );
        }

        // Add all files option in last
        if( all_files )
        {
            valid_exts.emplace_back( "*.*" );
        }
    }

    bool ImGuiFileBrowser::validateFile()
    {
        bool match = false;

        // If there is an item selected, check if the selected file name (the input filename, in
        // other words) matches the selection.
        if( selected_idx >= 0 )
        {
            if( dialog_mode == DialogMode::Select )
            {
                match = ( filtered_dirs[selected_idx]->name == selected_fn );
            }
            else
            {
                match = ( filtered_files[selected_idx]->name == selected_fn );
            }
        }

        // If the input filename doesn't match we need to explicitly find the input filename..
        if( !match )
        {
            if( dialog_mode == DialogMode::Select )
            {
                for( auto &subdir : subdirs )
                {
                    if( subdir.name == selected_fn )
                    {
                        match = true;
                        break;
                    }
                }
            }
            else
            {
                for( auto &subfile : subfiles )
                {
                    if( subfile.name == selected_fn )
                    {
                        match = true;
                        break;
                    }
                }
            }
        }

        // If file doesn't match, return true on SAVE mode (since file doesn't exist, hence can be
        // saved directly) and return false on other modes (since file doesn't exist so cant
        // open/select)
        if( !match )
        {
            return ( dialog_mode == DialogMode::Save );
        }

        // If file matches, return false on SAVE, we need to show a replace file modal
        if( dialog_mode == DialogMode::Save )
        {
            return false;
        }

        // Return true on SELECT, no need to validate extensions
        if( dialog_mode == DialogMode::Select )
        {
            return true;
        }
        // If list of extensions has all types, no need to validate.
        for( auto ext : valid_exts )
        {
            if( ext == "*.*" )
            {
                return true;
            }
        }
        size_t idx = selected_fn.find_last_of( '.' );
        std::string file_ext =
            idx == std::string::npos ? "" : selected_fn.substr( idx, selected_fn.length() - idx );

        std::transform( file_ext.begin(), file_ext.end(), file_ext.begin(),
                        []( unsigned char c ) { return std::tolower( c ); } );

        return ( std::find( valid_exts.begin(), valid_exts.end(), file_ext ) != valid_exts.end() );
    }

    ImVec2 ImGuiFileBrowser::getButtonSize( std::string button_text )
    {
        return ( ImGui::CalcTextSize( button_text.c_str() ) + ImGui::GetStyle().FramePadding * 2.0 );
    }

    void ImGuiFileBrowser::parsePathTabs( std::string path )
    {
        std::string path_element = "";
        std::string root = "";

#ifdef OSWIN
        current_dirlist.emplace_back( "Computer" );
#else
        if( path[0] == '/' )
            current_dirlist.push_back( "/" );
#endif  // OSWIN

        std::istringstream iss( path );
        while( std::getline( iss, path_element, '/' ) )
        {
            if( !path_element.empty() )
            {
                current_dirlist.push_back( path_element );
            }
        }
    }

    std::string ImGuiFileBrowser::wStringToString( const wchar_t *wchar_arr )
    {
        auto state = std::mbstate_t();

        // MinGW bug (patched in mingw-w64), wcsrtombs doesn't ignore length parameter when dest =
        // nullptr. Hence the large number.
        size_t len = 1 + std::wcsrtombs( nullptr, &( wchar_arr ), 600000, &state );

        auto char_arr = new char[len];
        std::wcsrtombs( char_arr, &wchar_arr, len, &state );

        std::string ret_val( char_arr );

        delete[] char_arr;
        return ret_val;
    }

    bool ImGuiFileBrowser::alphaSortComparator( const Info &a, const Info &b )
    {
        const char *str1 = a.name.c_str();
        const char *str2 = b.name.c_str();
        int ca, cb;
        do
        {
            ca = static_cast<unsigned char>( *str1++ );
            cb = static_cast<unsigned char>( *str2++ );
            ca = std::tolower( std::toupper( ca ) );
            cb = std::tolower( std::toupper( cb ) );
        } while( ca == cb && ca != '\0' );
        if( ca < cb )
        {
            return true;
        }
        return false;
    }

    // Windows Exclusive function
#ifdef OSWIN
    bool ImGuiFileBrowser::loadWindowsDrives()
    {
        DWORD len = GetLogicalDriveStringsA( 0, nullptr );
        auto drives = new char[len];
        if( !GetLogicalDriveStringsA( len, drives ) )
        {
            delete[] drives;
            return false;
        }

        clearFileList();
        char *temp = drives;
        for( char *drv = nullptr; *temp != '\0'; temp++ )
        {
            drv = temp;
            if( DRIVE_REMOVABLE == GetDriveTypeA( drv ) )
            {
                subdirs.emplace_back( "Removable Disk: " + std::string( 1, drv[0] ), false );
            }
            else if( DRIVE_FIXED == GetDriveTypeA( drv ) )
            {
                subdirs.emplace_back( "Local Disk: " + std::string( 1, drv[0] ), false );
            }
            // Go to nullptr character
            while( *( ++temp ) )
                ;
        }
        delete[] drives;
        return true;
    }
#endif

    // Unix only
#ifndef OSWIN
    void ImGuiFileBrowser::initCurrentPath()
    {
        bool path_max_def = false;

#    ifdef PATH_MAX
        path_max_def = true;
#    endif  // PATH_MAX

        char *buffer = nullptr;

        // If PATH_MAX is defined deal with memory using new/delete. Else fallback to malloc'ed
        // memory from `realpath()`
        if( path_max_def )
            buffer = new char[PATH_MAX];

        char *real_path = realpath( "./", buffer );
        if( real_path == nullptr )
        {
            current_path = "/";
            current_dirlist.push_back( "/" );
        }
        else
        {
            current_path = std::string( real_path );
            current_path += "/";
            parsePathTabs( current_path );
        }

        if( path_max_def )
            delete[] buffer;
        else
            free( real_path );
    }
#endif  // OSWIN

    ImGuiFileBrowser::Info::Info( std::string name, bool is_hidden ) :
        name( std::move( name ) ),
        is_hidden( is_hidden )
    {
    }
}  // namespace workphone::ui
