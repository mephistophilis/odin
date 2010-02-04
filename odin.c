#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "odin.h"

#define RESPONSE_DELAY 100000

static FILE *serial = NULL;

int odin_connect()
{
    serial = fopen("/dev/serial/by-id/usb-SAMSUNG_Gadget_Serial-if00", "rb+");

    if(serial == NULL) {
        return 0;
    }

    unsigned char buf[1024] = "SAMSUNG";

    if(!fwrite(buf, strlen((char*)buf), 1, serial)) {
        fprintf(stderr, "%s: Failed to send handshake\n", __FUNCTION__);
        return 0;
    }

    fflush(serial);
    usleep(RESPONSE_DELAY);

    if(fread(buf, 4, 1, serial)) {
        if(strncmp((char*)buf, "LAST", 4)) {
            fprintf(stderr, "%s: Unsupported Samsung device detected\n", __FUNCTION__);
            return 0;
        }
    } else {
        fprintf(stderr, "%s: Failed to read handshake response\n", __FUNCTION__);
        return 0;
    }

    return 1;
}

unsigned int odin_read_int()
{
    unsigned int buf;

    if(!fread((char*)&buf, sizeof(buf), 1, serial)) {
        fprintf(stderr, "%s: Failed to read int\n", __FUNCTION__);
        return 0;
    }

    return buf;
}

void odin_send_msg(struct odin_msg *msg)
{
    unsigned char buf[0x400];
    memset(buf, 0, sizeof(buf));
    memcpy(buf, msg, sizeof(struct odin_msg));

    if(!fwrite(buf, sizeof(buf), 1, serial)) {
        fprintf(stderr, "%s: Failed to send message\n", __FUNCTION__);
    }

    fflush(serial);
    usleep(RESPONSE_DELAY);
}

unsigned int odin_get_version()
{
    struct odin_msg msg;
    memset(&msg, 0, sizeof(msg));

    msg.cmd = ODIN_CMD_GET_VERSION;

    odin_send_msg(&msg);
    return odin_read_int();
}

unsigned int odin_get_platform()
{
    struct odin_msg msg;
    memset(&msg, 0, sizeof(msg));

    msg.cmd = ODIN_CMD_GET_PLATFORM;

    odin_send_msg(&msg);
    return odin_read_int();
}

void odin_close()
{
    fclose(serial);
    serial = NULL;
}

int main(int argc, char *argv[])
{
    if(odin_connect()) {
        printf("Connected to device\n");
    } else {
        fprintf(stderr, "Failed to connect to device\n");
        exit(1);
    }

    unsigned int version = odin_get_version();
    printf("Odin version: %d date: %d\n", (version & 0xFFF), ((version & ~0xFFF) >> 12));

    unsigned int platform = odin_get_platform();
    printf("Odin platform: %X\n", platform);

    odin_close();
    return 0;
}

