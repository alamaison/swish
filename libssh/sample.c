/* client.c */
/*
Copyright 2003 Aris Adamantiadis

This file is part of the SSH Library

You are free to copy this file, modify it in any way, consider it being public
domain. This does not apply to the rest of the library though, but it is 
allowed to cut-and-paste working code from this file to any license of
program.
The goal is to show the API in action. It's not a reference on how terminal
clients must be made or how a client should react.
*/

#include "libssh/priv.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <sys/time.h>
#ifdef HAVE_PTY_H
#include <pty.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#include <errno.h>
#include <libssh/libssh.h>
#include <libssh/sftp.h>

#include <fcntl.h>

#define MAXCMD 10
char *host;
char *user;
int sftp;
char *cmds[MAXCMD];
#ifdef HAVE_TERMIOS_H
struct termios terminal;
#endif
void do_sftp(SSH_SESSION *session);

void add_cmd(char *cmd){
    int n;
    for(n=0;cmds[n] && (n<MAXCMD);n++);
    if(n==MAXCMD)
        return;
    cmds[n]=strdup(cmd);
}

void usage(){
    fprintf(stderr,"Usage : ssh [options] [login@]hostname\n"
    "Options :\n"
    "  -l user : log in as user\n"
    "  -p port : connect to port\n"
    "  -d : use DSS to verify host public key\n"
    "  -r : use RSA to verify host public key\n");
    exit(0);
}

int opts(int argc, char **argv){
    int i;
    if(strstr(argv[0],"sftp"))
        sftp=1;
//    for(i=0;i<argc;i++)
//        printf("%d : %s\n",i,argv[i]);
    /* insert your own arguments here */
    while((i=getopt(argc,argv,""))!=-1){
        switch(i){
            default:
                fprintf(stderr,"unknown option %c\n",optopt);
                usage();
        }
    }
    if(optind < argc)
        host=argv[optind++];
    while(optind < argc)
        add_cmd(argv[optind++]);
    if(host==NULL)
        usage();
    return 0;
}

void do_cleanup(){
#ifdef HAVE_TCSETATTR
    tcsetattr(0,TCSANOW,&terminal);
#endif
}
void do_exit(){
    do_cleanup();
    exit(0);
}
CHANNEL *chan;
int signal_delayed=0;
void setsignal();
void sigwindowchanged(){
    signal_delayed=1;
}
void sizechanged(){
#ifdef HAVE_PTY_H
    struct winsize win = { 0, 0, 0, 0 };
    ioctl(1, TIOCGWINSZ, &win);
    channel_change_pty_size(chan,win.ws_col, win.ws_row);
//    printf("Changed pty size\n");
    setsignal();
#endif
}
void setsignal(){
#ifdef HAVE_SIGNAL
    signal(SIGWINCH,sigwindowchanged);
    signal_delayed=0;
#endif
}

