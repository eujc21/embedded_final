#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/pin_map.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"


#define SAMPLE_SIZE 40    // Number of samples to store
#define AVERAGE_INTERVAL 1000 // Average every 1000 ms
#define SAMPLE_INTERVAL 100   // Sample every 100 ms

typedef struct {
    uint8_t buffer[SAMPLE_SIZE];
    uint8_t head;
    uint8_t count;
} CircularBuffer;

typedef enum {
    LED_STATE_OFF = 0,
    LED_STATE_1,
    LED_STATE_2,
    LED_STATE_3,
    LED_STATE_4,
    LED_STATE_MAX
} LED_State;

volatile LED_State currentState = LED_STATE_OFF;
char uartBuffer[32]; // Buffer for incoming UART data
int uartIndex = 0;   // Current position in the buffer
bool messageReady = false; // Flag to indicate a full message is ready

// Function Prototypes
void GPIO_Init(void);
void UART_Init(void);
void UART_SendChar(char c);
void UART_SendString(const char *str);
void UART1_Init(void);
void UART5_Init(void);
void Timer_Init(void);
void Timer0IntHandler(void);
uint8_t Read_GPIO_Input(void);
// Global Variables
volatile uint32_t elapsed_time = 0;
CircularBuffer cb;
volatile uint32_t readInputsCounter = 0;
volatile uint32_t averageInputsCounter = 0;

int main(void) {
    CircularBuffer_Init(&cb);

    // Set up the system clock to 50 MHz
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    // Initialize peripherals
    LED_Init();
    GPIO_Init();
//    UART_Init();
    UART1_Init();
    UART5_Init();
    SysTick_Init();

    // Enable global interrupts
    IntMasterEnable();

    // Main loop remains idle; interrupts handle functionality
    while (1) {
//       ProcessPayload();
//       UpdateLEDs();
//        UART1_SendMessage();
        TestLEDs();
//        TestLEDLogic();
   }
}




void SysTick_Handler(void) {
    static uint32_t elapsed_time = 0;

    // Increment elapsed time
    elapsed_time += SAMPLE_INTERVAL;

    // Perform periodic sampling
    uint8_t msb_state = Read_GPIO_Input();
    CircularBuffer_Add(&cb, msb_state);

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
        UART1_SendString("<d>");
        UART1_SendString(result_str);
        UART1_SendString("</d>\r\n");

        elapsed_time = 0; // Reset elapsed time
    }
}


void SysTick_Init(void) {
    // Disable SysTick during configuration
    SysTickDisable();

    // Set the SysTick period to generate interrupts every 100 ms
    SysTickPeriodSet(4000000);

    // Register the SysTick handler
    SysTickIntRegister(SysTick_Handler);

    // Enable the SysTick interrupt
    SysTickIntEnable();

    // Enable SysTick
    SysTickEnable();
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
void UART_Handler(void) {
    uint32_t status = UARTIntStatus(UART0_BASE, true); // Get interrupt status
    UARTIntClear(UART0_BASE, status);                 // Clear the interrupt

    // Process incoming data
    while (UARTCharsAvail(UART0_BASE)) {
        char c = UARTCharGet(UART0_BASE);             // Read received character
        UARTCharPut(UART0_BASE, c);                   // Echo back for testing
    }
}

// UART1 Handler
void UART1_Handler(void) {
    // Get the interrupt status
    uint32_t intStatus = UARTIntStatus(UART1_BASE, true);

    // Clear the asserted interrupts
    UARTIntClear(UART1_BASE, intStatus);

    // Handle the received data
    while (UARTCharsAvail(UART1_BASE)) {
        // Read the received character
        char receivedChar = UARTCharGet(UART1_BASE);

        // Echo the character back (for testing purposes)
        UARTCharPut(UART1_BASE, receivedChar);

        // Additional processing can be added here
    }
}

// For Debugging purpose.
void UART1_SendMessage(void) {
    const char *message = "<d>5.0</d>\r\n";
    UART1_SendString(message);

    // Debug message for confirmation
    UART_SendString("UART1 sent: "); // Assuming UART0 is available for debugging
    UART_SendString(message);
}

void UART5_Handler(void) {
    uint32_t intStatus = UARTIntStatus(UART5_BASE, true);
    UARTIntClear(UART5_BASE, intStatus);

    // Read incoming data
    while (UARTCharsAvail(UART5_BASE)) {
        char c = UARTCharGetNonBlocking(UART5_BASE);

        // Store in buffer
        if (c == '\r' || c == '\n') {
            uartBuffer[uartIndex] = '\0'; // Null-terminate the string
            uartIndex = 0;
            messageReady = true; // Flag for processing
        } else if (uartIndex < sizeof(uartBuffer) - 1) {
            uartBuffer[uartIndex++] = c;
        } else {
            // Reset buffer if overflow occurs
            uartIndex = 0;
        }
    }
}

void UART5_Handler_Debug(void) {
    uint32_t intStatus = UARTIntStatus(UART5_BASE, true);
    UARTIntClear(UART5_BASE, intStatus);

    char receivedChar;

    // Read and echo received data
    while (UARTCharsAvail(UART5_BASE)) {
        receivedChar = UARTCharGetNonBlocking(UART5_BASE);

        // Debug message for confirmation
        UART_SendString("UART5 received: "); // Assuming UART0 is available for debugging
        UARTCharPut(UART0_BASE, receivedChar); // Echo to UART0 for monitoring
        UARTCharPut(UART0_BASE, '\n');

        // Echo back to UART5
        UARTCharPut(UART5_BASE, receivedChar);
    }
}

void UART_Init(void) {
    // Enable clock for UART0 and GPIOA
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0) || !SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA)) {}

    // Configure PA0 (U0RX) and PA1 (U0TX) for UART
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Configure UART0 with 115200 baud rate
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    // Enable UART interrupts
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT); // Enable RX and RX timeout interrupts
    UARTIntRegister(UART0_BASE, UART_Handler);           // Register the UART interrupt handler
    IntEnable(INT_UART0);                                // Enable UART0 interrupt in NVIC
}

