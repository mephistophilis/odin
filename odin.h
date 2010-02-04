#define ODIN_CMD_GET_VERSION    0x2C8
#define ODIN_CMD_GET_PLATFORM   0x2C9

struct odin_msg {
    unsigned int cmd;
    unsigned int arg;
    unsigned int len;
    unsigned int flag;
} __attribute__((__packed__));

