/* grove_uart_demo.c
 * Simple demo: initialize UART2 and echo received bytes to TX
 * Build with: make BOARD=epii_evb APP_TYPE=grove_uart_demo APPL_DEFINES+=-DENABLE_GROVE_UART2
 */

#include <stdio.h>
#include <stdint.h>
#include "WE2_device.h"
#ifdef IP_uart
#include "hx_drv_uart.h"
#endif

#include "grove_uart_demo.h"

void grove_uart_demo_app(void)
{
#ifdef IP_uart
    DEV_UART *uart2 = hx_drv_uart_get_dev(USE_DW_UART_2);
    if (uart2 == NULL) {
        printf("UART2 device not available\n");
        while (1);
    }

    uart2->uart_open(UART_BAUDRATE_115200);
    printf("Grove UART2 demo started (115200)\n");

    while (1) {
        uint8_t ch;
        /* blocking read */
        while (!uart2->uart_read((void*)&ch, 1)) {}
        /* echo back */
        uart2->uart_write((const void*)&ch, 1);
    }
#else
    printf("UART driver not enabled in this build\n");
    while (1);
#endif
}