void UART1_Init(void) {
    // Enable the UART1 and GPIOB peripherals
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    // Wait for the peripherals to be ready
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_UART1) || !SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB)) {}

    if (SysCtlPeripheralReady(SYSCTL_PERIPH_UART1)) {
        UART_SendString("UART1 clock is ready\r\n");
    } else {
        UART_SendString("UART1 clock is NOT ready\r\n");
    }

    if (SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB)) {
        UART_SendString("GPIOB clock is ready\r\n");
    } else {
        UART_SendString("GPIOB clock is NOT ready\r\n");
    }
    // Configure PB0 (U1RX) and PB1 (U1TX) for UART1
    GPIOPinConfigure(GPIO_PB0_U1RX);
    GPIOPinConfigure(GPIO_PB1_U1TX);
    GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Configure UART1 for 115200 baud, 8-N-1 operation
    UARTConfigSetExpClk(UART1_BASE, SysCtlClockGet(), 115200,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    // Enable the UART1 receive interrupt
    UARTIntEnable(UART1_BASE, UART_INT_RX | UART_INT_RT);

    // Register the interrupt handler
    UARTIntRegister(UART1_BASE, UART1_Handler);

    // Enable the interrupt in the NVIC
    IntEnable(INT_UART1);

    // Enable global interrupts
    IntMasterEnable();
}

void UART5_Init(void) {
    // Enable UART5 and GPIOE peripherals
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART5);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);

    // Wait for peripherals to be ready
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_UART5) || !SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE));

    // Configure GPIO pins for UART5
    GPIOPinConfigure(GPIO_PE4_U5RX); // PE4 as UART5 RX
    GPIOPinConfigure(GPIO_PE5_U5TX); // PE5 as UART5 TX
    GPIOPinTypeUART(GPIO_PORTE_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    // Configure UART5 for 115200 baud rate, 8-N-1 format
    UARTConfigSetExpClk(UART5_BASE, SysCtlClockGet(), 115200,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    // Enable UART5 receive interrupts
    UARTIntEnable(UART5_BASE, UART_INT_RX | UART_INT_RT);

    // Register the UART5 interrupt handler
//    UARTIntRegister(UART5_BASE, UART5_Handler_Debug);
    UARTIntRegister(UART5_BASE, UART5_Handler);

    // Enable UART5 interrupt in NVIC
    IntEnable(INT_UART5);

    // Enable global interrupts
    IntMasterEnable();
}


// UART Helper Functions
void UART_SendChar(char c) {
    UARTCharPut(UART0_BASE, c);
}

void UART1_SendChar(char c) {
    UARTCharPut(UART1_BASE, c);
}

void UART1_SendString(const char *str) {
    while (*str) {
        UART1_SendChar(*(str++));
    }
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

void ProcessPayload(void) {
    if (messageReady) {
        messageReady = false;

        // Check for valid payload structure
        if (strncmp(uartBuffer, "<d>", 3) == 0 && strstr(uartBuffer, "</d>")) {
            // Extract the numeric value
            float value = atof(uartBuffer + 3); // Skip "<d>"

            // Determine the state based on the value
            if (value >= 4.0 && value <= 5.0) {
                currentState = LED_STATE_4;
            } else if (value >= 3.0 && value < 4.0) {
                currentState = LED_STATE_3;
            } else if (value >= 2.0 && value < 3.0) {
                currentState = LED_STATE_2;
            } else if (value >= 1.0 && value < 2.0) {
                currentState = LED_STATE_1;
            } else {
                currentState = LED_STATE_OFF;
            }
        }
    }
}

void UpdateLEDs(void) {
    switch (currentState) {
        case LED_STATE_OFF:
            RedLED_Off();
            GreenLED_Off();
            WhiteLED_Off();
            break;
        case LED_STATE_1:
            RedLED_On();
            GreenLED_Off();
            WhiteLED_Off();
            break;
        case LED_STATE_2:
            RedLED_Off();
            GreenLED_On();
            WhiteLED_Off();
            break;
        case LED_STATE_3:
            RedLED_Off();
            GreenLED_Off();
            WhiteLED_On();
            break;
        case LED_STATE_4:
            RedLED_On();
            GreenLED_On();
            WhiteLED_On();
            break;
        default:
            break;
    }
}

void TestLEDLogic(void) {
    LED_State testStates[] = {
        LED_STATE_OFF,
        LED_STATE_1,
        LED_STATE_2,
        LED_STATE_3,
        LED_STATE_4
    };
    int i = 0;
    for (i; i < sizeof(testStates) / sizeof(LED_State); i++) {
        currentState = testStates[i];
        UpdateLEDs();

        // Add a delay to observe the LED state changes
        SysCtlDelay(SysCtlClockGet() / 3);
    }
}

void TestLEDs(void) {
    // Turn on Red LED
    RedLED_On();
    SysCtlDelay(SysCtlClockGet() / 3);

    // Turn on Green LED
    GreenLED_On();
    SysCtlDelay(SysCtlClockGet() / 3);

    // Turn on White LED
    WhiteLED_On();
    SysCtlDelay(SysCtlClockGet() / 3);

    // Turn off all LEDs
    RedLED_Off();
    GreenLED_Off();
    WhiteLED_Off();
    SysCtlDelay(SysCtlClockGet() / 3);
}

