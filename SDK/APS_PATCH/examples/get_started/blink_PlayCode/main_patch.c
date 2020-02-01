/******************************************************************************
*  Copyright 2017 - 2018, Opulinks Technology Ltd.
*  ----------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Opulinks Technology Ltd. (C) 2018
******************************************************************************/

/******************************************************************************
*  Filename:
*  ---------
*  main_patch.c
*
*  Project:
*  --------
*  OPL1000 Project - the main patch implement file
*
*  Description:
*  ------------
*  This implement file is include the main patch function and api.
*
*  Author:
*  -------
*  SDK Team
*
******************************************************************************/
/***********************
Head Block of The File
***********************/
// Sec 0: Comment block of the file


// Sec 1: Include File
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "sys_init.h"
#include "sys_init_patch.h"
#include "hal_system.h"
#include "mw_fim.h"
#include "cmsis_os.h"
#include "sys_os_config.h"
#include "hal_pin.h"
#include "hal_pin_def.h"
#include "hal_pin_config_project.h"
#include "at_cmd_common_patch.h"

#include "hal_dbg_uart.h"
#include "hal_vic.h"
#include "boot_sequence.h"

// Sec 2: Constant Definitions, Imported Symbols, miscellaneous
#define TIME_MINUTE 1000

/********************************************
Declaration of data structure
********************************************/
// Sec 3: structure, uniou, enum, linked list
typedef enum {
	PREPARE_TO_LEAVE = 0,
	GOING_TO_STATION,
	WAIT_FOR_BUS,
	ON_BUS_IDLE,
} person_state_t;

/********************************************
Declaration of Global Variables & Functions
********************************************/
// Sec 4: declaration of global variable


// Sec 5: declaration of global function prototype


/***************************************************
Declaration of static Global Variables & Functions
***************************************************/
// Sec 6: declaration of static global variable
static osSemaphoreId g_tAppSemaphoreId_Person;
static osSemaphoreId g_tAppSemaphoreId_Dispatch;
static osTimerId g_tAppTimerId_Person;
static osTimerId g_tAppTimerId_Dispatch;
static E_IO01_UART_MODE g_eAppIO01UartMode;
static uint16_t g_bus_location_m = 0;

// Sec 7: declaration of static function prototype
void __Patch_EntryPoint(void) __attribute__((section("ENTRY_POINT")));
void __Patch_EntryPoint(void) __attribute__((used));
static void Main_PinMuxUpdate(void);
static void Main_FlashLayoutUpdate(void);
static void Main_AppInit_patch(void);
static void Main_AppThread_Bus(void *argu);
static void Main_AppThread_Person(void *argu);
static void Main_AppTimer_Clock(void const *argu);
static void Main_AppTimer_Dispatch(void const *argu);
static void Main_MiscDriverConfigSetup(void);
static void Main_AtUartDbgUartSwitch(void);

static void Main_ApsUartRxDectecConfig(void);
static void Main_ApsUartRxDectecCb(E_GpioIdx_t tGpioIdx);

/***********
C Functions
***********/
// Sec 8: C Functions

/*************************************************************************
* FUNCTION:
*   __Patch_EntryPoint
*
* DESCRIPTION:
*   the entry point of SW patch
*
* PARAMETERS
*   none
*
* RETURNS
*   none
*
*************************************************************************/
void __Patch_EntryPoint(void)
{
    // don't remove this code
    SysInit_EntryPoint();
    
    // update the pin mux
    Hal_SysPinMuxAppInit = Main_PinMuxUpdate;
    
    // update the flash layout
    MwFim_FlashLayoutUpdate = Main_FlashLayoutUpdate;
    Sys_MiscDriverConfigSetup = Main_MiscDriverConfigSetup;
    
    // update the switch AT UART / dbg UART function
    at_cmd_switch_uart1_dbguart = Main_AtUartDbgUartSwitch;
	
    Sys_SetUnsuedSramEndBound(0x440000);
    // application init
    Sys_AppInit = Main_AppInit_patch;
}

/*************************************************************************
* FUNCTION:
*   Main_MiscDriverConfigSetup
*
* DESCRIPTION:
*   the initial of driver part for cold and warm boot
*
* PARAMETERS
*   none
*
* RETURNS
*   none
*
*************************************************************************/
static void Main_MiscDriverConfigSetup(void)
{
    //Hal_Wdt_Stop();   //disable watchdog here.

    // IO 1 : detect the GPIO high level if APS UART Rx is connected to another UART Tx port.
    // cold boot
    if (0 == Boot_CheckWarmBoot())
    {
        Hal_DbgUart_RxIntEn(0);
        
        if (HAL_PIN_TYPE_IO_1 == PIN_TYPE_UART_APS_RX)
        {
            Main_ApsUartRxDectecConfig();
        }
    }
}


