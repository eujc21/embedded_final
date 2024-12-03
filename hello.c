#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/pin_map.h"

#define SAMPLE_SIZE 40    // Number of samples to store
#define AVERAGE_INTERVAL 1000 // Average every 1000 ms
#define SAMPLE_INTERVAL 100   // Sample every 100 ms

typedef struct {
    uint8_t buffer[SAMPLE_SIZE];
    uint8_t head;
    uint8_t count;
} CircularBuffer;

// Function Prototypes
void GPIO_Init(void);
void UART_Init(void);
void UART_SendChar(char c);
void UART_SendString(const char *str);
uint8_t Read_GPIO_Input(void);

// Main Function
int main(void) {
    CircularBuffer cb;
    CircularBuffer_Init(&cb);
    // Set up the system clock to 50 MHz
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    // Initialize GPIO and UART
    GPIO_Init();
    UART_Init();

    uint32_t elapsed_time = 0;

    while (1) {
        // Read the state of the DIP switch
        uint8_t msb_state = Read_GPIO_Input();
        CircularBuffer_Add(&cb, msb_state);
        Delay(SAMPLE_INTERVAL); // 100 ms delay
        elapsed_time += SAMPLE_INTERVAL;

        // Send the state to UART
        UART_SendString("Most Significant Bit: 0x");
        UARTCharPut(UART0_BASE, "0123456789ABCDEF"[(msb_state >> 4) & 0x0F]); // Send high nibble
        UARTCharPut(UART0_BASE, "0123456789ABCDEF"[msb_state & 0x0F]);       // Send low nibble
        UART_SendString("\r\n");


        // Every 1000 ms, calculate and process the average
//        if (elapsed_time >= AVERAGE_INTERVAL) {
//            uint8_t average = Calculate_Average(&cb);
//            UART_SendString("<d>");
//            UARTCharPut(UART0_BASE, "0123456789ABCDEF"[(average >> 4) & 0x0F]); // Send high nibble
//            UARTCharPut(UART0_BASE, "0123456789ABCDEF"[average & 0x0F]);       // Send low nibble
//            UART_SendString("</d>\n");
//
//            elapsed_time = 0; // Reset elapsed time
//        }

        if (elapsed_time >= AVERAGE_INTERVAL) {
            float average = Calculate_Average(&cb);

            // Format and transmit the average
            int integer_part = (int)average;
            int fractional_part = (int)((average - integer_part) * 10);

            UART_SendString("<d>");
            UARTCharPut(UART0_BASE, '0' + integer_part);
            UARTCharPut(UART0_BASE, '.');
            UARTCharPut(UART0_BASE, '0' + fractional_part);
            UART_SendString("</d>\r\n");

            elapsed_time = 0; // Reset elapsed time
        }
        // Delay for stability
        SysCtlDelay(SysCtlClockGet() / 3); // ~1 second delay
    }
}

// GPIO Initialization for DIP Switch
void GPIO_Init(void) {
    // Enable clocks for GPIOB, GPIOC, GPIOD, and GPIOF
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB) || !SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOC) ||
           !SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD) || !SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)) {}

    // Configure PB3 as input
    GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, GPIO_PIN_3);
//    GPIOPadConfigSet(GPIO_PORTB_BASE, GPIO_PIN_3, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    // Configure PC4-PC7 as inputs
    GPIOPinTypeGPIOInput(GPIO_PORTC_BASE, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
//    GPIOPadConfigSet(GPIO_PORTC_BASE, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    // Configure PD6-PD7 as inputs
    GPIOPinTypeGPIOInput(GPIO_PORTD_BASE, GPIO_PIN_6 | GPIO_PIN_7);
//    GPIOPadConfigSet(GPIO_PORTD_BASE, GPIO_PIN_6 | GPIO_PIN_7, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    // Configure PF4 as input
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_4);
//    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
}
uint8_t Get_Most_Significant_Bit(uint8_t value) {
    // Iterate from MSB (bit 7) to LSB (bit 0)
    int i = 7;
    for (i; i >= 0; i--) {
        if (value & (1 << i)) { // Check if bit i is set
            return i + 1; // Return 1-based index of the MSB
        }
    }
    return 0; // Return 0 if no bits are set
}

uint8_t Read_GPIO_Input(void) {
    uint8_t value = 0;

    // Read PB3 and map to bit 1(b1)
    value |= ((GPIOPinRead(GPIO_PORTB_BASE, GPIO_PIN_3) >> 3) & 0x01) << 0;

    // Read PC4-PC7 (b2-b5)
   value |= ((GPIOPinRead(GPIO_PORTC_BASE, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7) >> 4) & 0x0F) << 1;

   // Read PD6-PD7 (b6-b7)
   value |= ((GPIOPinRead(GPIO_PORTD_BASE, GPIO_PIN_6 | GPIO_PIN_7) >> 6) & 0x03) << 5;

   // Read PF4 (b8)
   value |= ((GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) >> 4) & 0x01) << 7;

   return Get_Most_Significant_Bit(value);
}

void CircularBuffer_Init(CircularBuffer *cb) {
    cb->head = 0;
    cb->count = 0;
}

void CircularBuffer_Add(CircularBuffer *cb, uint8_t value) {
    cb->buffer[cb->head] = value;
    cb->head = (cb->head + 1) % SAMPLE_SIZE;
    if (cb->count < SAMPLE_SIZE) {
        cb->count++;
    }
}

uint8_t Calculate_Average(CircularBuffer *cb) {
    uint16_t sum = 0; // Use a 16-bit integer to avoid overflow
    uint8_t i = 0;
    for (i; i < cb->count; i++) {
        sum += cb->buffer[i];
    }
    return (cb->count > 0) ? (uint8_t)(sum / cb->count) : 0; // Return average
}

void Delay(uint32_t ms) {
    uint32_t i = 0;
    for (i; i < (ms * 4000); i++) {
        __asm("NOP"); // Simple delay loop
    }
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
