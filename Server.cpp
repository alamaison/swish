/*
    Server connection and associated operations

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

#include <sstream>
using std::string;
using std::stringstream;

#include "swish.h"

#include <windows.h>

#include <string.h>

Server::Server(SSH_OPTIONS *options)
{
	// Connect to server
	DPRINTLN("Connecting to Server ...");
	session = ssh_connect(options);
	if(session == NULL)
	{
		DPRINTLN("Error: " << ssh_get_error(NULL) << endl);
//		cerr << "Error: " << ssh_get_error(NULL) << endl;
	}
	DPRINTLN("Connected to server");
	
	// Get hash and check known_hosts
	ssh_get_pubkey_hash(session, hash);
	known = ssh_is_server_known(session);
}

void Server::setHash(char* hash)
{
	strncpy(this->hash, hash, MD5_DIGEST_LEN);
}


int Server::authenticatePassword()
{
	char pword[255];
	cout << "Please enter your password:" << endl;
	cin >> pword;
	int ret = ssh_userauth_password(session, "swish", pword);
	
	switch(ret)
	{
	case SSH_AUTH_ERROR:
		DPRINTLN("Some serious error happened during authentication");
		return -1;
	case SSH_AUTH_DENIED:
		DPRINTLN("Authentication failed");
		return -1;
	case SSH_AUTH_SUCCESS:
		DPRINTLN("You are now authenticated");
		return 0;
	case SSH_AUTH_PARTIAL:
		DPRINTLN("Some key matched but you still have to give \
			another mean of authentication (like password).");
		return -1;
	default:
		DPRINTLN("Unknown return from ssh_userauth_password(): " << ret);
		return -1;
	}
}

int Server::authenticatePubKey()
{
	int ret = ssh_userauth_autopubkey(session);	
	switch(ret)
	{
	case SSH_AUTH_ERROR:
		DPRINTLN("Some serious error happened during authentication");
		return -1;
	case SSH_AUTH_DENIED:
		DPRINTLN("Authentication failed");
		return -1;
	case SSH_AUTH_SUCCESS:
		DPRINTLN("You are now authenticated");
		return 0;
	case SSH_AUTH_PARTIAL:
		DPRINTLN("Some key matched but you still have to give \
			another mean of authentication (like password).");
		break;
		return -1;
	default:
		DPRINTLN("Unknown return from ssh_userauth_password(): " << ret);
		return -1;
	}
}

void ErrorExit(char* lpszFunction) 
{ 
    char* lpMsgBuf;
    char* lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );
    

    lpDisplayBuf = (char*)LocalAlloc(LMEM_ZEROINIT, 
        (strlen(lpMsgBuf)+strlen(lpszFunction)+40)*sizeof(TCHAR)); 
    wsprintf(lpDisplayBuf, 
        TEXT("%s failed with error %d: %s"), 
        lpszFunction, dw, lpMsgBuf); 
    MessageBox(NULL, lpDisplayBuf, TEXT("Error"), MB_OK); 

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(dw); 
}


int Server::authenticateKeyboardInteractive()
{
	char echo, ch;
	
	stringstream answerBuffer;
	string answer;
	const char* c_answer;
	
	int ret = ssh_userauth_kbdint(session, NULL, NULL);
	int promptCount;
	
	// Keep presenting messages and prompts to user as server sends them
	do {
		cout << ssh_userauth_kbdint_getname(session) << endl;
		cout << ssh_userauth_kbdint_getinstruction(session) << endl;
		
		promptCount = ssh_userauth_kbdint_getnprompts(session);
		for(int i=0; i<promptCount; i++)
		{
			// Output next prompt
			cout 
				<< ssh_userauth_kbdint_getprompt(session, i, &echo) 
				<< endl;
			
			// Read in next answer
/*			do {
				cin.get(ch);
				answerBuffer << ch;
			} while (ch != '\n' && ch != '\r');
			answerBuffer << '\n' << '\n';
			answer = answerBuffer.str();
			c_answer = answer.c_str();*/
			
			// Read in next answer
			cin >> answer;
			c_answer = answer.c_str();
			
			// Send answer to server
			ssh_userauth_kbdint_setanswer(
				session, 
				i, 
				(char *)c_answer // Safe to cast as function copies string
			);
			cout << c_answer;
		}
		
		ret = ssh_userauth_kbdint(session, NULL, NULL);
	} while (ret == SSH_AUTH_INFO);
	
	// Server need no more input and has returned auth status
	switch(ret)
	{
	case SSH_AUTH_INFO:
		assert("This condition should not be reached outside do/while loop");
		return -1;
	case SSH_AUTH_ERROR:
		DPRINTLN("Some serious error happened during authentication");
		return -1;
	case SSH_AUTH_DENIED:
		DPRINTLN("Authentication failed");
		return -1;
	case SSH_AUTH_SUCCESS:
		DPRINTLN("You are now authenticated");
		return 0;
	case SSH_AUTH_PARTIAL:
		DPRINTLN("Some key matched but you still have to give \
			another mean of authentication (like password).");
		return -1;
	default:
		DPRINTLN("Unknown return from ssh_userauth_password(): " << ret);
		return -1;
	}
}

/* $Id$ */
