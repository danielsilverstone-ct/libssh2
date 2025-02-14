/*
 * Sample doing an SFTP directory listing.
 *
 * The sample code has default values for host name, user name, password and
 * path, but you can specify them on the command line like:
 *
 * "sftpdir 192.168.0.1 user password /tmp/secretdir"
 */

#ifdef WIN32
#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif
#endif

#include "libssh2_config.h"
#include <libssh2.h>
#include <libssh2_sftp.h>

#ifdef HAVE_WINSOCK2_H
# include <winsock2.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#endif

#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

#if defined(_MSC_VER) && _MSC_VER < 1900
#pragma warning(disable:4127)
#endif

#if defined(_MSC_VER)
#define __FILESIZE "I64u"
#else
#define __FILESIZE "llu"
#endif

const char *keyfile1 = "~/.ssh/id_rsa.pub";
const char *keyfile2 = "~/.ssh/id_rsa";
const char *username = "username";
const char *password = "password";

static void kbd_callback(const char *name, int name_len,
                         const char *instruction, int instruction_len,
                         int num_prompts,
                         const LIBSSH2_USERAUTH_KBDINT_PROMPT *prompts,
                         LIBSSH2_USERAUTH_KBDINT_RESPONSE *responses,
                         void **abstract)
{
    (void)name;
    (void)name_len;
    (void)instruction;
    (void)instruction_len;
    if(num_prompts == 1) {
        responses[0].text = strdup(password);
        responses[0].length = (unsigned int)strlen(password);
    }
    (void)prompts;
    (void)abstract;
} /* kbd_callback */

