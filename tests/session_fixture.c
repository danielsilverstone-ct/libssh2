/* Copyright (C) 2016 Alexander Lamaison
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms,
 * with or without modification, are permitted provided
 * that the following conditions are met:
 *
 *   Redistributions of source code must retain the above
 *   copyright notice, this list of conditions and the
 *   following disclaimer.
 *
 *   Redistributions in binary form must reproduce the above
 *   copyright notice, this list of conditions and the following
 *   disclaimer in the documentation and/or other materials
 *   provided with the distribution.
 *
 *   Neither the name of the copyright holder nor the names
 *   of any other contributors may be used to endorse or
 *   promote products derived from this software without
 *   specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */

#include "session_fixture.h"
#include "libssh2_config.h"
#include "openssh_fixture.h"

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef WIN32
#include <windows.h>
#ifdef _MSC_VER
#include <direct.h>
#define getcwd _getcwd
#define chdir _chdir
#endif
#endif
#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <assert.h>

LIBSSH2_SESSION *connected_session = NULL;
libssh2_socket_t connected_socket = LIBSSH2_INVALID_SOCKET;

static int connect_to_server(void)
{
    int rc;
    connected_socket = open_socket_to_openssh_server();
    if(connected_socket == LIBSSH2_INVALID_SOCKET) {
        return -1;
    }

    rc = libssh2_session_handshake(connected_session, connected_socket);
    if(rc != 0) {
        print_last_session_error("libssh2_session_handshake");
        return -1;
    }

    return 0;
}

static void setup_fixture_workdir(void)
{
#ifdef WIN32
    char wd_buf[_MAX_PATH];
#else
    char wd_buf[MAXPATHLEN];
#endif
    const char *wd = getenv("FIXTURE_WORKDIR");
#ifdef FIXTURE_WORKDIR
    if(!wd) {
        wd = FIXTURE_WORKDIR;
    }
#endif
    if(!wd) {
        getcwd(wd_buf, sizeof(wd_buf));
        wd = wd_buf;
    }

    chdir(wd);
}

LIBSSH2_SESSION *start_session_fixture(void)
{
    int rc;
    const char *env;

    setup_fixture_workdir();

    rc = start_openssh_fixture();
    if(rc != 0) {
        return NULL;
    }
    rc = libssh2_init(0);
    if(rc != 0) {
        fprintf(stderr, "libssh2_init failed (%d)\n", rc);
        return NULL;
    }

    connected_session = libssh2_session_init_ex(NULL, NULL, NULL, NULL);
    if(getenv("FIXTURE_TRACE_ALL")) {
        libssh2_trace(connected_session, ~0);
    }
    if(connected_session == NULL) {
        fprintf(stderr, "libssh2_session_init_ex failed\n");
        return NULL;
    }

    /* Override crypt algorithm for the test */
    env = getenv("FIXTURE_TEST_CRYPT");
    if(env) {
        if(libssh2_session_method_pref(connected_session,
                                       LIBSSH2_METHOD_CRYPT_CS, env) ||
           libssh2_session_method_pref(connected_session,
                                       LIBSSH2_METHOD_CRYPT_SC, env)) {
            fprintf(stderr, "libssh2_session_method_pref CRYPT failed "
                    "(probably disabled in the build): '%s'\n", env);
            return NULL;
        }
    }
    /* Override mac algorithm for the test */
    env = getenv("FIXTURE_TEST_MAC");
    if(env) {
        if(libssh2_session_method_pref(connected_session,
                                       LIBSSH2_METHOD_MAC_CS, env) ||
           libssh2_session_method_pref(connected_session,
                                       LIBSSH2_METHOD_MAC_SC, env)) {
            fprintf(stderr, "libssh2_session_method_pref MAC failed "
                    "(probably disabled in the build): '%s'\n", env);
            return NULL;
        }
    }

    libssh2_session_set_blocking(connected_session, 1);

    rc = connect_to_server();
    if(rc != 0) {
        return NULL;
    }

    return connected_session;
}

void print_last_session_error(const char *function)
{
    if(connected_session) {
        char *message;
        int rc =
            libssh2_session_last_error(connected_session, &message, NULL, 0);
        fprintf(stderr, "%s failed (%d): %s\n", function, rc, message);
    }
    else {
        fprintf(stderr, "No session");
    }
}

void stop_session_fixture(void)
{
    if(connected_session) {
        libssh2_session_disconnect(connected_session, "test ended");
        libssh2_session_free(connected_session);
        shutdown(connected_socket, 2);
        connected_session = NULL;
    }
    else {
        fprintf(stderr, "Cannot stop session - none started");
    }

    stop_openssh_fixture();
}


/* Return a static string that contains a file path relative to the srcdir
 * variable, if found. It does so in a way that avoids leaking memory by using
 * a fixed number of static buffers.
 */
#define NUMPATHS 3
const char *srcdir_path(const char *file)
{
#ifdef WIN32
    static char filepath[NUMPATHS][_MAX_PATH];
#else
    static char filepath[NUMPATHS][MAXPATHLEN];
#endif
    static int curpath;
    char *p = getenv("srcdir");
    assert(curpath < NUMPATHS);
    if(p) {
        /* Ensure the final string is nul-terminated on Windows */
        filepath[curpath][sizeof(filepath[0])-1] = 0;
        snprintf(filepath[curpath], sizeof(filepath[0])-1, "%s/%s",
                p, file);
    }
    else {
        /* Ensure the final string is nul-terminated on Windows */
        filepath[curpath][sizeof(filepath[0])-1] = 0;
        snprintf(filepath[curpath], sizeof(filepath[0])-1, "%s",
                file);
    }

    return filepath[curpath++];
}
