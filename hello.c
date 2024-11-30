#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/pin_map.h"

void UART_Init(void);
void UART_SendString(const char *str);

int main(void) {
    uint8_t pb3_state;
    // Set up the system clock to 50 MHz
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);



    GPIO_Init();
    // Initialize UART for debugging
    UART_Init();

    // Continuously read the state of PB3
    while (1) {
        pb3_state = GPIOPinRead(GPIO_PORTB_BASE, GPIO_PIN_3);

        if (pb3_state == 0) { // Active low
            UART_SendString("PB3 is ON\r\n");
        } else {
            UART_SendString("PB3 is OFF\r\n");
        }
        UART_SendString("Current value of ")

        SysCtlDelay(SysCtlClockGet() / 3); // Delay to avoid flooding UART (approx. 1 second)
    }

    return 0;
}

void GPIO_Init(void) {
    // Enable GPIOB peripheral clock
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    // Wait for GPIOB to be ready
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB));

    // Configure PB3 as an input
    GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, GPIO_PIN_3);
    GPIOPadConfigSet(GPIO_PORTB_BASE, GPIO_PIN_3, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
}

unit8_t Read_GPIO_Input(void) {
    uint8_t value = 0;

    // Read PB3 and map to bit 1(b1)
    value |= ((GPIOinRead(GPIO_PORTB_BASE, GPIO_PIN_3) >> 3) & 0x01) << 0;

    // Read PC4-PC7 (b2-b5)
   value |= ((GPIOPinRead(GPIO_PORTC_BASE, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7) >> 4) & 0x0F) << 1;

   // Read PD6-PD7 (b6-b7)
   value |= ((GPIOPinRead(GPIO_PORTD_BASE, GPIO_PIN_6 | GPIO_PIN_7) >> 6) & 0x03) << 5;

   // Read PF4 (b8)
   value |= ((GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) >> 4) & 0x01) << 7;

   return value;
}

// UART Initialization
void UART_Init(void) {
    // Enable clock for UART0 and GPIOA
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0) || !SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA)) {}

    // Configure PA0 and PA1 for UART
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Configure UART0
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
}

// UART Helper Functions
void UART_SendChar(char c) {
    UARTCharPut(UART0_BASE, c);
}


void UART_SendString(const char *str) {
    while (*str) {
        UART_SendChar(*(str++));
    }
}
