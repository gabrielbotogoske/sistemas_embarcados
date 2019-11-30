#include <stdint.h>
#include <stdbool.h>

#include "system_tm4c1294.h" // CMSIS-Core
#include "cmsis_os2.h" // CMSIS-RTOS

#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/pin_map.h"
#include "utils/uartstdio.h"



osThreadId_t elevador_esq_id, elevador_cen_id, elevador_dir_id, UART_READ_id, UART_WRITE_id;
osTimerId_t timer_esq_id,timer_cen_id,timer_dir_id;
osMessageQueueId_t fila_esq_id,fila_dir_id,fila_cen_id,fila_envio_id;

extern void UARTStdioIntHandler(void);

typedef enum{parado,subindo,descendo
}estado_elevador;

typedef struct {
  int andar;
  char posicao;
  estado_elevador estado;
  uint16_t paradas;
  uint16_t chamadas;
  osMessageQueueId_t fila;  
}elevador_estrutura;

void callback(void *arg){
  
} 

int letra2andar(char letter)
{
   return (letter-0x61);
}

int numero2andar(char u, char d)
{
      return (d-0x30)*10+(u-0x30);  
}


int lerandar(char u, char d)
{
    if(u=='\0')
    {
      return   (d-0x30);
    }
     return (d-0x30)*10+(u-0x30);  
}

void prencher_parada(elevador_estrutura* e,int andar_desejado)
{
   e->paradas|=0x1<<andar_desejado;
}

void retirar_parada(elevador_estrutura* e)
{
  uint16_t m=0x1<<e->andar;
  m=~m;
  e->paradas&=m;
}

void retirar_chamada(elevador_estrutura* e,int andar)
{
  uint16_t m=0x1<<andar;
  m=~m;
  e->chamadas&=m;
}

bool esvaziou_parada(uint16_t parada)
{
  if (parada == 0)
  {
    return true;
  }
  return false;
}

void prencher_chamada(elevador_estrutura* e,int andar_desejado)
{
   e->chamadas|=0x1<<andar_desejado;
}

bool comparar_andar(int andar_atual, uint16_t lista)
{
    if(((0x1<<andar_atual) & lista) == (0x1<<andar_atual))
    {
      return 1;
    }
    return 0;
}

void resetar(char posicao, osMessageQueueId_t fila)
{
   
    char send[2];
    send[0]=posicao;
    send[1]='r';//resetar
    osMessageQueuePut(fila_envio_id, send, NULL, osWaitForever);

}

void abrir(char posicao, osMessageQueueId_t fila)
{
  
    char send[2];
    send[0]=posicao;
    send[1]='a';//abrir
    osMessageQueuePut(fila_envio_id, send, NULL, osWaitForever);

}

void fechar(char posicao, osMessageQueueId_t fila)
{

    char send[2];
    send[0]=posicao;
    send[1]='f';//fechar
    osMessageQueuePut(fila_envio_id, send, NULL, osWaitForever);

}

void subir(char posicao)
{
  char send[2];
  send[0]=posicao;
  send[1]='s';//subir
  osMessageQueuePut(fila_envio_id, send, NULL, osWaitForever);
}

void descer(char posicao)
{
  char send[2];
  send[0]=posicao;
  send[1]='d';//descer
  osMessageQueuePut(fila_envio_id, send, NULL, osWaitForever);
}

void parar(char posicao)
{
  char send[2];
  send[0]=posicao;
  send[1]='p';//parar
  osMessageQueuePut(fila_envio_id, send, NULL, osWaitForever);
}

void atender_andar(elevador_estrutura e)
{
    parar(e.posicao);
    osDelay(5000);
    abrir(e.posicao,e.fila);
    osDelay(5000);
    fechar(e.posicao,e.fila);
    osDelay(5000);
}

bool chamadas_acima(elevador_estrutura e)
{
  if(e.chamadas>(0x1<<e.andar))
  {
    return true;
  }
  return false;
}

