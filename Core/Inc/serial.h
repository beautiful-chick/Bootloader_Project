#ifndef SERIAL_H_
#define SERIAL_H_

#include <stdio.h>
#include<stdbool.h>
//#include "r_sci_uart.h"
//#include "hal_data.h"
#include "ringbuf.h"
#include "usart.h"
//#define RXBUF_SIZE      1024

/*+--------------------------------+
 *| standard output over UART4 API |
 *+--------------------------------+*/

#define g_console(x)    &huart1
#define g_console_ctrl  g_console(ctrl)
#define g_console_cfg   g_console(cfg)

typedef enum {
    UART_EVENT_TX_COMPLETE,
    UART_EVENT_RX_CHAR,
    // 其他可能的事件
} uart_event_t;



typedef struct uart_callback_args
{
    uart_event_t event;   // 事件类型，如 UART_EVENT_RX_CHAR 或 UART_EVENT_TX_COMPLETE
    uint32_t     data;    // 事件相关的数据，例如接收到的字符
    void         *p_context; // 上下文指针，通常用于传递用户定义的数据
} uart_callback_args_t;




extern volatile bool g_console_txComplete;

/* Function declaration */

extern int console_init(void);
extern int console_deinit(void);
extern int _write(int fd, char *pBuffer, int size);


/*+--------------------------------------+
 *| xymodem compatible with FreeRTOS API |
 *+--------------------------------------+*/

typedef long              BaseType_t;
typedef uint32_t          TickType_t;
typedef void *            xComPortHandle;
#define portBASE_TYPE     long

#define pdFALSE           ( ( BaseType_t ) 0 )
#define pdTRUE            ( ( BaseType_t ) 1 )


signed portBASE_TYPE xSerialGetChar( xComPortHandle pxPort, signed char *pcRxedChar, int tiemout );

int portBASE_TYPE xSerialPutChar(   char cOutChar );

void vSerialPutString( xComPortHandle pxPort, const signed char * const pcString, unsigned short usStringLength );

#endif /* SERIAL_H_ */
