/*
Copyright 2003,04 Aris Adamantiadis

This file is part of the SSH Library

The SSH Library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version.

The SSH Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the SSH Library; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA. */

/* priv.h file */
/* This include file contains everything you shouldn't deal with in user programs. */
/* Consider that anything in this file might change without notice; libssh.h file will keep */
/* backward compatibility on binary & source */

#ifndef _LIBSSH_PRIV_H
#define _LIBSSH_PRIV_H
#include "libssh/libssh.h"

#ifdef HAVE_CONFIG_H
#include "libssh/config.h"
#endif

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#define HAVE_SELECT 1
#define HAVE_GETHOSTBYADDR 1
#define HAVE_GETHOSTBYNAME 1
#define HAVE_INET_ADDR 1
#undef HAVE_INET_ATON
#undef HAVE_SIGNAL
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_MALLOC
#if HAVE_MALLOC == 0
void *rpl_malloc(size_t n);
#endif
#endif

/* Debugging constants */

/* Define this if you want to debug crypto systems */
/* it's usefull when you are debugging the lib */
/*#define DEBUG_CRYPTO */

/* some constants */
#define MAX_PACKET_LEN 262144
#define ERROR_BUFFERLEN 1024
#define CLIENTBANNER "SSH-2.0-" LIBSSH_VERSION
#define KBDINT_MAX_PROMPT 256 /* more than openssh's :) */
/* some types for public keys */
#define TYPE_DSS 1
#define TYPE_RSA 2
#define TYPE_RSA1 3

/* profiling constants. Don't touch them unless you know what you do */
#define OPENSSL_CRYPTO
#define OPENSSL_BIGNUMS


