/*
    Swish main

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

#include "swish.h"
//#include "gui.h"
#include "Mode.h"

#include <string>
using std::string;

SSH_OPTIONS *options;
Server *server;
std::ostringstream traceStream;


void setupOptions()
{
	int ret; // Stores return values
	
	// Create ssh options contruct
	options = options_new();
	assert(options != NULL);
	
	// Request bidirectional compression
	//if(options_set_wanted_method(options, KEX_COMP_C_S, "zlib,none"))
	//	cerr << "Error: " << ssh_get_error(NULL) << endl;
	//if(options_set_wanted_method(options, KEX_COMP_S_C, "zlib,none"))
	//	cerr << "Error: " << ssh_get_error(NULL) << endl;
		
	// Connection info
	options_set_host(options, "localhost");
	options_set_username(options, "swish");
	
	assert(options != NULL);
}

void testServerAuthentication()
{
	switch(server->getKnownStatus())
	{
	case SSH_SERVER_KNOWN_OK:
		DPRINTLN("The host is known and the key has not changed");
		break;
	case SSH_SERVER_KNOWN_CHANGED:
		DPRINTLN("The host’s key has changed. Either you are under \
			an active attack or the key changed. The API doesn’t \
			give any way to modify the key in known hosts yet. \
			I Urge end developers to WARN the user about the \
			possibility of an attack.");
		break;
	case SSH_SERVER_FOUND_OTHER:
		DPRINTLN("The host gave us a public key of one type, which \
			does not exist yet in our known host file, but there \
			is an other type of key which is known.\
			ie. server sent a DSA key and we had a RSA key. \
			Be carreful it’s a possible attack (coder should use \
			option_set_wanted_method() to specify which key to use).");
		break;
	case SSH_SERVER_NOT_KNOWN: 
		DPRINTLN("The server is unknown in known hosts. Possible \
			reasons : case not matching, alias, ... In any case \
			the user MUST confirm the Md5 hash is correct.");
		break;
	case SSH_SERVER_ERROR: 
		DPRINTLN("Some error happened while opening known host file.");
		break;
	default:
		DPRINTLN("Unknown return from ssh_is_server_known(): " << server->getKnownStatus());
		break;
	}
	
	// Get hash and print
	cout << "Host key: " << server->getHash() << endl;
}
/*
void printDir(MyFrame *frame)
{
	SFTP_SESSION *sftpSession = sftp_new(server->getSession());
	if(sftpSession == NULL)
		cerr << "An error occurred while creating the sftp session" << endl;
		
	assert(sftpSession);
	
	int ret = sftp_init(sftpSession);
	if(ret != 0)
		cerr << "An error occurred while initialising the sftp session" << endl;
	
	SFTP_DIR *dir = sftp_opendir(sftpSession, "/tmp");
	if(dir == NULL)
	{
		cerr << "An error occurred while opening the directory" << endl;
		cerr << "Error: " << ssh_get_error(NULL) << endl;
	}
	
	assert(dir);
	
	SFTP_ATTRIBUTES *dirAttr = sftp_readdir(sftpSession, dir);
	while (dirAttr != NULL) {
		assert(dirAttr);
		long index = frame->appendItem(dirAttr->name);
		frame->setItem(index, SFTP_LONGNAME_COL_IDX, dirAttr->longname);
		cout << dirAttr->longname << endl;
		frame->setItem(index, SFTP_OWNER_COL_IDX, dirAttr->owner);
		frame->setItem(index, SFTP_GROUP_COL_IDX, dirAttr->group);
		
		frame->setItem(index, SFTP_SIZE_COL_IDX, dirAttr->size);
		
		Mode tempMode = Mode(dirAttr->permissions);
		if(tempMode.isDirectory())
			frame->setItemBackgroundColour(index, "GOLD");
		wxString tempPerm;
		tempPerm.sprintf(tempMode.toString().c_str());
		frame->setItem(index, SFTP_PERMISSIONS_COL_IDX, tempPerm);
		frame->setItem(index, SFTP_TYPE_COL_IDX, dirAttr->type);
		
		if(dirAttr->acl != NULL)
			frame->setItem(index, SFTP_ACL_COL_IDX, 
				string_to_char(dirAttr->acl));
		if(dirAttr->extended_type != NULL)
			frame->setItem(index, SFTP_EXTENDED_TYPE_COL_IDX, 
				string_to_char(dirAttr->extended_type));
		if(dirAttr->extended_data != NULL)
			frame->setItem(index, SFTP_EXTENDED_DATA_COL_IDX, 
				string_to_char(dirAttr->extended_data));
			
		sftp_attributes_free(dirAttr);
		dirAttr = sftp_readdir(sftpSession, dir);
	}
	if(!sftp_dir_eof(dir))
		cerr << "An error occurred while listing the directory" << endl;
	
	ret = sftp_dir_close(dir);
	if(ret != 0)
		cerr << "An error occurred while closing the directory" << endl;
	
}
*/