void select_loop(SSH_SESSION *session, CHANNEL *channel){
    fd_set fds;
    struct timeval timeout;
    char buffer[10];
    BUFFER *readbuf = buffer_new();
    CHANNEL *channels[2] = {channel, NULL};
    CHANNEL *outchannel[2] = {NULL, NULL};
    int lus;
    int eof = 0;
    int ret;
    while (channel) {
       /* when a signal is caught, ssh_select will return
         * with SSH_EINTR, which means it should be started 
         * again. It lets you handle the signal the faster you
         * can, like in this window changed example. Of course, if
         * your signal handler doesn't call libssh at all, you're
         * free to handle signals directly in sighandler.
         */
        do {
            FD_ZERO(&fds);
            if(!eof) {
                FD_SET(fileno(stdin), &fds);
            }
            timeout.tv_sec = 30;
            timeout.tv_usec = 0;
            ret = ssh_select(channels,outchannel,fileno(stdin)+1,&fds,&timeout);
            if (signal_delayed) {
                sizechanged();        
            }
        } while (ret==SSH_EINTR);
        if(FD_ISSET(fileno(stdin),&fds)){
            lus=fread(buffer,1,10,stdin);
            if(lus){
                channel_write(channel,buffer,lus);
            }
            else{
                eof=1;
                channel_send_eof(channel);
            }
        }
        if(outchannel[0]){
            while(channel_poll(outchannel[0],0)){
                lus=channel_read(outchannel[0],readbuf,0,0);
                if(lus==-1){
                    ssh_say(0,"error reading channel : %s\n",ssh_get_error(session));
                    return;
                }
                if(lus==0){
                    ssh_say(1,"EOF received\n");
                } else
                    write(1,buffer_get(readbuf),lus);
            }
            while(channel_poll(outchannel[0],1)){ /* stderr */
                lus=channel_read(outchannel[0],readbuf,0,1);
                if(lus==-1){
                    ssh_say(0,"error reading channel : %s\n",ssh_get_error(session));
                    return;
                }
                if(lus==0){
                    ssh_say(1,"EOF received\n");
                } else
                    write(2,buffer_get(readbuf),lus);
            }
        }
        if(!channel_is_open(channel)){
            channel_free(channel);
            channel=NULL;
        }
    }
    buffer_free(readbuf);
}

void shell(SSH_SESSION *session){
    CHANNEL *channel;
#ifdef HAVE_TERMIOS_H
    struct termios terminal_local;
#endif
    int interactive=isatty(0);
    if(interactive){
#ifdef HAVE_TCGETATTR
        tcgetattr(0,&terminal_local);
        memcpy(&terminal,&terminal_local,sizeof(struct termios));
        cfmakeraw(&terminal_local);
        tcsetattr(0,TCSANOW,&terminal_local);
#endif
        setsignal();
    }
#ifdef HAVE_SIGNAL
    signal(SIGTERM,do_cleanup);
#endif
    channel = channel_open_session(session);
    chan=channel;
    if(interactive){
        channel_request_pty(channel);
        sizechanged();
    }
    channel_request_shell(channel);
    select_loop(session,channel);
}
void batch_shell(SSH_SESSION *session){
    CHANNEL *channel;
    char buffer[1024];
    int i,s=0;
    for(i=0;i<MAXCMD && cmds[i];++i)
        s+=snprintf(buffer+s,sizeof(buffer)-s,"%s ",cmds[i]);
    channel=channel_open_session(session);
    if(channel_request_exec(channel,buffer)){
        printf("error executing \"%s\" : %s\n",buffer,ssh_get_error(session));
        return;
    }
    select_loop(session,channel);
}
        
