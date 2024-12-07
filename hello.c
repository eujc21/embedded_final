#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/pin_map.h"
#include <stdint.h>
#include <stdbool.h>


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
void UART1_Init(void);
void UART5_Init(void);
uint8_t Read_GPIO_Input(void);

int main(void) {
    CircularBuffer cb;
    CircularBuffer_Init(&cb);

    // Set up the system clock to 50 MHz
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    // Initialize GPIO and UART
    GPIO_Init();
    UART_Init();
    UART1_Init();
    UART5_Init();

    uint32_t elapsed_time = 0;

    while (1) {
        // Read the state of the DIP switch and get the most significant bit
        uint8_t msb_state = Read_GPIO_Input();

        // Add the MSB state to the circular buffer
        CircularBuffer_Add(&cb, msb_state);

        // Delay for the sample interval (100 ms)
        Delay(SAMPLE_INTERVAL);
        elapsed_time += SAMPLE_INTERVAL;

        // Send the MSB state to UART in hexadecimal format
        UART_SendString("Most Significant Bit: 0x");
        UARTCharPut(UART0_BASE, "0123456789ABCDEF"[(msb_state >> 4) & 0x0F]); // High nibble
        UARTCharPut(UART0_BASE, "0123456789ABCDEF"[msb_state & 0x0F]);       // Low nibble
        UART_SendString("\r\n");

        // Every 1000 ms, calculate and process the average
        if (elapsed_time >= AVERAGE_INTERVAL) {
            // Calculate the average times 10, rounded up
            int avg_times10 = Calculate_Average_Times10_RoundedUp(&cb);

            // Extract integer and fractional parts
            int integer_part = avg_times10 / 10;
            int fractional_part = avg_times10 % 10;

            // Build the result string manually
            char result_str[16];
            int index = 0;

            // Convert integer part to string
            if (integer_part >= 10) {
                result_str[index++] = '0' + (integer_part / 10);
            }
            result_str[index++] = '0' + (integer_part % 10);

            result_str[index++] = '.'; // Decimal point

            // Convert fractional part to string
            result_str[index++] = '0' + fractional_part;

            result_str[index] = '\0'; // Null-terminate the string

            // Send the average value in XML format over UART
            UART_SendString("<d>");
            UART_SendString(result_str);
            UART_SendString("</d>\r\n");

            elapsed_time = 0; // Reset elapsed time
        }

        // Additional delay for stability (~1 second)
        SysCtlDelay(SysCtlClockGet() / 10);
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
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_3 | GPIO_PIN_4);
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

    // Read PB3 and map to bit 1 (b1)
    uint8_t pb3_value = (GPIOPinRead(GPIO_PORTB_BASE, GPIO_PIN_3) >> 3) & 0x01;
    value |= (pb3_value << 0);

    // Read PC4 and map to bit 2 (b2)
    uint8_t pc4_value = (GPIOPinRead(GPIO_PORTC_BASE, GPIO_PIN_4) >> 4) & 0x01;
    value |= (pc4_value << 1);

    // Read PC5 and map to bit 3 (b3)
    uint8_t pc5_value = (GPIOPinRead(GPIO_PORTC_BASE, GPIO_PIN_5) >> 5) & 0x01;
    value |= (pc5_value << 2);

    // Read PC6 and map to bit 4 (b4)
    uint8_t pc6_value = (GPIOPinRead(GPIO_PORTC_BASE, GPIO_PIN_6) >> 6) & 0x01;
    value |= (pc6_value << 3);

    // Read PC7 and map to bit 5 (b5)
    uint8_t pc7_value = (GPIOPinRead(GPIO_PORTC_BASE, GPIO_PIN_7) >> 7) & 0x01;
    value |= (pc7_value << 4);

    // Read PD6 and map to bit 6 (b6)
    uint8_t pd6_value = (GPIOPinRead(GPIO_PORTD_BASE, GPIO_PIN_6) >> 6) & 0x01;
    value |= (pd6_value << 5);

    // Read PF3 and map to bit 7 (b7)
    uint8_t pf3_value = (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_3) >> 3) & 0x01;
    value |= (pf3_value << 6);

    // Read PF4 and map to bit 8 (b8)
    uint8_t pf4_value = (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) >> 4) & 0x01;
    value |= (pf4_value << 7);

    // Return the most significant bit (MSB) for further processing
    return Get_Most_Significant_Bit(value);
}


void CircularBuffer_Init(CircularBuffer *cb) {
    cb->head = 0;
    cb->count = 0;
}

void CircularBuffer_Add(CircularBuffer *cb, uint8_t value) {
    cb->head = (cb->head + 1) % SAMPLE_SIZE;
    cb->buffer[cb->head] = value;
    if (cb->count < SAMPLE_SIZE) {
        cb->count++;
    }
}

int Calculate_Average_Times10_RoundedUp(CircularBuffer *cb) {
    uint32_t sum = 0;

    if (cb->count == 0) {
        return 0;
    }

    uint8_t index = cb->head;
    uint8_t i = 0;
    for (i; i < cb->count; i++) {
        if (index == 0) {
            index = SAMPLE_SIZE - 1;
        } else {
            index--;
        }
        sum += cb->buffer[index];
    }

    // Multiply sum by 10 to preserve one decimal place, add (count - 1) for rounding up
    return (int)((sum * 10 + cb->count - 1) / cb->count);
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

void UART5_Init(void) {
    // Enable the UART1 and GPIOE peripherals
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART5);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);

    // Wait for the peripherals to be ready
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_UART5) || !SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE));

    // Configure PE4 (U1RX) and PE5 (U1TX) for UART
    GPIOPinConfigure(GPIO_PE4_U5RX);
    GPIOPinConfigure(GPIO_PE5_U5TX);
    GPIOPinTypeUART(GPIO_PORTE_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    // Configure UART5 for 115200 baud rate
    UARTConfigSetExpClk(UART5_BASE, SysCtlClockGet(), 115200,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
}

void UART1_Init(void) {
    // Enable the UART5 and GPIOB peripherals
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    // Wait for the peripherals to be ready
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_UART1) || !SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB));

    // Configure PB0 (U1RX) and PB1 (U1TX) for UART
    GPIOPinConfigure(GPIO_PB0_U1RX);
    GPIOPinConfigure(GPIO_PB1_U1TX);
    GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Configure UART5 for 115200 baud rate
    UARTConfigSetExpClk(UART5_BASE, SysCtlClockGet(), 115200,
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

void RedLED_On(void) {
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2, GPIO_PIN_2); // Set PB2 high
}

void RedLED_Off(void) {
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2, 0); // Set PB2 low
}

void GreenLED_On(void) {
    GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0, GPIO_PIN_0); // Set PE0 high
}

void GreenLED_Off(void) {
    GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0, 0); // Set PE0 low
}

void WhiteLED_On(void) {
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_PIN_0); // Set PF0 high
}

void WhiteLED_Off(void) {
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 0); // Set PF0 low
}

void LED_Init(void) {
    // Enable GPIO peripherals for the LEDs
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB); // PB2 (Red LED)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE); // PE0 (Green LED)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); // PF0 (White LED)

    // Wait for the peripherals to be ready
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB) ||
           !SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE) ||
           !SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));

    // Configure pins as outputs
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_2); // PB2 (Red LED)
    GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_0); // PE0 (Green LED)
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0); // PF0 (White LED)
}

