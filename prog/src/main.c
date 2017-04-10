/*****************************************************************************
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 *
 *****************************************************************************/
#include <stm32f4xx.h>
#include <stdio.h>
#include <carme.h>
#include <can.h>
#include <lcd.h>

/* maximal number of chars per line on lcd */
#define LCD_STRING_LENGTH   (LCD_HOR_RESOLUTION / FONT_MIN_WIDTH)

/* ID for messages from board 1 */
#define TX_ID 0x666

/* ID for messages from borad 2 */
#define RX_ID 0x555

static __IO uint32_t TimingDelay;
CARME_CAN_ACCEPTANCE_FILTER af;

/* Pointers and variables */
volatile unsigned char* LED = (volatile unsigned char*)0x6C000200;
volatile unsigned char* SWITCH = (volatile unsigned char*)0x6C000400;
volatile unsigned char leds = 0;
volatile unsigned char switches = 0;

GPIO_InitTypeDef g;

/* Init GPIO */
void init_gpio(void)
{
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    g.GPIO_Pin= GPIO_Pin_0;
    g.GPIO_Mode     = GPIO_Mode_OUT;
    g.GPIO_OType    = GPIO_OType_PP;
    g.GPIO_Speed= GPIO_Speed_2MHz;
    g.GPIO_PuPd= GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &g);
}

/* Delay using systick */
void Delay(__IO uint32_t nTime)
{
    TimingDelay = nTime;
    while (TimingDelay != 0);
}

/* Systick handler */
void SysTick_Handler(void)
{
    if (TimingDelay) {
        TimingDelay--;
    }
}

/* setup acceptance filter */
void setup_acceptance_filter()
{
    /* set the SJA1000 chip in running mode */
     CARME_CAN_SetMode(CARME_CAN_DF_RESET);

    /* Acceptance Filter */
    af.afm = MODE_SINGLE;   /* Single filter */

    /* unmask the important bits by setting them to Zero */
    af.amr[0] = 0x00; /* unmask bit 0 - 7                         */
    af.amr[1] = 0x1f; /* unmask bit 8 - 6                         */
    af.amr[2] = 0xff; /* don't care in Mode with normal id length */
    af.amr[3] = 0xff; /* don't care in Mode with normal id length */

    /* set the bits which have to be high */
    af.acr[0] = 0xAA; /* 11001100                                 */
    af.acr[1] = 0xA0; /* 11000000                                 */
    af.acr[2] = 0x00; /* don't care in Mode with normal id length */
    af.acr[3] = 0x00; /* don't care in Mode with normal id length */
    
    /* set the AF */
    CARME_CAN_SetAcceptaceFilter(&af);

    /* set the SJA1000 chip in running mode */
    CARME_CAN_SetMode(CARME_CAN_DF_NORMAL);
}

/* Main loop */
int main(void)
{
    /* local variables */
    CARME_CAN_MESSAGE rxMsg; // rxMsg object
    CARME_CAN_MESSAGE txMsg; // txMsg object
    char LCDmsg[LCD_STRING_LENGTH]; // LCD String

    /* Default value for LEDs */
    *LED = 0b10101010;

    /* initialize systick with 1 kHz */
    if (SysTick_Config(SystemCoreClock / 1000)) {
        while (1);
    }

    /* initialize the LCD */
    LCD_Init();
    LCD_SetFont(&font_6x12);

    /* Init gpio */
    init_gpio();

    /* Init can chip */
    CARME_CAN_Init(CARME_CAN_BAUD_250K, CARME_CAN_DF_RESET);
    CARME_CAN_SetMode(CARME_CAN_DF_NORMAL);
    setup_acceptance_filter();

    /* initialize the CAN message */
    for(int i = 0; i < 7; i++){
        rxMsg.data[i] = 0;
    }

    /* initialize the CAN message */
    for(int i = 0; i < 7; i++){
        txMsg.data[i] = 0;
    }

    while (1) {

        /* --------------------------------------- Read switches and send value */

        /* Setup basic CAN message */
        txMsg.id = TX_ID; // Your ID
        txMsg.rtr = 0; // Something 
        txMsg.ext = 0; // Something
        txMsg.dlc = 1; // Send 1byte
        
        /* read switches */
        switches = *SWITCH;

        /* store switch state in data[1] */
        txMsg.data[0] = switches;

        /* send the message to the LCD */
        snprintf(LCDmsg,
                 sizeof(LCDmsg),
                 "CAN Tx: ID=0x%04lX DLC=%d D0=0x%02X",
                 txMsg.id,
                 txMsg.dlc,
                 txMsg.data[0]);

        LCD_Log_AddMsg(LCDmsg); // Display on LCD
        CARME_CAN_Write(&txMsg); // Send to CAN BUS

        /* --------------------------------------- Recieve value and set LEDS */

        /* check if anything is received */
        if (CARME_CAN_Read(&rxMsg) == CARME_NO_ERROR) {
            
            /* send it to LCD */
            snprintf(LCDmsg, 
                     sizeof(LCDmsg), 
                     "CAN Rx: ID=0x%04lX DLC=%d",
                     rxMsg.id, 
                     rxMsg.dlc);

            LCD_Log_AddMsg(LCDmsg);

            *LED = rxMsg.data[0];
        }
        
        Delay(1000); // Delay 1s
    }

    return 0;
}