/* it's just a proof of concept code for sftp, till i write a real documentation about it */
void do_sftp(SSH_SESSION *session){
    SFTP_SESSION *sftp=sftp_new(session);
    SFTP_DIR *dir;
    SFTP_ATTRIBUTES *file;
    SFTP_FILE *fichier;
    SFTP_FILE *to;
    int len=1;
    int i;
    char data[8000];
    if(!sftp){
        ssh_say(0,"sftp error initialising channel : %s\n",ssh_get_error(session));
        return;
    }
    if(sftp_init(sftp)){
        ssh_say(0,"error initialising sftp : %s\n",ssh_get_error(session));
        return;
    }
    /* the connection is made */
    /* opening a directory */
    dir=sftp_opendir(sftp,"./");
    if(!dir) {
        ssh_say(0,"Directory not opened(%s)\n",ssh_get_error(session));
        return ;
    }
    /* reading the whole directory, file by file */
    while((file=sftp_readdir(sftp,dir))){
        ssh_say(0,"%30s(%.8lo) : %.5d.%.5d : %.10lld bytes\n",file->name,file->permissions,file->uid,file->gid,file->size);
        sftp_attributes_free(file);
    }
    /* when file=NULL, an error has occured OR the directory listing is end of file */
    if(!sftp_dir_eof(dir)){
        ssh_say(0,"error : %s\n",ssh_get_error(session));
        return;
    }
    if(sftp_dir_close(dir)){
        ssh_say(0,"Error : %s\n",ssh_get_error(session));
        return;
    }
    /* this will open a file and copy it into your /home directory */
    /* the small buffer size was intended to stress the library. of course, you can use a buffer till 20kbytes without problem */

    fichier=sftp_open(sftp,"/usr/bin/ssh",O_RDONLY,NULL);
    if(!fichier){
        ssh_say(0,"Error opening /usr/bin/ssh : %s\n",ssh_get_error(session));
        return;
    }
    /* open a file for writing... */
    to=sftp_open(sftp,"ssh-copy",O_WRONLY | O_CREAT,NULL);
    if(!to){
        ssh_say(0,"Error opening ssh-copy for writing : %s\n",ssh_get_error(session));
        return;
    }
    while((len=sftp_read(fichier,data,4096)) > 0){
        if(sftp_write(to,data,len)!=len){
            ssh_say(0,"error writing %d bytes : %s\n",len,ssh_get_error(session));
            return;
        }
    }
    printf("finished\n");
    if(len<0)
        ssh_say(0,"Error reading file : %s\n",ssh_get_error(session));     
    sftp_file_close(fichier);
    sftp_file_close(to);
    printf("fichiers fermés\n");
    to=sftp_open(sftp,"/tmp/grosfichier",O_WRONLY|O_CREAT,NULL);
    for(i=0;i<1000;++i){
        len=sftp_write(to,data,8000);
        printf("wrote %d bytes\n",len);
        if(len != 8000){
            printf("chunk %d : %d (%s)\n",i,len,ssh_get_error(session));
        }
    }
    sftp_file_close(to);
    /* close the sftp session */
    sftp_free(sftp);
    printf("session sftp terminée\n");
}
int auth_kbdint(SSH_SESSION *session){
    int err=ssh_userauth_kbdint(session,NULL,NULL);
    char *name,*instruction,*prompt,*ptr;
    char buffer[128];
    int i,n;
    char echo;
    while (err==SSH_AUTH_INFO){
        name=ssh_userauth_kbdint_getname(session);
        instruction=ssh_userauth_kbdint_getinstruction(session);
        n=ssh_userauth_kbdint_getnprompts(session);
        if(strlen(name)>0)
            printf("%s\n",name);
        if(strlen(instruction)>0)
            printf("%s\n",instruction);
        for(i=0;i<n;++i){
            prompt=ssh_userauth_kbdint_getprompt(session,i,&echo);
            if(echo){
                printf("%s",prompt);
                fgets(buffer,sizeof(buffer),stdin);
                buffer[sizeof(buffer)-1]=0;
                if((ptr=strchr(buffer,'\n')))
                    *ptr=0;
                ssh_userauth_kbdint_setanswer(session,i,buffer);
                memset(buffer,0,strlen(buffer));
            } else {
#ifdef HAVE_GETPASS
                ptr=getpass(prompt);
#else
		fprintf(stderr, "%s", prompt);
		ptr = malloc(1024);
		fgets(ptr, 1024, stdin);
		if (strlen(ptr) > 0) {
			if (ptr[strlen(ptr) - 1] == '\n') {
				ptr[strlen(ptr) - 1] = '\0';
			}
		}
#endif
                ssh_userauth_kbdint_setanswer(session,i,ptr);
            }
        }
        err=ssh_userauth_kbdint(session,NULL,NULL);
    }
    return err;
}