void printDir()
{
	SFTP_SESSION *sftpSession = sftp_new(server->getSession());
	if(sftpSession == NULL)
		cerr << "An error occurred while creating the sftp session" << endl;
		
	assert(sftpSession);
	
	int ret = sftp_init(sftpSession);
	if(ret != 0)
		cerr << "An error occurred while initialising the sftp session" << endl;
	
	SFTP_DIR *dir = sftp_opendir(sftpSession, "/tmp");
	if(dir == NULL)
	{
		cerr << "An error occurred while opening the directory" << endl;
		cerr << "Error: " << ssh_get_error(NULL) << endl;
	}
	
	assert(dir);
	
	SFTP_ATTRIBUTES *dirAttr = sftp_readdir(sftpSession, dir);
	while (dirAttr != NULL) {
		assert(dirAttr);
		//long index = frame->appendItem(dirAttr->name);

		//frame->setItem(index, SFTP_LONGNAME_COL_IDX, dirAttr->longname);
		cout << dirAttr->longname << endl;
		//frame->setItem(index, SFTP_OWNER_COL_IDX, dirAttr->owner);
		cout << dirAttr->owner << endl;
		//frame->setItem(index, SFTP_GROUP_COL_IDX, dirAttr->group);
		cout << dirAttr->group << endl;
		//frame->setItem(index, SFTP_SIZE_COL_IDX, dirAttr->size);
		cout << dirAttr->size << endl;
		
		Mode tempMode = Mode(dirAttr->permissions);
		//if(tempMode.isDirectory())
		//	frame->setItemBackgroundColour(index, "GOLD");
		//wxString tempPerm;
		//tempPerm.sprintf(tempMode.toString().c_str());
		//frame->setItem(index, SFTP_PERMISSIONS_COL_IDX, tempPerm);
		//frame->setItem(index, SFTP_TYPE_COL_IDX, dirAttr->type);
		cout << tempMode.toString().c_str() << endl;
		
		//if(dirAttr->acl != NULL)
		//	frame->setItem(index, SFTP_ACL_COL_IDX, 
		//		string_to_char(dirAttr->acl));
		//if(dirAttr->extended_type != NULL)
		//	frame->setItem(index, SFTP_EXTENDED_TYPE_COL_IDX, 
		//		string_to_char(dirAttr->extended_type));
		//if(dirAttr->extended_data != NULL)
		//	frame->setItem(index, SFTP_EXTENDED_DATA_COL_IDX, 
		//		string_to_char(dirAttr->extended_data));
			
		sftp_attributes_free(dirAttr);
		dirAttr = sftp_readdir(sftpSession, dir);
	}
	if(!sftp_dir_eof(dir))
		cerr << "An error occurred while listing the directory" << endl;
	
	ret = sftp_dir_close(dir);
	if(ret != 0)
		cerr << "An error occurred while closing the directory" << endl;
	
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
//int main(int argc, char** argv)
{
//	frame = new MyFrame( "Hello World", wxPoint(50,50), wxSize(450,340) );
//	frame->Show(TRUE);
//	SetTopWindow(frame);

	// Make a console, for debugging with stdout
#ifdef _DEBUG
	AllocConsole();
	freopen("CONOUT$","w",stdout);
	freopen("CONERR$","w",stderr);
#endif

	DPRINTLN("Starting main() ...");

	setupOptions();
	
	DPRINTLN("Creating new Server ...");
	server = new Server(options);
	DPRINTLN("Created new Server");
	
	testServerAuthentication();
	server->authenticateKeyboardInteractive();
	
	//printDir(frame);
	printDir();

	DPRINTLN("Finishing main()");

	return TRUE;
}

/* $Id$ */
