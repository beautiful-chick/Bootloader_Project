#include <serial.h>
#include "ringbuf.h"
#include "keyled.h"

/*
 *+--------------------------------+
 *| standard output over UART0 API |
 *+--------------------------------+
 */

volatile bool g_console_txComplete = false;

#define RXBUF_SIZE  2048
extern uint8_t      g_xymodem_rxbuf[RXBUF_SIZE];
extern struct ring_buffer  g_xymodem_rb;





/*+------------------------+
 *| xymodem over UART4 API |
 *+------------------------+*/

/* In serial communication,
 * it is used to attempt to receive data within a limited waiting time,
 * utilizing the ring buffer `g_xymodem_rb` to store the received data,
 * and polling the buffer within the timeout period to check if there is data available to read.*/

signed portBASE_TYPE xSerialGetChar( xComPortHandle pxPort, signed char *pcRxedChar, int tiemout )
{
    portBASE_TYPE   rv = pdFALSE;

    /* The port handle is not required as this driver only supports one port. */
    ( void ) pxPort;


    while( tiemout -- )
    {
        if( rb_data_size(&g_xymodem_rb) > 0  )
        {
            rb_read(&g_xymodem_rb, (uint8_t *)pcRxedChar, 1);
            rv = pdTRUE;
            break;
        }
        HAL_Delay(1);
    }

    return rv;
}


/* This function `vSerialPutString` is used to send a string through the serial port
 * and wait for the transmission to complete.*/

void vSerialPutString( xComPortHandle pxPort, const signed char * const pcString, unsigned short usStringLength )
{
    /* The port handle is not required as this driver only supports one port. */
    ( void ) pxPort;

    if( !pcString || !usStringLength )
    {
        return ;
    }

    g_console_txComplete = false;

    uart_put_data( pcString, usStringLength, 1000);
    while(g_console_txComplete == false)
    {
    }
}

/* send a byte */
int portBASE_TYPE xSerialPutChar(   char cOutChar )
{
    /* Only one port is supported. */

    uart_put_data( &cOutChar, sizeof(cOutChar), 100);

    return 0;
}



