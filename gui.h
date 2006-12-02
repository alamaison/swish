/*
    Construct wxWidgets file-info window (header)

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

#ifndef GUI_H_
#define GUI_H_

#include <wx/wx.h>
#include <wx/listctrl.h>

enum
{
	ID_Quit = 1,
	ID_About,
};

class MyFrame: public wxFrame
{
	long listPos;
	wxListCtrl *list;

public:

	MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size);

	long appendItem(const wxString& text)
	{
		return list->InsertItem(listPos++, text);
	}
	long appendItem(char *text)
	{
		return list->InsertItem(listPos++, text);
	}
	template <class T> long appendItem(T uInt)
	{
		wxString uString;
		uString.sprintf("%u", uInt);
		return list->InsertItem(listPos++, uString);
	}

	long setItem(long index, int col, const wxString& label)
	{
		return list->SetItem(index, col, label);
	}
	long setItem(long index, int col, char *label)
	{
		return list->SetItem(index, col, label);
	}
	void setItemBackgroundColour(long index, char *colour)
	{
		list->SetItemBackgroundColour(index, colour);
	}
	void setItemBackgroundColour(long index, wxString& colour)
	{
		list->SetItemBackgroundColour(index, colour);
	}
	template <class T> long setItem(long index, int col, T uInt)
	{
		wxString uString;
		uString.sprintf("%u", uInt);
		return list->SetItem(index, col, uString);
	}

	void OnQuit(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);

	DECLARE_EVENT_TABLE()
};

class MyApp : public wxApp
{
	MyFrame *frame;
	virtual bool OnInit();
};

#define SFTP_NAME_COL_IDX 0
#define SFTP_LONGNAME_COL_IDX 9
#define SFTP_FLAGS_COL_IDX 99
#define SFTP_TYPE_COL_IDX 4
#define SFTP_SIZE_COL_IDX 5
#define SFTP_UID_COL_IDX 99
#define SFTP_GID_COL_IDX 99
#define SFTP_OWNER_COL_IDX 1
#define SFTP_GROUP_COL_IDX 2
#define SFTP_PERMISSIONS_COL_IDX 3
#define SFTP_ATIME64_COL_IDX 99
#define SFTP_ATIME_COL_IDX 99
#define SFTP_ATIME_NSECONDS_COL_IDX 99
#define SFTP_CREATETIME_COL_IDX 99
#define SFTP_CREATETIME_NSECONDS_COL_IDX 99
#define SFTP_MTIME64_COL_IDX 99
#define SFTP_MTIME_COL_IDX 99
#define SFTP_MTIME_NSECONDS_COL_IDX 99
#define SFTP_ACL_COL_IDX 6
#define SFTP_EXTENDED_COUNT_COL_IDX 99
#define SFTP_EXTENDED_TYPE_COL_IDX 7
#define SFTP_EXTENDED_DATA_COL_IDX 8

#endif /*GUI_H_*/

/* $Id$ */
