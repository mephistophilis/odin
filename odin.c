#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "odin.h"

/**
 * Mine sweeper!
 * ldr r0, =0x81000000
 * mov r1, #0x0
 * str r1, [r0]
 */
static const unsigned char PAYLOAD[] = "\x81\x04\xa0\xe3\x00\x10\xa0\xe3\x00\x10\x80\xe5";

static FILE *serial = NULL;
static unsigned int bytes_sent = 0;

/**
 * Connect to the device through the USB serial interface
 */
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

/**
 * Read back a simple four byte reply
 */
unsigned int odin_read_int()
{
    unsigned int buf;

    if(!fread((char*)&buf, sizeof(buf), 1, serial)) {
        fprintf(stderr, "%s: Failed to read int\n", __FUNCTION__);
        return 0;
    }

    return buf;
}

/**
 * Sends a chunk of data to the device
 * Requires buf to be a multiple of MESSAGE_SIZE
 */
void odin_send_data(const unsigned char *buf, unsigned int len)
{
    unsigned char *sendbuf = (unsigned char*)malloc(MESSAGE_SIZE);
    unsigned char *p = (unsigned char*)buf;

    int left = (len - (p - buf));

    while(left > 0) {
        memset(sendbuf, 0, MESSAGE_SIZE);
        memcpy(sendbuf, p, (left < MESSAGE_SIZE) ? left : MESSAGE_SIZE);

        if(!fwrite(sendbuf, MESSAGE_SIZE, 1, serial)) {
            fprintf(stderr, "%s: Failed to send data\n", __FUNCTION__);
            exit(1);
        }

        fflush(serial);

        p += MESSAGE_SIZE;
        bytes_sent += MESSAGE_SIZE;
        left = (len - (p - buf));
    }
}

/**
 * Send a typical Odin command, aligned to a MESSAGE_SIZE boundary
 */
void odin_send_msg(struct odin_msg *msg)
{
    unsigned char *buf = (unsigned char*)malloc(MESSAGE_SIZE);
    memset(buf, 0, MESSAGE_SIZE);
    memcpy(buf, msg, sizeof(struct odin_msg));

    if(!fwrite(buf, MESSAGE_SIZE, 1, serial)) {
        fprintf(stderr, "%s: Failed to send message\n", __FUNCTION__);
        exit(1);
    }

    fflush(serial);

    free(buf);

    usleep(RESPONSE_DELAY);
}

/**
 * Get the Odin (protocol?) version
 */
unsigned int odin_get_version()
{
    struct odin_msg msg;
    memset(&msg, 0, sizeof(msg));

    msg.cmd = ODIN_CMD_GET_VERSION;

    odin_send_msg(&msg);
    return odin_read_int();
}

/**
 * Align upload to BLOCK_SIZE
 */
void odin_upload_finish()
{

    unsigned char *buf = (unsigned char*)malloc(MESSAGE_SIZE);
    memset(buf, 0, MESSAGE_SIZE);

    while((bytes_sent % BLOCK_SIZE) != 0) {
        odin_send_data(buf, MESSAGE_SIZE);
    }


    printf("Upload finished, sent %d bytes total...\n", bytes_sent);

    free(buf);
}

/**
 * Gets the 'platform' version and date
 */
unsigned int odin_get_platform()
{
    struct odin_msg msg;
    memset(&msg, 0, sizeof(msg));

    msg.cmd = ODIN_CMD_GET_PLATFORM;

    odin_send_msg(&msg);
    return odin_read_int();
}

/**
 * Close the serial connection
 */
void odin_close()
{
    fclose(serial);
    serial = NULL;
}

/**
 * Prepare the device for an upload by sending the upload length
 * and memory base of upload
 */
void odin_upload_prepare(unsigned int base, unsigned int len) {
    unsigned char base_idx;

    switch(base) {
        case RAM_BASE:
            base_idx = 0;
            break;
        case ONW_BASE:
            base_idx = 1;
            break;
        default:
            fprintf(stderr, "Invalid upload base specified!\n");
            exit(1);
    }

    /* Add one MESSAGE_SIZE for payload */
    len += MESSAGE_SIZE;

    /* Align to BLOCK_SIZE */
    len += (BLOCK_SIZE - (len % BLOCK_SIZE));

    printf("Preparing upload of length %08X (%d) at %#08X\n", len, len, base);

    struct odin_msg msg;
    memset(&msg, 0, sizeof(msg));

    msg.cmd = ODIN_CMD_DOWNLOAD_IMAGE;
    msg.len = len;
    msg.flag = base_idx;

    odin_send_msg(&msg);

    /* Do not remove, unless you fancy formatting your device.. */
    printf("Sending mine sweeper payload...\n");
    odin_send_data(PAYLOAD, sizeof(PAYLOAD));
}

/**
 * Upload a file
 */
void odin_upload_file(char *file)
{
    printf("Opening %s for upload...\n", file);

    FILE *fp = fopen(file, "rb");
    if(fp == NULL) {
        fprintf(stderr, "%s: Failed to open file: %s\n", __FUNCTION__, file);
    }

    fseek(fp, 0, SEEK_END);
    unsigned int len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if(len > MAX_UPLOAD_SIZE) {
        printf("Input file is too large (max: %d bytes)\n", MAX_UPLOAD_SIZE);
    }

    odin_upload_prepare(ONW_BASE, len);

    unsigned char *buf = (unsigned char*)malloc(MESSAGE_SIZE);
    memset(buf, 0, MESSAGE_SIZE);

    int num = 0;
    while((num = fread(buf, 1, MESSAGE_SIZE, fp))) {
        odin_send_data(buf, num);
        memset(buf, 0, MESSAGE_SIZE);
    }

    free(buf);

    odin_upload_finish();

    fclose(fp);
}

/**
 * Execute the OneNAND writer command to run our own code
 * Ensure the flag at 0x81000000 is cleared, else a
 * NAND format will be performed by Sbl.
 */
unsigned int odin_execute_onw()
{
    struct odin_msg msg;
    memset(&msg, 0, sizeof(msg));

    msg.cmd = ODIN_CMD_EXECUTE_ONW;

    odin_send_msg(&msg);

    return odin_read_int();
}

/**
 * Send the 'download finished' command to get
 * the device out of download mode.
 */
void odin_download_finish()
{
    struct odin_msg msg;
    memset(&msg, 0, sizeof(msg));

    msg.cmd = ODIN_CMD_DOWNLOAD_FINISH;

    odin_send_msg(&msg);
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
    printf("Odin version: %d date: %d\n",(version & 0xFFF), ((version & ~0xFFF) >> 12));

    unsigned int platform = odin_get_platform();
    printf("Odin platform: %X\n", platform);

    if(argc >= 2 && !strcmp(argv[1], "-f")) {
        printf("Sending 'download finished' command...\n");
        odin_download_finish();
        odin_close();
        exit(0);
    }

#ifdef VOID_MY_WARRANTY
    if(argc < 2) {
        fprintf(stderr, "To upload and execute use: odin <file>\n");
        exit(1);
    }

    odin_upload_file(argv[1]);
    odin_execute_onw();
#endif

    odin_close();
    return 0;
}

