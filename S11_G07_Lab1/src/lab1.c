//Ariadne Camille Caricas Piasson
//Gabriel Botogoske

#include <stdint.h>
#include <stdbool.h>
// includes da biblioteca driverlib
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/pin_map.h"
#include "utils/uartstdio.h"

#include "system_TM4C1294.h" 


#define Nup 14 // linhas assembly
#define Ndown 14 //
#define freq 120e6 

extern void UARTStdioIntHandler(void);


void UARTInit(void){
  // Enable the GPIO Peripheral used by the UART.
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));

  // Enable UART0
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));

  // Configure GPIO Pins for UART mode.
  GPIOPinConfigure(GPIO_PA0_U0RX);
  GPIOPinConfigure(GPIO_PA1_U0TX);
  GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

  // Initialize the UART for console I/O.
  UARTStdioConfig(0, 9600, SystemCoreClock);
} // UARTInit

void UART0_Handler(void){
  UARTStdioIntHandler();
} // UART0_Handler


void main(void)
{
  
  UARTInit();
 
  
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE); 
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE)); 
  
  GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_4);
  
  int contup;
  int contdown;
  
  double Temp=1e9/freq;
  double Temp_up=Nup*Temp;
  double Temp_down=Ndown*Temp;
  double Tup, Tdown, T;
  double dutycicle,f;
  
  int n_medidas=10;
  while(n_medidas>0)
  {
    contup=0;
    contdown=0;
    
    while(GPIOPinRead(GPIO_PORTE_BASE, GPIO_PIN_4)!=GPIO_PIN_4){} //espera a leitura de um nivel alto
    while(GPIOPinRead(GPIO_PORTE_BASE, GPIO_PIN_4)==GPIO_PIN_4){} //espera o nivel alto acabar
    
    do
    {
      contdown++;
    }while(GPIOPinRead(GPIO_PORTE_BASE, GPIO_PIN_4)==0); // conta numero de loops dado quando a entrada está em zero
    do
    {
      contup++;
    }while(GPIOPinRead(GPIO_PORTE_BASE, GPIO_PIN_4)!=0); // conta numero de loops dado quando a entrada está em um
    
    dutycicle=(double)(contup/(double)(contdown+contup));
    Tup=(double)contup*Temp_up;
    Tdown=(double)contdown*Temp_down;
    T=Tup+Tdown;
    f=1/T;

      
    
    UARTprintf("contup=%d    contdown=%d    dutycicle=%d/10000   Tempo=%d(ns)   freq=%dHz\n",contup,contdown,(int)(dutycicle*10000),(int)(T),(int)(f*1e9));
    n_medidas--;
    //UARTprintf("D_C= %d     ",(int)dutycicle*100);
    //UARTprintf("T= %d       ",(int)T);
    //UARTprintf("f= %d\n",(int)f);
 
  }
  
  while(1){};
  
} 