/*************************************************************************
* FUNCTION:
*   Main_AtUartDbgUartSwitch
*
* DESCRIPTION:
*   switch the UART1 and dbg UART
*
* PARAMETERS
*   none
*
* RETURNS
*   none
*
*************************************************************************/
static void Main_AtUartDbgUartSwitch(void)
{
    if (g_eAppIO01UartMode == IO01_UART_MODE_AT)
    {
        Hal_Pin_ConfigSet(0, PIN_TYPE_UART_APS_TX, PIN_DRIVING_FLOAT);
        Hal_Pin_ConfigSet(1, PIN_TYPE_UART_APS_RX, PIN_DRIVING_LOW);

        Hal_Pin_ConfigSet(8, PIN_TYPE_UART1_TX, PIN_DRIVING_FLOAT);
        Hal_Pin_ConfigSet(9, PIN_TYPE_UART1_RX, PIN_DRIVING_HIGH);

        Hal_DbgUart_RxIntEn(1);
    }
    else
    {
        Hal_DbgUart_RxIntEn(0);

        Hal_Pin_ConfigSet(0, PIN_TYPE_UART1_TX, PIN_DRIVING_FLOAT);
        Hal_Pin_ConfigSet(1, PIN_TYPE_UART1_RX, PIN_DRIVING_LOW);
        
        Hal_Pin_ConfigSet(8, PIN_TYPE_UART_APS_TX, PIN_DRIVING_FLOAT);
        Hal_Pin_ConfigSet(9, PIN_TYPE_UART_APS_RX, PIN_DRIVING_HIGH);
    }
    
    g_eAppIO01UartMode = (E_IO01_UART_MODE)!g_eAppIO01UartMode;
}


/*************************************************************************
* FUNCTION:
*   Main_ApsUartRxDectecConfig
*
* DESCRIPTION:
*   detect the GPIO high level if APS UART Rx is connected to another UART Tx port.
*
* PARAMETERS
*   none
*
* RETURNS
*   none
*
*************************************************************************/
static void Main_ApsUartRxDectecConfig(void)
{
    E_GpioLevel_t eGpioLevel;

    Hal_Pin_ConfigSet(1, PIN_TYPE_GPIO_INPUT, PIN_DRIVING_LOW);
    eGpioLevel = Hal_Vic_GpioInput(GPIO_IDX_01);
    if (GPIO_LEVEL_HIGH == eGpioLevel)
    {
        // it is connected
        Hal_Pin_ConfigSet(1, HAL_PIN_TYPE_IO_1, HAL_PIN_DRIVING_IO_1);
        Hal_DbgUart_RxIntEn(1);
    }
    else //if (GPIO_LEVEL_LOW == eGpioLevel)
    {
        // it is not conncected, set the high level to trigger the GPIO interrupt
        Hal_Vic_GpioCallBackFuncSet(GPIO_IDX_01, Main_ApsUartRxDectecCb);
        //Hal_Vic_GpioDirection(GPIO_IDX_01, GPIO_INPUT);
        Hal_Vic_GpioIntTypeSel(GPIO_IDX_01, INT_TYPE_LEVEL);
        Hal_Vic_GpioIntInv(GPIO_IDX_01, 0);
        Hal_Vic_GpioIntMask(GPIO_IDX_01, 0);
        Hal_Vic_GpioIntEn(GPIO_IDX_01, 1);
    }
}

/*************************************************************************
* FUNCTION:
*   Main_ApsUartRxDectecCb
*
* DESCRIPTION:
*   detect the GPIO high level if APS UART Rx is connected to another UART Tx port.
*
* PARAMETERS
*   1. tGpioIdx : Index of call-back GPIO
*
* RETURNS
*   none
*
*************************************************************************/
static void Main_ApsUartRxDectecCb(E_GpioIdx_t tGpioIdx)
{
    // disable the GPIO interrupt
    Hal_Vic_GpioIntEn(GPIO_IDX_01, 0);

    // it it connected
    Hal_Pin_ConfigSet(1, HAL_PIN_TYPE_IO_1, HAL_PIN_DRIVING_IO_1);
    Hal_DbgUart_RxIntEn(1);
}