void transferir_ultimo(elevador_estrutura* e)
{
    int i;
    for(i=15;i>=0;i--)
    {
      if((0x1<<i) <= e->chamadas)
      {
        break;
      }
    }
    retirar_chamada(e,i);
    prencher_parada(e,i);
}

bool chamadas_abaixo(elevador_estrutura e)
{
  uint16_t mascara=(0x1<<e.andar+1)-1;
  e.chamadas&=mascara;
  if(e.chamadas<(0x1<<e.andar))
  {
    return true;
  }
  return false;
}

void transferir_primeiro(elevador_estrutura* e)
{
   int i;
    for(i=0;i<=15;i++)
    {
      if(((0x1<<i) & e->chamadas)== (0x1<<i))
      {
        break;
      }
    }
    retirar_chamada(e,i);
    prencher_parada(e,i);
}

void chamadas2paradas(elevador_estrutura* e)
{
  e->paradas=e->chamadas;
  e->chamadas=0;
}

void elevador(void *arg){
  
  elevador_estrutura e;
  e.andar=0;
  e.estado=parado;
  e.posicao=(char)(arg);
  e.paradas=0x00;
  e.chamadas=0x00;
  
  
  if(e.posicao=='e')
  {
    e.fila=fila_esq_id;
  }
  else if(e.posicao=='c')
  {
    e.fila=fila_cen_id;
  }
  else if(e.posicao=='d')
  {
    e.fila=fila_dir_id;
  }
  
  //------------------------------------
  
  int andar_desejado;
  char sentido_desejado;
  
  //---------------------------------------
  
  char msg[10];

  //-------------------------------------------
  resetar(e.posicao,e.fila);
  //osDelay(1000);
  fechar(e.posicao,e.fila);

  //------------------------------------------------
  
  while(1){
    osMessageQueueGet(e.fila, msg ,NULL, osWaitForever);
    if(msg[1]=='E')//botao externo
    {
      andar_desejado=numero2andar(msg[3],msg[2]);
      sentido_desejado=msg[4];
      if(e.estado==parado)
      {
        if(andar_desejado>e.andar)//andar acima
        {
            subir(e.posicao);
            prencher_parada(&e,andar_desejado);
            e.estado=subindo;
        }
        else if(andar_desejado<e.andar)//andar abaixo
        {
            descer(e.posicao);
            prencher_parada(&e,andar_desejado);
            e.estado=descendo;
        }
        else //mesmo andar
        {
            atender_andar(e);
        }
      }
      else if(e.estado==subindo)
      {
          if(sentido_desejado=='s')
          {
              if(andar_desejado>=e.andar)
              {
                  prencher_parada(&e,andar_desejado);
              }
              else
              {
                 prencher_chamada(&e,andar_desejado);
              }
          }
          else //sentido_desejado == 'd'
          {
               prencher_chamada(&e,andar_desejado);
          }
      }
      else //e.estado==descendo
      {
          if(sentido_desejado=='s')
          {
                prencher_chamada(&e,andar_desejado);
          }
          else //sentido_desejado == 'd'
          {
              if(andar_desejado<=e.andar)
              {
                 prencher_parada(&e,andar_desejado);
              }
              else
              {
                  prencher_chamada(&e,andar_desejado);
              }
          }
      }
    }
    else if(msg[1]=='I')//botao interno
    {
      andar_desejado=letra2andar(msg[2]);
       if(e.estado==parado)
      {
          if(andar_desejado>e.andar)//andar acima
          {
              subir(e.posicao);
              prencher_parada(&e,andar_desejado);
              e.estado=subindo;
          }
          else if(andar_desejado<e.andar)//andar abaixo
          {
              descer(e.posicao);
              prencher_parada(&e,andar_desejado);
              e.estado=descendo; 
          }
          else //mesmo andar
          {
              atender_andar(e);
          }
      }
       else if(e.estado==subindo)
      {
           if(andar_desejado>=e.andar)
           {
              prencher_parada(&e,andar_desejado);
           }
           else
           {
             prencher_chamada(&e,andar_desejado);
           }
       }
        else //e.estado==descendo
        {
           if(andar_desejado<=e.andar)
           {
             prencher_parada(&e,andar_desejado);
           }
           else
           {
             prencher_chamada(&e,andar_desejado);
           }      
      }
      
    }
    else if(msg[1]>='0' && msg[1]<='9') //informação  do andar
    {
        e.andar=lerandar(msg[2],msg[1]);
        if(comparar_andar(e.andar, e.paradas))
        {
            atender_andar(e);
            retirar_parada(&e);
            if(esvaziou_parada(e.paradas))//true
            {
                if(esvaziou_parada(e.chamadas))//fica parado
                {
                   parar(e.posicao);
                   e.estado=parado;
                }
                else // mas tem gente esperando
                {
                  if(e.estado==subindo)
                  {
                      if(chamadas_acima(e)) //tem gente mais acima
                      {
                          transferir_ultimo(&e);
                          subir(e.posicao);
                      }
                      else // pode inverter o sentido
                      {
                          chamadas2paradas(&e);
                          descer(e.posicao);
                      }
                  }
                  else if(e.estado==descendo)
                  {
                    if(chamadas_abaixo(e)) //tem gente mais abaixo
                      {
                          transferir_primeiro(&e);
                          descer(e.posicao);
                      }
                      else // pode inverter o sentido
                      {
                          chamadas2paradas(&e);
                          subir(e.posicao);
                      }
                  }
                }
            }
            else//false
            {
                if(e.estado==subindo)//continua subindo
                {
                    subir(e.posicao);
                    
                }
                else if(e.estado==descendo)//continua descendo
                {
                  descer(e.posicao);
                }
            }
        }
        //if a lista de paradas esvaziou
        //--> ficar parado/transferir(parte) da lista de chamadas para a lista de paradas
    }
  } 
} 

