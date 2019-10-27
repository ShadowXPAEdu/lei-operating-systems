#include <stdlib.h>
#include <stdio.h>
#include "msgdist.h"
#include "msgdist_c.h"

int main(int argc, char* argv[], char** envp) {
    if (argc != 2) {
        printf("You need to specify a username!\nTry %s [username]\n", argv[0]);
        cl_exit(-1);
    } else {
        if (IsServerRunning(sv_fifo) == 0) {
            printf("Server is not online...\nPlease try another time.\n");
            cl_exit(-1);
        } else {
            // Server is online.. try to connect
        }
    }
    cl_exit(0);
}

void cl_exit(int return_val) {


    exit(return_val);
}