/*************************************************************************
* FUNCTION:
*   Main_PinMuxUpdate
*
* DESCRIPTION:
*   update the flash layout
*
* PARAMETERS
*   none
*
* RETURNS
*   none
*
*************************************************************************/
static void Main_PinMuxUpdate(void)
{
    Hal_Pin_ConfigSet(0, HAL_PIN_TYPE_IO_0, HAL_PIN_DRIVING_IO_0);
    Hal_Pin_ConfigSet(1, HAL_PIN_TYPE_IO_1, HAL_PIN_DRIVING_IO_1);
    Hal_Pin_ConfigSet(2, HAL_PIN_TYPE_IO_2, HAL_PIN_DRIVING_IO_2);
    Hal_Pin_ConfigSet(3, HAL_PIN_TYPE_IO_3, HAL_PIN_DRIVING_IO_3);
    Hal_Pin_ConfigSet(4, HAL_PIN_TYPE_IO_4, HAL_PIN_DRIVING_IO_4);
    Hal_Pin_ConfigSet(5, HAL_PIN_TYPE_IO_5, HAL_PIN_DRIVING_IO_5);
    Hal_Pin_ConfigSet(6, HAL_PIN_TYPE_IO_6, HAL_PIN_DRIVING_IO_6);
    Hal_Pin_ConfigSet(7, HAL_PIN_TYPE_IO_7, HAL_PIN_DRIVING_IO_7);
    Hal_Pin_ConfigSet(8, HAL_PIN_TYPE_IO_8, HAL_PIN_DRIVING_IO_8);
    Hal_Pin_ConfigSet(9, HAL_PIN_TYPE_IO_9, HAL_PIN_DRIVING_IO_9);
    Hal_Pin_ConfigSet(10, HAL_PIN_TYPE_IO_10, HAL_PIN_DRIVING_IO_10);
    Hal_Pin_ConfigSet(11, HAL_PIN_TYPE_IO_11, HAL_PIN_DRIVING_IO_11);
    Hal_Pin_ConfigSet(12, HAL_PIN_TYPE_IO_12, HAL_PIN_DRIVING_IO_12);
    Hal_Pin_ConfigSet(13, HAL_PIN_TYPE_IO_13, HAL_PIN_DRIVING_IO_13);
    Hal_Pin_ConfigSet(14, HAL_PIN_TYPE_IO_14, HAL_PIN_DRIVING_IO_14);
    Hal_Pin_ConfigSet(15, HAL_PIN_TYPE_IO_15, HAL_PIN_DRIVING_IO_15);
    Hal_Pin_ConfigSet(16, HAL_PIN_TYPE_IO_16, HAL_PIN_DRIVING_IO_16);
    Hal_Pin_ConfigSet(17, HAL_PIN_TYPE_IO_17, HAL_PIN_DRIVING_IO_17);
    Hal_Pin_ConfigSet(18, HAL_PIN_TYPE_IO_18, HAL_PIN_DRIVING_IO_18);
    Hal_Pin_ConfigSet(19, HAL_PIN_TYPE_IO_19, HAL_PIN_DRIVING_IO_19);
    Hal_Pin_ConfigSet(20, HAL_PIN_TYPE_IO_20, HAL_PIN_DRIVING_IO_20);
    Hal_Pin_ConfigSet(21, HAL_PIN_TYPE_IO_21, HAL_PIN_DRIVING_IO_21);
    Hal_Pin_ConfigSet(22, HAL_PIN_TYPE_IO_22, HAL_PIN_DRIVING_IO_22);
    Hal_Pin_ConfigSet(23, HAL_PIN_TYPE_IO_23, HAL_PIN_DRIVING_IO_23);
    
    at_io01_uart_mode_set(HAL_PIN_0_1_UART_MODE);
}

/*************************************************************************
* FUNCTION:
*   Main_FlashLayoutUpdate
*
* DESCRIPTION:
*   update the flash layout
*
* PARAMETERS
*   none
*
* RETURNS
*   none
*
*************************************************************************/
static void Main_FlashLayoutUpdate(void)
{
    // update here
}

/*************************************************************************
* FUNCTION:
*   Main_AppInit_patch
*
* DESCRIPTION:
*   the initial of application
*
* PARAMETERS
*   none
*
* RETURNS
*   none
*
*************************************************************************/
static void Main_AppInit_patch(void)
{
    osThreadDef_t tThreadDef;
    osSemaphoreDef_t tSemaphoreDef;
    osTimerDef_t tTimerDef_Clock, tTimerDef_Dispatch;
    
    // create the thread for AppThread_Bus
    tThreadDef.name = "App_Bus";
    tThreadDef.pthread = Main_AppThread_Bus;
    tThreadDef.tpriority = OS_TASK_PRIORITY_APP;        // osPriorityNormal
    tThreadDef.instances = 0;                           // reserved, it is no used
    tThreadDef.stacksize = OS_TASK_STACK_SIZE_APP;      // (512), unit: 4-byte, the size is 512*4 bytes
    osThreadCreate(&tThreadDef, NULL);
    
    // create the thread for AppThread_Person
    tThreadDef.name = "App_Person";
    tThreadDef.pthread = Main_AppThread_Person;
    tThreadDef.tpriority = OS_TASK_PRIORITY_APP;        // osPriorityNormal
    tThreadDef.instances = 0;                           // reserved, it is no used
    tThreadDef.stacksize = OS_TASK_STACK_SIZE_APP;      // (512), unit: 4-byte, the size is 512*4 bytes
    osThreadCreate(&tThreadDef, NULL);
    	
    // create the semaphore
    g_tAppSemaphoreId_Person = osSemaphoreCreate(&tSemaphoreDef, 1);
	g_tAppSemaphoreId_Dispatch = osSemaphoreCreate(&tSemaphoreDef, 1);

    // create the timer
    tTimerDef_Clock.ptimer = Main_AppTimer_Clock;
    g_tAppTimerId_Person = osTimerCreate(&tTimerDef_Clock, osTimerOnce, NULL);
	
	tTimerDef_Dispatch.ptimer = Main_AppTimer_Dispatch;
    g_tAppTimerId_Dispatch = osTimerCreate(&tTimerDef_Dispatch, osTimerOnce, NULL);
}