int main(int argc, char **argv){
    SSH_SESSION *session;
    SSH_OPTIONS *options;
    int auth=0;
    char *password;
    char *banner;
    int state;
    char buf[10];
    char hash[MD5_DIGEST_LEN];

    options=ssh_getopt(&argc, argv);
    if(!options){
        fprintf(stderr,"Error : %s\n",ssh_get_error(NULL));
        usage();
        return -1;
    }    
    opts(argc,argv);
#ifdef HAVE_SIGNAL
    signal(SIGTERM,do_exit);
#endif
    if(user)
        options_set_username(options,user);
    options_set_host(options,host);
    session=ssh_connect(options);
    if(!session){
        fprintf(stderr,"[4891] Connection failed : %s\n",ssh_get_error(NULL));
        return -1;
    }

    state=ssh_is_server_known(session);
    switch(state){
        case SSH_SERVER_KNOWN_OK:
            break; /* ok */
        case SSH_SERVER_KNOWN_CHANGED:
            fprintf(stderr,"Host key for server changed : server's one is now :\n");
            ssh_get_pubkey_hash(session,hash);
            ssh_print_hexa("Public key hash",hash,MD5_DIGEST_LEN);
            fprintf(stderr,"For security reason, connection will be stopped\n");
            ssh_disconnect(session);
            exit(-1);
        case SSH_SERVER_FOUND_OTHER:
            fprintf(stderr,"The host key for this server was not found but an other type of key exists.\n");
            fprintf(stderr,"An attacker might change the default server key to confuse your client"
                "into thinking the key does not exist\n"
                "We advise you to rerun the client with -d or -r for more safety.\n");
                ssh_disconnect(session);
                exit(-1);
        case SSH_SERVER_NOT_KNOWN:
            fprintf(stderr,"The server is unknown. Do you trust the host key ?\n");
            ssh_get_pubkey_hash(session,hash);
            ssh_print_hexa("Public key hash",hash,MD5_DIGEST_LEN);
            fgets(buf,sizeof(buf),stdin);
            if(strncasecmp(buf,"yes",3)!=0){
                ssh_disconnect(session);
                exit(-1);
            }
            fprintf(stderr,"This new key will be written on disk for further usage. do you agree ?\n");
            fgets(buf,sizeof(buf),stdin);
            if(strncasecmp(buf,"yes",3)==0){
                if(ssh_write_knownhost(session))
                    fprintf(stderr,"error %s\n",ssh_get_error(session));
            }
            
            break;
        case SSH_SERVER_ERROR:
            fprintf(stderr,"%s",ssh_get_error(session));
            ssh_disconnect(session);
            exit(-1);
    }

    /* no ? you should :) */
    auth=ssh_userauth_autopubkey(session);
    if(auth==SSH_AUTH_ERROR){
        fprintf(stderr,"Authenticating with pubkey: %s\n",ssh_get_error(session));
        return -1;
    }
    banner=ssh_get_issue_banner(session);
    if(banner){
        printf("%s\n",banner);
        free(banner);
    }
    if(auth!=SSH_AUTH_SUCCESS){
        auth=auth_kbdint(session);
        if(auth==SSH_AUTH_ERROR){
            fprintf(stderr,"authenticating with keyb-interactive: %s\n",
                    ssh_get_error(session));
            return -1;
        }
    }
    if(auth!=SSH_AUTH_SUCCESS){
#ifdef HAVE_GETPASS
        password=getpass("Password : ");
#else
	fprintf(stderr, "Password : ");
	password = malloc(1024);
	fgets(password, 1024, stdin);
	if (strlen(password) > 0) {
		if (password[strlen(password) - 1] == '\n') {
			password[strlen(password) - 1] = '\0';
		}
	}
#endif
        if(ssh_userauth_password(session,NULL,password) != SSH_AUTH_SUCCESS){
            fprintf(stderr,"Authentication failed: %s\n",ssh_get_error(session));
            ssh_disconnect(session);
            return -1;
        }
        memset(password,0,strlen(password));
    }
    ssh_say(1,"Authentication success\n");
    if(!sftp){
        if(!cmds[0])
            shell(session);
        else
            batch_shell(session);
    }
    else
        do_sftp(session);    
    if(!sftp && !cmds[0])
        do_cleanup();
    ssh_disconnect(session);
    return 0;
}
