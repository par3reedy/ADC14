//Patrick Reedy and Harshal Acharya; May 14, 2021

/*--------------------------------------------------------------------------------------------------------------------------------------
*  Library Definitions
--------------------------------------------------------------------------------------------------------------------------------------*/
#include "msp.h"
#include "DCO.h"
#include "UART.h"
#include "ADC14.h"

/*--------------------------------------------------------------------------------------------------------------------------------------
*  Function Definitions
--------------------------------------------------------------------------------------------------------------------------------------*/
void UART_init(void);
void UART_print(uint8_t cha);
void set_DCO(uint32_t freq);
void UART_print_string(char string[]);
void UART_esc_code(char string[]);
void ADC14_init(void);
uint32_t find_min_array(uint32_t calculation[]);
uint32_t find_max_array(uint32_t calculation[]);

/*--------------------------------------------------------------------------------------------------------------------------------------
*  Global Variables
--------------------------------------------------------------------------------------------------------------------------------------*/
volatile int16_t sample[SAMPLES];
volatile uint32_t count = 0;
uint32_t minimum, maximum, average, calculation[SAMPLES], num;
char min_string[MICROVOLTS], max_string[MICROVOLTS], avg_string[MICROVOLTS];

/*--------------------------------------------------------------------------------------------------------------------------------------
*  Main
--------------------------------------------------------------------------------------------------------------------------------------*/
void main(void)
{
    int32_t i = 0;
	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;		    //Stop watchdog timer
    set_DCO(FREQ_24_MHZ);                               //Set SMCLK to 8MHz, HSMCLK to 24MHz
    UART_init();                                        //Initialize the UART
	ADC14_init();                                       //Initialize the ADC14
	__enable_irq();                                     //Enable global Interrupts
	NVIC->ISER[0] = 1 << ((ADC14_IRQn) & 31);           //NVIC enable for ADC14 Interrupts
	ADC14->IER0 |= ADC14_IER0_IE0;                      //Enable conversion complete interrupt
	UART_esc_code("[1;0H");                             //Move cursor the 1st row and 0th column
	UART_print_string("Minimum: ");                     //Print the string to the terminal
	UART_esc_code("[2;0H");
	UART_print_string("Maximum: ");
	UART_esc_code("[3;0H");
	UART_print_string("Average: ");
	while(1) {                                          //Run forever
	    count = 0;                                      //Reset the count (for another 20 samples)
        while(count < SAMPLES) {                        //Keep running for 20 Samples
            ADC14->CTL0 |= (ADC14_CTL0_ENC              //Enable Conversion
                          | ADC14_CTL0_SC);             //Start the sample/conversion Process
        }
        for (i=0;i<SAMPLES;i++) {                       //Run for the amount of samples
            calculation[i] = (sample[i]*2014-32742)/10; //Voltage optimization in micro volts for each sample taken
        }
        minimum = find_min_array(calculation);          //Find the minimum value within the calculation array
        maximum = find_max_array(calculation);          //Find the maximum value within the calculation array
        average = (maximum + minimum)/2;                //Get the average from the min and max valuess
        for (i=MICROVOLTS-1;i>=0;i--) {                 //Start from the beginning of the voltage and convert each digit into an array
            num = minimum % 10;                         //Take the lowest digit of the minimum value
            min_string[i] = ASCIIOFFSET+num;            //Add offset to make the number an ASCII number character
            minimum /= 10;                              //Divide minimum by 10 to eliminate already processed digit
        }
        for (i=MICROVOLTS-1;i>=0;i--) {
            num = maximum % 10;
            max_string[i] = ASCIIOFFSET+num;
            maximum /= 10;
        }
        for (i=MICROVOLTS-1;i>=0;i--) {
            num = average % 10;
            avg_string[i] = ASCIIOFFSET+num;
            average /= 10;
        }
        UART_esc_code("[1;10H");                        //Move cursor to the 1st row and 10th column (b/c print for reference of value)
        for (i=0;i<TWODEC;i++) {                        //Run until voltage is with percision of 2 decimal values (i.e. 1.00 V)
            UART_print(min_string[i]);                  //Print the current voltage digit
            if (i == 0) {                               //After the first digit is printed
                UART_print('.');                        //Print the decimal point
            }
            else if (i == 2) {                          //After all digits have been printed
                UART_print('V');                        //Print the units
            }
        }
        UART_esc_code("[2;10H");
        for (i=0;i<TWODEC;i++) {
            UART_print(max_string[i]);
            if (i == 0) {
                UART_print('.');
            }
            else if (i == 2) {
                UART_print('V');
            }
        }
        UART_esc_code("[3;10H");
        for (i=0;i<TWODEC;i++) {
            UART_print(avg_string[i]);
            if (i == 0) {
                UART_print('.');
            }
            else if (i == 2) {
                UART_print('V');
            }
        }
	}
}

void ADC14_IRQHandler(void) {
    if (ADC14->IFGR0 & ADC14_IFGR0_IFG0) {              //Check to see if its the MEM interrupt flag for end of conversion
        sample[count] = ADC14->MEM[0];                  //Save the value of the current sample/conversion
        count++;                                        //Increment the count to get next value of array
    }
}