/*************************************************************************
* FUNCTION:
*   Main_AppThread_Bus
*
* DESCRIPTION:
*   the application thread of bus
*
* PARAMETERS
*   1. argu     : [In] the input argument
*
* RETURNS
*   none
*
*************************************************************************/
int wait_for_start_by_io (void)
{
	return Hal_Vic_GpioInput(GPIO_IDX_07);
}
static void Main_AppThread_Bus(void *argu)
{
// TRNG implementation
	while (wait_for_start_by_io())
	{
		osDelay(100);
	}
		
	srand(xTaskGetTickCount());
	uint8_t wait_time_to_dispatch = rand() % 10;
	osSemaphoreWait(g_tAppSemaphoreId_Dispatch, osWaitForever);		//Take the default Semaphore
	
	osTimerStart(g_tAppTimerId_Dispatch, TIME_MINUTE * wait_time_to_dispatch);
	printf("The bus will take off after %d minutes...\r\n", wait_time_to_dispatch);
	
	osSemaphoreWait(g_tAppSemaphoreId_Dispatch, osWaitForever);
	printf("The bus now leave from Home station.\r\n");
	g_bus_location_m = 0;
	//Bus Mission: 20mins 10km, it means 500m/min
	osTimerStart(g_tAppTimerId_Person, TIME_MINUTE * 15);      //15 secs means 15 mins
	
    while (1)
    {
		g_bus_location_m += 500;
		osDelay(TIME_MINUTE);
    }
}

/*************************************************************************
* FUNCTION:
*   Main_AppThread_Person
*
* DESCRIPTION:
*   the application thread of person
*
* PARAMETERS
*   1. argu     : [In] the input argument
*
* RETURNS
*   none
*
*************************************************************************/
static void Main_AppThread_Person(void *argu)
{   
	static person_state_t person_state = PREPARE_TO_LEAVE;
	int16_t distance = 300;
	
	osSemaphoreWait(g_tAppSemaphoreId_Person, osWaitForever);		//Take the default Semaphore
	
    while (1) {
		switch (person_state) {  
			case PREPARE_TO_LEAVE:
				// wait the mutex	
				osSemaphoreWait(g_tAppSemaphoreId_Person, osWaitForever);		//Go outdoors
				printf("[%d]Go Now!!!\r\n", osKernelSysTick());
				person_state = GOING_TO_STATION; 
				osDelay(500);			//It took 30secs(0.5s) to close the door.
			break;

			case GOING_TO_STATION:
				//Distance(300m/5min) --
				distance -= 60;
				if (distance > 0) {
					osDelay(1000);
				} else {
					person_state = WAIT_FOR_BUS;
				}
				break;
			
			case WAIT_FOR_BUS:
				if (g_bus_location_m < 10000) {
					osDelay(1000);
				} else {
					printf("[%d]Get on bus now!!!\r\n", osKernelSysTick());
					person_state = ON_BUS_IDLE;
				}
				break;
				
			case ON_BUS_IDLE:
				osDelay(5000);
			    break;    
		}
    }
}

/*************************************************************************
* FUNCTION:
*   Main_AppTimer_Clock
*
* DESCRIPTION:
*   the timeout function of AppTimer
*
* PARAMETERS
*   1. argu     : [In] the pointer of argument
*
* RETURNS
*   none
*
*************************************************************************/
static void Main_AppTimer_Clock(void const *argu)
{
    // release the semaphore
    osSemaphoreRelease(g_tAppSemaphoreId_Person);
}

static void Main_AppTimer_Dispatch(void const *argu)
{
	osSemaphoreRelease(g_tAppSemaphoreId_Dispatch);
}
