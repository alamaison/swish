/*
    Construct wxWidgets file-info window

    Copyright (C) 2006  Alexander Lamaison <awl03 (at) doc.ic.ac.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "gui.h"

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
	EVT_MENU(ID_Quit, MyFrame::OnQuit)
	EVT_MENU(ID_About, MyFrame::OnAbout)
END_EVENT_TABLE()

IMPLEMENT_APP(MyApp)

MyFrame::MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
: wxFrame((wxFrame *)NULL, -1, title, pos, size)
{
	wxMenu *menuFile = new wxMenu;

	menuFile->Append( ID_About, "&About..." );
	menuFile->AppendSeparator();
	menuFile->Append( ID_Quit, "E&xit" );

	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append( menuFile, "&File" );

	SetMenuBar( menuBar );

	CreateStatusBar();
	SetStatusText( "Swish fetched this file listing using SFTP through libssh!" );

	list = new wxListCtrl(
		this, -1, wxPoint( 0, 0 ), wxSize( 400, 300 ),
		wxLC_REPORT | wxLC_HRULES);

	list->InsertColumn(SFTP_NAME_COL_IDX,"Name");
	list->InsertColumn(SFTP_LONGNAME_COL_IDX,"Long Name");
	list->InsertColumn(SFTP_OWNER_COL_IDX,"Owner");
	list->InsertColumn(SFTP_GROUP_COL_IDX,"Group");
	list->InsertColumn(SFTP_ACL_COL_IDX,"ACL");
	list->InsertColumn(SFTP_EXTENDED_TYPE_COL_IDX,"Extended Type");
	list->InsertColumn(SFTP_EXTENDED_DATA_COL_IDX,"Extended Data");
	list->InsertColumn(SFTP_SIZE_COL_IDX,"Size");
	list->InsertColumn(SFTP_PERMISSIONS_COL_IDX,"Permissions");
	list->InsertColumn(SFTP_TYPE_COL_IDX,"Type");

}

void MyFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	Close(TRUE);
}

void MyFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
	wxMessageBox("Swish proof-of-concept",
	"About Swish", wxOK | wxICON_INFORMATION, this);
}

/* $Id$ */
