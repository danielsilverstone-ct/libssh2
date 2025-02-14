#include "session_fixture.h"
#include "runner.h"

#include <libssh2.h>

#include <stdio.h>

/* configured in Dockerfile */
static const char *USERNAME = "libssh2";
static const char *PASSWORD = "my test password";

int test(LIBSSH2_SESSION *session)
{
    int rc;

    const char *userauth_list =
        libssh2_userauth_list(session, USERNAME,
                              (unsigned int)strlen(USERNAME));
    if(userauth_list == NULL) {
        print_last_session_error("libssh2_userauth_list");
        return 1;
    }

    if(strstr(userauth_list, "password") == NULL) {
        fprintf(stderr, "'password' was expected in userauth list: %s\n",
                userauth_list);
        return 1;
    }

    rc = libssh2_userauth_password_ex(session, USERNAME,
                                      (unsigned int)strlen(USERNAME),
                                      PASSWORD,
                                      (unsigned int)strlen(PASSWORD), NULL);
    if(rc != 0) {
        print_last_session_error("libssh2_userauth_password_ex");
        return 1;
    }

    if(libssh2_userauth_authenticated(session) == 0) {
        fprintf(stderr, "Password auth appeared to succeed but "
                        "libssh2_userauth_authenticated returned 0\n");
        return 1;
    }

    return 0;
}