#ifdef __cplusplus
extern "C" {
#endif

/* wrapper things */

#ifdef OPENSSL_CRYPTO
#include <openssl/dsa.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <openssl/hmac.h>
typedef SHA_CTX SHACTX;
typedef MD5_CTX MD5CTX;
typedef HMAC_CTX HMACCTX;
#ifdef MD5_DIGEST_LEN
    #undef MD5_DIGEST_LEN
#endif
#define SHA_DIGEST_LEN SHA_DIGEST_LENGTH
#define MD5_DIGEST_LEN MD5_DIGEST_LENGTH

#endif /* OPENSSL_CRYPTO */
#ifdef OPENSSL_BIGNUMS
#include <openssl/bn.h>
typedef BIGNUM*  bignum;
typedef BN_CTX* bignum_CTX;

#define bignum_new() BN_new()
#define bignum_free(num) BN_clear_free(num)
#define bignum_set_word(bn,n) BN_set_word(bn,n)
#define bignum_bin2bn(bn,datalen,data) BN_bin2bn(bn,datalen,data)
#define bignum_bn2hex(num) BN_bn2hex(num)
#define bignum_rand(rnd, bits, top, bottom) BN_rand(rnd,bits,top,bottom)
#define bignum_ctx_new() BN_CTX_new()
#define bignum_ctx_free(num) BN_CTX_free(num)
#define bignum_mod_exp(dest,generator,exp,modulo,ctx) BN_mod_exp(dest,generator,exp,modulo,ctx)
#define bignum_num_bytes(num) BN_num_bytes(num)
#define bignum_num_bits(num) BN_num_bits(num)
#define bignum_is_bit_set(num,bit) BN_is_bit_set(num,bit)
#define bignum_bn2bin(num,ptr) BN_bn2bin(num,ptr)

#endif /* OPENSSL_BIGNUMS */
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

/* wrapper.c */
MD5CTX *md5_init(void);
void md5_update(MD5CTX *c, const void *data, unsigned long len);
void md5_final(unsigned char *md,MD5CTX *c);
SHACTX *sha1_init(void);
void sha1_update(SHACTX *c, const void *data, unsigned long len);
void sha1_final(unsigned char *md,SHACTX *c);
void sha1(unsigned char *digest,int len,unsigned char *hash);
#define HMAC_SHA1 1
#define HMAC_MD5 2
HMACCTX *hmac_init(const void *key,int len,int type);
void hmac_update(HMACCTX *c, const void *data, unsigned long len);
void hmac_final(HMACCTX *ctx,unsigned char *hashmacbuf,int *len);

/* strings and buffers */
/* must be 32 bits number + immediatly our data */
struct string_struct {
	u32 size;
	char string[MAX_PACKET_LEN];
} __attribute__ ((packed));


struct buffer_struct {
    char *data;
    int used;
    int allocated;
    int pos;
};

/* i should remove it one day */
typedef struct packet_struct {
	int valid;
	u32 len;
	u8 type;
} PACKET;

typedef struct kex_struct {
	char cookie[16];
	char **methods;
} KEX;

struct public_key_struct {
    int type;
    char *type_c; /* Don't free it ! it is static */
    DSA *dsa_pub;
    RSA *rsa_pub;
};

struct private_key_struct {
    int type;
    DSA *dsa_priv;
    RSA *rsa_priv;
};

typedef struct signature_struct {
    int type;
    DSA_SIG *dsa_sign;
    STRING *rsa_sign;
} SIGNATURE;

struct ssh_options_struct {
    char *clientbanner; /* explicit banner to send */
    char *username;
    char *host;
    char *bindaddr;
    char *identity;
    char *ssh_dir;
    char *known_hosts_file;
    int fd; /* specificaly wanted file descriptor, don't connect host */
    int port;
    int dont_verify_hostkey; /* Don't spare time, don't check host key ! unneeded to say it's dangerous and not safe */
    int use_nonexisting_algo; /* if user sets a not supported algorithm for kex, don't complain */
    char *wanted_methods[10]; /* the kex methods can be choosed. better use the kex fonctions to do that */
    void *wanted_cookie; /* wants a specific cookie to be sent ? if null, generate a new one */
    void *passphrase_function; /* this functions will be called if a keyphrase is needed. look keyfiles.c for more info */
    void (*connect_status_function)(void *arg, float status); /* status callback function */
    void *connect_status_arg; /* arbitrary argument */
    long timeout; /* seconds */
    long timeout_usec;
    };

typedef struct ssh_crypto_struct {
    bignum e,f,x,k;
    char session_id[SHA_DIGEST_LEN];
    
    char encryptIV[SHA_DIGEST_LEN];
    char decryptIV[SHA_DIGEST_LEN];

    char decryptkey[SHA_DIGEST_LEN*2];
    char encryptkey[SHA_DIGEST_LEN*2];

    char encryptMAC[SHA_DIGEST_LEN];
    char decryptMAC[SHA_DIGEST_LEN];
    char hmacbuf[EVP_MAX_MD_SIZE];
    struct crypto_struct *in_cipher, *out_cipher; /* the cipher structures/objects */
    STRING *server_pubkey;
    char *server_pubkey_type;
    int do_compress_out; /* idem */
    int do_compress_in; /* don't set them, set the option instead */
    void *compress_out_ctx; /* don't touch it */
    void *compress_in_ctx; /* really, don't */
} CRYPTO;

struct channel_struct {
    struct channel_struct *prev;
    struct channel_struct *next;
    SSH_SESSION *session; /* SSH_SESSION pointer */
    u32 local_channel;
    u32 local_window;
    int local_eof;
    u32 local_maxpacket;
    u32 remote_channel;
    u32 remote_window;
    int remote_eof; /* end of file received */
    u32 remote_maxpacket;
    int open; /* shows if the channel is still opened */
    void (*write_fct)(struct channel_struct *channel, void *data, int len, void *userarg);
    /* this write function is a callback on some userdefined function which is used for writing datas *coming from remote ssh* */
    /* use channel_write() to write into a ssh pipe */
    void (*write_err_fct)(struct channel_struct *channel, void *data, int len, void *userarg);
    /* same as write_fct for stderr */
    BUFFER *stdout_buffer;
    BUFFER *stderr_buffer;
    void *userarg;
};

struct ssh_session {
    int fd;
    SSH_OPTIONS *options;
    char *serverbanner;
    char *clientbanner;
    int protoversion;
    u32 send_seq;
    u32 recv_seq;
    int connected; /* !=0 when the user got a session handle */
    int alive;
    int auth_service_asked;
    int datatoread; /* reading now on socket will not block */
    STRING *banner; /* that's the issue banner from the server */
    BUFFER *in_buffer;
    PACKET in_packet;
    BUFFER *out_buffer;
    KEX server_kex;
    KEX client_kex;
    BUFFER *in_hashbuf;
    BUFFER *out_hashbuf;
    CRYPTO *current_crypto;
    CRYPTO *next_crypto;  /* next_crypto is going to be used after a SSH2_MSG_NEWKEYS */
    CHANNEL *channels; /* linked list of channels */
    int maxchannel;
    int error_code;
    char error_buffer[ERROR_BUFFERLEN];
    struct ssh_kbdint *kbdint;
};

struct ssh_kbdint {
    u32 nprompts;
    char *name;
    char *instruction;
    char **prompts;
    char *echo; /* bool array */
    char **answers;
};

/* errors.c */
void ssh_set_error(SSH_SESSION *session,enum ssh_error code,char *descr,...);

/* in dh.c */
/* DH key generation */
void dh_generate_e(SSH_SESSION *session);
void dh_generate_x(SSH_SESSION *session);
STRING *dh_get_e(SSH_SESSION *session);
void dh_import_f(SSH_SESSION *session,STRING *f_string);
void dh_import_pubkey(SSH_SESSION *session,STRING *pubkey_string);
void dh_build_k(SSH_SESSION *session);
void make_sessionid(SSH_SESSION *session);
/* add data for the final cookie */
void hashbufin_add_cookie(SSH_SESSION *session,unsigned char *cookie);
void hashbufout_add_cookie(SSH_SESSION *session);
void generate_session_keys(SSH_SESSION *session);
/* returns 1 if server signature ok, 0 otherwise. The NEXT crypto is checked, not the current one */
int signature_verify(SSH_SESSION *session,STRING *signature);
bignum make_string_bn(STRING *string);
STRING *make_bignum_string(bignum num);

/* in crypt.c */
u32 packet_decrypt_len(SSH_SESSION *session,char *crypted);
int packet_decrypt(SSH_SESSION *session, void *packet,unsigned int len);
char *packet_encrypt(SSH_SESSION *session,void *packet,unsigned int len);
 /* it returns the hmac buffer if exists*/
int packet_hmac_verify(SSH_SESSION *session,BUFFER *buffer,char *mac);

/* in packet.c */
void packet_clear_out(SSH_SESSION *session);
void packet_parse(SSH_SESSION *session);
int packet_send(SSH_SESSION *session);
int packet_read(SSH_SESSION *session);
int packet_translate(SSH_SESSION *session);
int packet_wait(SSH_SESSION *session,int type,int blocking);

/* connect.c */
SSH_SESSION *ssh_session_new();
int ssh_connect_host(const char *host,const char *bind_addr, int port, long timeout, long usec);
int ssh_net_init(void);

/* in kex.c */
extern char *ssh_kex_nums[];
void send_kex(SSH_SESSION *session,int server_kex);
void list_kex(KEX *kex);
int set_kex(SSH_SESSION *session);
int ssh_get_kex(SSH_SESSION *session, int server_kex);
int verify_existing_algo(int algo,char *name);
char **space_tokenize(char *chain);

/* in keys.c */
char *ssh_type_to_char(int type);
PUBLIC_KEY *publickey_make_dss(BUFFER *buffer);
PUBLIC_KEY *publickey_make_rsa(BUFFER *buffer);
PUBLIC_KEY *publickey_from_string(STRING *pubkey_s);
SIGNATURE *signature_from_string(STRING *signature,PUBLIC_KEY *pubkey,int needed_type);
void signature_free(SIGNATURE *sign);
STRING *ssh_do_sign(SSH_SESSION *session,BUFFER *sigbuf, PRIVATE_KEY *privatekey);

/* channel.c */
void channel_handle(SSH_SESSION *session, int type);

/* options.c */
void options_free(SSH_OPTIONS *opt);
/* this function must be called when no specific username has been asked. it has to guess it */
int options_default_username(SSH_OPTIONS *opt);
int options_default_ssh_dir(SSH_OPTIONS *opt);
int options_default_known_hosts_file(SSH_OPTIONS *opt);

/* buffer.c */
void buffer_add_ssh_string(BUFFER *buffer,STRING *string);
void buffer_add_u8(BUFFER *buffer, u8 data);
void buffer_add_u32(BUFFER *buffer, u32 data);
void buffer_add_u64(BUFFER *buffer,u64 data);
void buffer_add_data(BUFFER *buffer, void *data, int len);
void buffer_add_data_begin(BUFFER *buffer,void *data,int len);
void buffer_add_buffer(BUFFER *buffer, BUFFER *source);
void buffer_reinit(BUFFER *buffer);

/* buffer_get_rest returns a pointer to the current position into the buffer */
void *buffer_get_rest(BUFFER *buffer);
/* buffer_get_rest_len returns the number of bytes which can be read */
int buffer_get_rest_len(BUFFER *buffer);

/* buffer_read_*() returns the number of bytes read, except for ssh strings */
int buffer_get_u8(BUFFER *buffer,u8 *data);
int buffer_get_u32(BUFFER *buffer,u32 *data);
int buffer_get_u64(BUFFER *buffer, u64 *data);

int buffer_get_data(BUFFER *buffer,void *data,int requestedlen);
/* buffer_get_ssh_string() is an exception. if the String read is too large or invalid, it will answer NULL. */
STRING *buffer_get_ssh_string(BUFFER *buffer);
/* buffer_pass_bytes acts as if len bytes have been read (used for padding) */
int buffer_pass_bytes_end(BUFFER *buffer,int len);
int buffer_pass_bytes(BUFFER *buffer, int len);

/* in base64.c */
BUFFER *base64_to_bin(char *source);
char *bin_to_base64(unsigned char *source, int len);

/* gzip.c */
int compress_buffer(SSH_SESSION *session,BUFFER *buf);
int decompress_buffer(SSH_SESSION *session,BUFFER *buf);

/* wrapper.c */
int crypt_set_algorithms(SSH_SESSION *);
CRYPTO *crypto_new();
void crypto_free(CRYPTO *crypto);
bignum bignum_new();

/* in misc.c */
/* gets the user home dir. */
char *ssh_get_user_home_dir();
int ssh_file_readaccess_ok(char *file);

/* macro for byte ordering */
u64 ntohll(u64);
#define htonll(x) ntohll(x)


#ifdef __cplusplus
} ;
#endif

#endif /* _LIBSSH_PRIV_H */
