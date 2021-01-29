#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>

const int _ignore_lines = 1;
const int _string_length = 2048;
const char *_password_manager = "/usr/bin/pass";

/* Format:
 * url,username,password,totp,extra,name,grouping,fav
 */

void normalize_url(char *dest, const char *src);

int main() {
    char *format = (char *)malloc(sizeof(*format)*_string_length);
    char *format_name = (char *)malloc(sizeof(*format_name)*_string_length);
    char *s = (char *)malloc(sizeof(*s)*_string_length);
    char *url = (char *)malloc(sizeof(*url)*_string_length);
    char *username;
    char *password;
    char *totp;
    char *extra;
    char *name;
    char *grouping;

    pid_t child;
    int status;
    int exit_for_error = 0;
    int comm_pipe[2];

    int ignored = 0;
    while (fgets(s, _string_length, stdin) != NULL) {
        if (ignored < _ignore_lines) {
            ignored++;
            continue;
        }

        grouping = s;
        strsep(&grouping, ","); // url
        if (grouping == NULL) {
            continue;
        }
        username = strsep(&grouping, ",");
        password = strsep(&grouping, ",");
        totp = strsep(&grouping, ",");
        extra = strsep(&grouping, ",");
        name = strsep(&grouping, ",");
        *strchr(grouping, ',') = 0;

        normalize_url(url, s);

        //~ printf("url: %s ; username: %s ; password: %s ; name: %s; grouping: %s\n", url, username, password, name, grouping);

        pipe(comm_pipe);

        child = fork();

        if (grouping[0] != 0) {
            sprintf(format_name, "%s/%s", grouping, name);
        }
        else {
            sprintf(format_name, "%s", name);
        }

        if (child == 0) {
            memset(s, 0, _string_length);
            memset(username, 0, _string_length);
            memset(password, 0, _string_length);
            dup2(comm_pipe[0], STDIN_FILENO);
            close(comm_pipe[0]);
            close(comm_pipe[1]);

            //~ int err = execl("/usr/bin/more", "/usr/bin/more", NULL);//, "insert", "-m", format_name, NULL);
            int err = execl(_password_manager, _password_manager, "insert", "-m", format_name, NULL);

            fprintf(stderr, "error: failed to execute %s (%d)\n", _password_manager, err);
            exit(1);
        }
        close(comm_pipe[0]);

        sprintf(format, "%s\nURL: %s\nUsername: %s\n", password, url, username);
        //~ printf("format string: %s", format);

        if (write(comm_pipe[1], format, strlen(format)) != (ssize_t)strlen(format)) {
            fprintf(stderr, "error: could not write all data to child process\n");
            exit_for_error = 1;
        }
        close(comm_pipe[1]);
        if (child != wait(&status)) {
            fprintf(stderr, "error: wait of PID %d did not succeed\n", child);
            exit_for_error = 1;
        }
        if (!WIFEXITED(status)) {
            fprintf(stderr, "error: child process returned status (%d), exiting\n", status);
            exit_for_error = 1;
        }

        memset(format, 0, _string_length);
        memset(s, 0, _string_length);
        memset(url, 0, _string_length);
        if (exit_for_error) {
            break;
        }
        //~ break;
    }

    free(url);
    free(s);
    free(format);
    free(format_name);

    return 0;
}

void normalize_url(char *dest, const char *src) {
    dest[0] = '\0';

    char *url = strchr(src, '/');
    if (url == NULL) {
        return;
    }
    url += 2;
    char *post = strchr(url, '/');
    char *comma = strchr(src, '\0');
    if (comma == NULL) {
        return;
    }

    if (post == NULL || comma < post) {
        strncpy(dest, src, comma - src);
        return;
    }
    post += 1;

    strncpy(dest, src, post - src);
    strcat(dest, "*");
}