void UART_READ(void *arg){
  char msg[6];
  while(1){
    UARTgets(msg,20);
    if(msg[0]=='e')
    {
      osMessageQueuePut(fila_esq_id, msg, NULL, 0);
    }
    else if(msg[0]=='c')
    {
      osMessageQueuePut(fila_cen_id, msg, NULL, 0);
    }
    else if(msg[0]=='d')
    {
      osMessageQueuePut(fila_dir_id, msg, NULL, 0);
    }  
    osThreadYield();
  } 
} 

void UART_WRITE(void *arg){
  
  
  char msg[2];
  while(1){
    osMessageQueueGet(fila_envio_id, msg ,NULL, osWaitForever);
    UARTprintf("%c%c\n\r",msg[0],msg[1]); 
   // osDelay(500);
  } 
} 

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
  UARTStdioConfig(0,115200, SystemCoreClock);
} // UARTInit

void UART0_Handler(void){
  UARTStdioIntHandler();
} // UART0_Handler

void main(void){
  
  UARTInit();
  
  SystemInit();
 
  osKernelInitialize();

  elevador_esq_id = osThreadNew(elevador, (void*)'e', NULL);
  elevador_cen_id = osThreadNew(elevador, (void*)'c', NULL);
  elevador_dir_id = osThreadNew(elevador, (void*)'d', NULL);
 
  UART_READ_id= osThreadNew(UART_READ, NULL, NULL);
  UART_WRITE_id= osThreadNew(UART_WRITE, NULL, NULL);
  
  timer_esq_id = osTimerNew(callback, osTimerOnce , NULL, NULL);
  timer_cen_id = osTimerNew(callback, osTimerOnce , NULL, NULL);
  timer_dir_id = osTimerNew(callback, osTimerOnce , NULL, NULL);
  
  fila_dir_id =  osMessageQueueNew (15, sizeof(char)*6,NULL);
  fila_cen_id =  osMessageQueueNew (15, sizeof(char)*6,NULL); 
  fila_esq_id =  osMessageQueueNew (15, sizeof(char)*6,NULL);
  
  fila_envio_id =  osMessageQueueNew (30, sizeof(char)*3,NULL);
  
  
  
  if(osKernelGetState() == osKernelReady)
    osKernelStart();

  while(1);
} // main