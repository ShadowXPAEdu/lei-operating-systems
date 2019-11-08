#include "msgdist.h"

int IsServerRunning(const char *path) {
    FILE *fptr = fopen(path, "r+");

    if (fptr == NULL)
        return 0;

    fclose(fptr);
    return -1;
}

char *CMD_ID_to_STR(int CMD) {
    switch(CMD) {
    case CMD_ALERTSUB:
        return "CMD_ALERTSUB";
    case CMD_ALIVE:
        return "CMD_ALIVE";
    case CMD_CON:
        return "CMD_CON";
    case CMD_DC:
        return "CMD_DC";
    case CMD_ERR:
        return "CMD_ERR";
    case CMD_FDC:
        return "CMD_FDC";
    case CMD_GETMSG:
        return "CMD_GETMSG";
    case CMD_GETTITLES:
        return "CMD_GETTITLES";
    case CMD_GETTOPICS:
        return "CMD_GETTOPICS";
    case CMD_HEARTBEAT:
        return "CMD_HEARTBEAT";
    case CMD_IGN:
        return "CMD_IGN";
    case CMD_INVMSG:
        return "CMD_INVMSG";
    case CMD_NEWMSG:
        return "CMD_NEWMSG";
    case CMD_OK:
        return "CMD_OK";
    case CMD_SDC:
        return "CMD_SDC";
    case CMD_SUB:
        return "CMD_SUB";
    case CMD_UNSUB:
        return "CMD_UNSUB";
    default:
        return "UNKNOWN";
    }
}