int main(int argc, char *argv[])
{
    uint32_t hostaddr;
    libssh2_socket_t sock;
    int rc, i, auth_pw = 0;
    struct sockaddr_in sin;
    const char *fingerprint;
    char *userauthlist;
    LIBSSH2_SESSION *session;
    const char *sftppath = "/tmp/secretdir";
    LIBSSH2_SFTP *sftp_session;
    LIBSSH2_SFTP_HANDLE *sftp_handle;

#ifdef WIN32
    WSADATA wsadata;
    int err;

    err = WSAStartup(MAKEWORD(2, 0), &wsadata);
    if(err != 0) {
        fprintf(stderr, "WSAStartup failed with error: %d\n", err);
        return 1;
    }
#endif

    if(argc > 1) {
        hostaddr = inet_addr(argv[1]);
    }
    else {
        hostaddr = htonl(0x7F000001);
    }

    if(argc > 2) {
        username = argv[2];
    }
    if(argc > 3) {
        password = argv[3];
    }
    if(argc > 4) {
        sftppath = argv[4];
    }

    rc = libssh2_init(0);
    if(rc != 0) {
        fprintf(stderr, "libssh2 initialization failed (%d)\n", rc);
        return 1;
    }

    /*
     * The application code is responsible for creating the socket
     * and establishing the connection
     */
    sock = socket(AF_INET, SOCK_STREAM, 0);

    sin.sin_family = AF_INET;
    sin.sin_port = htons(22);
    sin.sin_addr.s_addr = hostaddr;
    if(connect(sock, (struct sockaddr*)(&sin),
               sizeof(struct sockaddr_in)) != 0) {
        fprintf(stderr, "failed to connect!\n");
        return -1;
    }

    /* Create a session instance
     */
    session = libssh2_session_init();
    if(!session)
        return -1;

    /* ... start it up. This will trade welcome banners, exchange keys,
     * and setup crypto, compression, and MAC layers
     */
    rc = libssh2_session_handshake(session, sock);
    if(rc) {
        fprintf(stderr, "Failure establishing SSH session: %d\n", rc);
        return -1;
    }

    /* At this point we have not yet authenticated.  The first thing to do
     * is check the hostkey's fingerprint against our known hosts Your app
     * may have it hard coded, may go to a file, may present it to the
     * user, that's your call
     */
    fingerprint = libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_SHA1);
    fprintf(stderr, "Fingerprint: ");
    for(i = 0; i < 20; i++) {
        fprintf(stderr, "%02X ", (unsigned char)fingerprint[i]);
    }
    fprintf(stderr, "\n");

    /* check what authentication methods are available */
    userauthlist = libssh2_userauth_list(session, username,
                                         (unsigned int)strlen(username));
    fprintf(stderr, "Authentication methods: %s\n", userauthlist);
    if(strstr(userauthlist, "password") != NULL) {
        auth_pw |= 1;
    }
    if(strstr(userauthlist, "keyboard-interactive") != NULL) {
        auth_pw |= 2;
    }
    if(strstr(userauthlist, "publickey") != NULL) {
        auth_pw |= 4;
    }

    /* if we got an 5. argument we set this option if supported */
    if(argc > 5) {
        if((auth_pw & 1) && !strcmp(argv[5], "-p")) {
            auth_pw = 1;
        }
        if((auth_pw & 2) && !strcmp(argv[5], "-i")) {
            auth_pw = 2;
        }
        if((auth_pw & 4) && !strcmp(argv[5], "-k")) {
            auth_pw = 4;
        }
    }

    if(auth_pw & 1) {
        /* We could authenticate via password */
        if(libssh2_userauth_password(session, username, password)) {
            fprintf(stderr, "\tAuthentication by password failed!\n");
            goto shutdown;
        }
        else {
            fprintf(stderr, "\tAuthentication by password succeeded.\n");
        }
    }
    else if(auth_pw & 2) {
        /* Or via keyboard-interactive */
        if(libssh2_userauth_keyboard_interactive(session, username,
                                                 &kbd_callback) ) {
            fprintf(stderr,
                    "\tAuthentication by keyboard-interactive failed!\n");
            goto shutdown;
        }
        else {
            fprintf(stderr,
                    "\tAuthentication by keyboard-interactive succeeded.\n");
        }
    }
    else if(auth_pw & 4) {
        /* Or by public key */
        if(libssh2_userauth_publickey_fromfile(session, username, keyfile1,
                                               keyfile2, password)) {
            fprintf(stderr, "\tAuthentication by public key failed!\n");
            goto shutdown;
        }
        else {
            fprintf(stderr, "\tAuthentication by public key succeeded.\n");
        }
    }
    else {
        fprintf(stderr, "No supported authentication methods found!\n");
        goto shutdown;
    }

    fprintf(stderr, "libssh2_sftp_init()!\n");
    sftp_session = libssh2_sftp_init(session);

    if(!sftp_session) {
        fprintf(stderr, "Unable to init SFTP session\n");
        goto shutdown;
    }

    /* Since we have not set non-blocking, tell libssh2 we are blocking */
    libssh2_session_set_blocking(session, 1);

    fprintf(stderr, "libssh2_sftp_opendir()!\n");
    /* Request a dir listing via SFTP */
    sftp_handle = libssh2_sftp_opendir(sftp_session, sftppath);

    if(!sftp_handle) {
        fprintf(stderr, "Unable to open dir with SFTP\n");
        goto shutdown;
    }
    fprintf(stderr, "libssh2_sftp_opendir() is done, now receive listing!\n");
    do {
        char mem[512];
        char longentry[512];
        LIBSSH2_SFTP_ATTRIBUTES attrs;

        /* loop until we fail */
        rc = libssh2_sftp_readdir_ex(sftp_handle, mem, sizeof(mem),
                                     longentry, sizeof(longentry), &attrs);
        if(rc > 0) {
            /* rc is the length of the file name in the mem
               buffer */

            if(longentry[0] != '\0') {
                printf("%s\n", longentry);
            }
            else {
                if(attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) {
                    /* this should check what permissions it
                       is and print the output accordingly */
                    printf("--fix----- ");
                }
                else {
                    printf("---------- ");
                }

                if(attrs.flags & LIBSSH2_SFTP_ATTR_UIDGID) {
                    printf("%4d %4d ", (int) attrs.uid, (int) attrs.gid);
                }
                else {
                    printf("   -    - ");
                }

                if(attrs.flags & LIBSSH2_SFTP_ATTR_SIZE) {
                    printf("%8" __FILESIZE " ", attrs.filesize);
                }

                printf("%s\n", mem);
            }
        }
        else
            break;

    } while(1);

    libssh2_sftp_closedir(sftp_handle);
    libssh2_sftp_shutdown(sftp_session);

 shutdown:

    libssh2_session_disconnect(session, "Normal Shutdown");
    libssh2_session_free(session);

#ifdef WIN32
    closesocket(sock);
#else
    close(sock);
#endif
    fprintf(stderr, "all done\n");

    libssh2_exit();

    return 0;
}
