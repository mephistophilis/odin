/* Test code for the MADC function
*
* Hugo Vincent, May 2009
*/
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
typedef uint8_t	 u8;
typedef uint16_t	u16;
#include "twl4030-madc.h"
/* Note: (very weird) the madc driver seems to crash if you've got driver debugging
* turned on in your defconfig... */
/* Channel numbering:
* ADC0-1 : to do with battery charging, not relevant on Overo
*   ADC2-7 : general purpose, input range = 0 - 2.5V.
*            On Overo, ADC2 seems to read as ~0.4 and ADC7 as ~1.4V (?).
*   ADC8 : USB OTG port bus voltage.
*   ADC9-11 : more battery charging stuff, not relevant.
*   ADC12 : main battery voltage.
*           This will be the system 3.3V rail in our case
*   ADC13-15: reserved or not relevant.
*/
struct adc_channel {
       int number;
       char name[16];
       float input_range;
};
struct adc_channel channels[] = {
       {
               .number = 2,
               .name = "ADCIN2",
               .input_range = 2.5,
       },
       {
               .number = 3,
               .name = "ADCIN3",
               .input_range = 2.5,
       },
       {
               .number = 4,
               .name = "ADCIN4",
               .input_range = 2.5,
       },
       {
               .number = 5,
               .name = "ADCIN5",
               .input_range = 2.5,
       },
       {
               .number = 6,
               .name = "ADCIN6",
               .input_range = 2.5,
       },
       {
               .number = 7,
               .name = "ADCIN7",
               .input_range = 2.5,
       },
       {
               .number = 8,
               .name = "VBUS_USB_OTG",
               .input_range = 7.0,
       },
       {
               .number = 12,
               .name = "VBATT/3.3V_RAIL",
               .input_range = 6.0,
       },
};
int main(int argc, char **argv)
{
    int d = open("/dev/twl4030-madc", O_RDWR | O_NONBLOCK);
    if (d == -1)
    {
        printf("could not open device /dev/twl4030-madc \n");
        exit(EXIT_FAILURE);
    }
    struct twl4030_madc_user_parms *par;
    par = malloc(sizeof(struct twl4030_madc_user_parms));
    int j;
    for (j = 0; j < (sizeof(channels)/sizeof(struct adc_channel)); j++)
    {
        memset(par, 0, sizeof(struct twl4030_madc_user_parms));
        par->channel = channels[j].number;
        int ret = ioctl(d, TWL4030_MADC_IOCX_ADC_RAW_READ, par);
        float result = ((unsigned int)par->result) / 1024.f;/* 10 bit ADC -> 1024 */
        if (ret == 0 && par->status != -1)
        {
        printf("%s (channel %d): %f %d\n", channels[j].name, channels[j].number,result * channels[j].input_range, par->result);  
        }
        else
        {
            if (par->status == -1)
            {
                printf("Channel %d (%s) timed out!\n", j, channels[j].name);
                printf("ERROR\n");
                exit(EXIT_FAILURE);
            }
        }
    }	
    return 0;
}
