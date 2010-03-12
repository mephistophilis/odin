#define RESPONSE_DELAY 50000

#define MESSAGE_SIZE 0x400
#define MESSAGES_PER_BLOCK 0x80

#define RAM_BASE    0x80000000
#define ONW_BASE    0x86C00000
#define SBL_BASE    0x87800000

/* Don't touch this */
#define BLOCK_SIZE      (MESSAGE_SIZE * MESSAGES_PER_BLOCK)
#define MAX_UPLOAD_SIZE (SBL_BASE - ONW_BASE - MESSAGE_SIZE) /* Reserve one message for cleanup payload */

/* Odin command opcodes */
#define ODIN_CMD_DOWNLOAD_IMAGE     0x2BC
#define ODIN_CMD_EXECUTE_ONW        0x2BE
#define ODIN_CMD_DOWNLOAD_FINISH    0x2C3
#define ODIN_CMD_GET_VERSION        0x2C8
#define ODIN_CMD_GET_PLATFORM       0x2C9

struct odin_msg {
    unsigned int cmd;
    unsigned int arg;
    unsigned int len;
    unsigned int flag;
} __attribute__((__packed__));

