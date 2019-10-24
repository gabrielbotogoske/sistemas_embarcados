//ARIADNE CAMILLE CARICAS PIASSON
//GABRIEL BOTOGOSKE
//EQUIPE 07

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
#include  "driverlib/timer.h"


#include "system_TM4C1294.h" 

extern void UARTStdioIntHandler(void);
extern void TIMER2A_Handler(void);

int a=3;
uint32_t tempo[3];
float freq_system=120e6;
int start=0;
int sinal=0;

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

void TIMER2A_Handler(void)
{ 
    a--;
    if(a>=0)
    {
      TimerIntClear(TIMER2_BASE,TIMER_CAPA_EVENT);
      tempo[2-a]= TimerValueGet(TIMER2_BASE,TIMER_A); //TIMER2->TAR;
    }
    else
    {
      start=0;
      TimerIntDisable(TIMER2_BASE,TIMER_CAPA_EVENT);
    }
    if(a==2)
    {
      sinal=GPIOPinRead(GPIO_PORTA_BASE,GPIO_PIN_4);
    }
}

void main(void)
{
  
  UARTInit();
  
  //UARTprintf("v[0]=%d v[1]=%d v[2]=%d\n",tempo[0],tempo[1],tempo[2]);
  //UARTprintf("f=%d \n",(int)freq_system);
  
// Enable the Timer0 peripheral
SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);

// Wait for the Timer0 module to be ready.
while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER2))
{
}

// SOURCE CLOCK
TimerClockSourceSet(TIMER2_BASE,TIMER_CLOCK_SYSTEM);
  
// Configure TimerA as a timer-edge up
TimerConfigure(TIMER2_BASE, (TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_TIME_UP));

 
// Set the count time for the the one-shot timer (TimerA).
TimerLoadSet(TIMER2_BASE, TIMER_A, 0xFFFF);

// Configure the counter (TimerA) to count both edges.
TimerControlEvent(TIMER2_BASE, TIMER_A, TIMER_EVENT_BOTH_EDGES);


// Enable the timers.
TimerEnable(TIMER2_BASE, TIMER_A);

// habilitar interrupção
  




  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA); //PA4
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA)); 
  
//CONFIGURA O PINO4 PARA TIMER 
GPIOPinConfigure(GPIO_PA4_T2CCP0);
GPIOPinTypeTimer(GPIO_PORTA_BASE,GPIO_PIN_4);
  float f=0.0;
  float duty_cycle=0.0;
  a=3;
  start=1;
  int i=10;
  int cont=120e4;
  
  TimerIntEnable(TIMER2_BASE,TIMER_CAPA_EVENT);
  TimerIntClear(TIMER2_BASE,TIMER_CAPA_EVENT);
  TimerIntRegister(TIMER2_BASE, TIMER_A, TIMER2A_Handler );
  
  while(i>0)
  {  
    if(start==0)
    {
      start=-1;
      if(tempo[1]<tempo[0])
      {
        tempo[1]+=0xFFFF;
        tempo[2]+=0xFFFF;
      }
      if(tempo[2]<tempo[1])
      {
        tempo[2]+=0xFFFF;
      }
      f=freq_system/(tempo[2]-tempo[0]);
      duty_cycle=(float)((float)(tempo[1]-tempo[0])/(float)((tempo[2]-tempo[0])));
      if (sinal==0)
      {
        duty_cycle=1-duty_cycle;
      }
      UARTprintf("\nv[0]=%d v[1]=%d v[2]=%d\t",tempo[0],tempo[1],tempo[2]);
      UARTprintf("f=%d Hz \t",(int)(f));
      UARTprintf("d_c=%d/100  \n",(int)(duty_cycle*100));
      i--;
      cont=120e4;
      a=3;
      start=1;
      TimerIntEnable(TIMER2_BASE,TIMER_CAPA_EVENT);
      TimerIntClear(TIMER2_BASE,TIMER_CAPA_EVENT);
    }
    cont--;
    if(cont==0)
    {
      UARTprintf("f=0 Hz \t");
      if(GPIOPinRead(GPIO_PORTA_BASE,GPIO_PIN_4)==0)
      {
          UARTprintf("d_c=0/100  \n");
      }
      else
      {    
          UARTprintf("d_c=100/100  \n");
      }
      break;
    }
  }
  while(1)
  {
  }
} 