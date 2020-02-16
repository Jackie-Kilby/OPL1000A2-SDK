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
*  SH SDK
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
#include "sys_init.h"
#include "sys_init_patch.h"
#include "hal_system.h"
#include "mw_fim.h"
#include "cmsis_os.h"
#include "sys_os_config.h"
#include "at_cmd_common_patch.h"
#include "hal_pin.h"
#include "hal_pin_def.h"
#include "hal_pin_config_project.h"
#include "msg_patch.h"
#include "msg.h"
#include "hal_dbg_uart.h"
#include "wpa2_station_app.h"
//#include "hal_wdt.h"
#include "hal_vic.h"
#include "boot_sequence.h"

// Sec 2: Constant Definitions, Imported Symbols, miscellaneous
// the number of elements in the message queue

#define  USER_APP1_LOG              0
#define  USER_APP2_LOG              1
#define  ROM_MODULES_NUMBER         8 

/********************************************
Declaration of data structure
********************************************/
// Sec 3: structure, union, enum, linked list

/********************************************
Declaration of Global Variables & Functions
********************************************/
// Sec 4: declaration of global variable

extern T_TracerTaskInfoExt g_taTracerDefIntTaskInfoBody[TRACER_INT_TASK_NUM_MAX];
extern T_TracerTaskInfoExt g_taTracerExtTaskInfoBody[TRACER_EXT_TASK_NUM_MAX];

// Sec 5: declaration of global function prototype
typedef void (*T_Main_AppInit_fp)(void);
extern T_Main_AppInit_fp Main_AppInit;

extern T_TracerLogLevelSetFp tracer_log_level_set_ext;

/***************************************************
Declaration of static Global Variables & Functions
***************************************************/
// Sec 6: declaration of static global variable
static osThreadId g_tAppThread_1;
static osThreadId g_tAppThread_2;
static E_IO01_UART_MODE g_eAppIO01UartMode;
osMessageQId g_tAppMessageQ;
osPoolId g_tAppMemPoolId;

// defined in wpa2_station_app.c
extern osMessageQId g_tWifiAppMessageQ;
extern osPoolId g_tWifiAppMemPoolId;

bool g_AP_connect_result = false;
bool g_AP_connect_complete = false;
uint32_t g_execution_count = 0;

// Sec 7: declaration of static function prototype
void __Patch_EntryPoint(void) __attribute__((section("ENTRY_POINT")));
void __Patch_EntryPoint(void) __attribute__((used));
static void Main_PinMuxUpdate(void);
static void Main_FlashLayoutUpdate(void);
static void Main_MiscModulesInit(void);
static void Main_AppInit_patch(void);
void User_Demo(void);

static void Main_AppThread_1(void *argu);
static void Main_AppThread_2(void *argu);
static void Main_MiscDriverConfigSetup(void);
static void Main_AtUartDbgUartSwitch(void);

static void Main_ApsUartRxDectecConfig(void);
static void Main_ApsUartRxDectecCb(E_GpioIdx_t tGpioIdx);

/***********
C Functions
***********/

// Sec 8: C Functions

void Internal_Module_Log_Set(char* module_name, bool on_off_set)
{
	  uint8_t log_level_set,i,module_index = TRACER_INT_TASK_NUM_MAX; 	
	
    if(on_off_set == true) 
        log_level_set = LOG_ALL_LEVEL;
    else
        log_level_set = LOG_NONE_LEVEL;	
    
    for (i = 0; i < TRACER_INT_TASK_NUM_MAX; i++) 
    {
        if (strcmp(module_name,g_taTracerDefIntTaskInfoBody[i].baName) == 0)
        {
            module_index = i;
            break;
        }
    }
    if(module_index < TRACER_INT_TASK_NUM_MAX) 
    {
        tracer_log_level_set_ext(module_index, log_level_set);
    } 
} 

void App_Log_Config(uint8_t log_idx, char* app_name , uint8_t level_set)
{
    //user log
	g_taTracerExtTaskInfoBody[log_idx].bLevel = level_set;
    strcpy(g_taTracerExtTaskInfoBody[log_idx].baName,app_name);
}

void Internal_Module_Log_Config(bool on_off_set)
{
	Internal_Module_Log_Set(OS_TASK_NAME_DIAG, on_off_set);
}

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
	
    Sys_SetUnsuedSramEndBound(0x440000);
    // the initial of driver part for cold and warm boot
    Sys_MiscModulesInit = Main_MiscModulesInit;
    Sys_MiscDriverConfigSetup = Main_MiscDriverConfigSetup;
    
    // update the switch AT UART / dbg UART function
    at_cmd_switch_uart1_dbguart = Main_AtUartDbgUartSwitch;	
    // application init
    Sys_AppInit = Main_AppInit_patch;
}

/*************************************************************************
* FUNCTION:
*   Main_PinMuxUpdate
*
* DESCRIPTION:
*   define pinmux 
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
//    at_io01_uart_mode_set(HAL_PIN_0_1_UART_MODE);
	g_eAppIO01UartMode=HAL_PIN_0_1_UART_MODE;
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
*   Main_MiscModulesInit
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
static void Main_MiscModulesInit(void)
{

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
    // Enable APS Uart log information output 
    Hal_DbgUart_RxIntEn(1);
    
	tracer_log_level_set_ext(0, LOG_NONE_LEVEL);
    tracer_log_level_set_ext(1, LOG_NONE_LEVEL);
    tracer_log_level_set_ext(2, LOG_NONE_LEVEL);
	tracer_log_level_set_ext(3, LOG_NONE_LEVEL);
    tracer_log_level_set_ext(4, LOG_NONE_LEVEL);
    tracer_log_level_set_ext(5, LOG_NONE_LEVEL);
	tracer_log_level_set_ext(6, LOG_NONE_LEVEL);
    tracer_log_level_set_ext(7, LOG_NONE_LEVEL);
    tracer_log_level_set_ext(8, LOG_NONE_LEVEL);
	tracer_log_level_set_ext(9, LOG_NONE_LEVEL);
    tracer_log_level_set_ext(10, LOG_NONE_LEVEL);
    tracer_log_level_set_ext(11, LOG_NONE_LEVEL);
	tracer_log_level_set_ext(12, LOG_NONE_LEVEL);
    tracer_log_level_set_ext(13, LOG_NONE_LEVEL);
    tracer_log_level_set_ext(14, LOG_NONE_LEVEL);
    tracer_log_level_set_ext(15, LOG_NONE_LEVEL);
    tracer_log_level_set_ext(16, LOG_NONE_LEVEL);
    tracer_log_level_set_ext(17, LOG_NONE_LEVEL);
	
    // Log example executin, create two thread 
    User_Demo();
    
    // Start WIFI app execution, scan AP and try to connect WIFI_SSID
    WifiAppInit();
    
}

/*************************************************************************
* FUNCTION:
*   Thread_demo
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
void User_Demo()
{
    osThreadDef_t tThreadDef;
    osMessageQDef_t tMessageDef;
    osPoolDef_t tMemPoolDef;

    // create the thread for AppThread_1
    tThreadDef.name = "demo_app1";
    tThreadDef.pthread = Main_AppThread_1;
    tThreadDef.tpriority = OS_TASK_PRIORITY_APP;        // osPriorityNormal
    tThreadDef.instances = 0;                           // reserved, it is no used
    tThreadDef.stacksize = OS_TASK_STACK_SIZE_APP;      // (512), unit: 4-byte, the size is 512*4 bytes
    g_tAppThread_1 = osThreadCreate(&tThreadDef, NULL);
    if (g_tAppThread_1 == NULL)
    {
        printf("To create the thread for AppThread_1 is fail.\n");
    }
    
    // create the thread for AppThread_2
    tThreadDef.name = "demo_app2";
    tThreadDef.pthread = Main_AppThread_2;
    tThreadDef.tpriority = OS_TASK_PRIORITY_APP;        // osPriorityNormal
    tThreadDef.instances = 0;                           // reserved, it is no used
    tThreadDef.stacksize = OS_TASK_STACK_SIZE_APP;      // (512), unit: 4-byte, the size is 512*4 bytes
    g_tAppThread_2 = osThreadCreate(&tThreadDef, NULL);
    if (g_tAppThread_2 == NULL)
    {
        printf("To create the thread for AppThread_2 is fail.\n");
    }
    else
    {
        printf("To create the thread for AppThread_2 successful \r\n");
    }
    
    // create the message queue for AppMessageQ
    tMessageDef.queue_sz = APP_MESSAGE_Q_SIZE;          // number of elements in the queue
    tMessageDef.item_sz = sizeof(S_MessageQ);           // size of an item
    tMessageDef.pool = NULL;                            // reserved, it is no used
    g_tAppMessageQ = osMessageCreate(&tMessageDef, g_tAppThread_2);
    if (g_tAppMessageQ == NULL)
    {
        printf("To create the message queue for AppMessageQ is fail.\n");
    }
    else
    {
        printf("To create the message queue for AppMessageQ successful \r\n");
    }
    
    // create the memory pool for AppMessageQ
    tMemPoolDef.pool_sz = APP_MESSAGE_Q_SIZE;           // number of items (elements) in the pool
    tMemPoolDef.item_sz = sizeof(S_MessageQ);           // size of an item
    tMemPoolDef.pool = NULL;                            // reserved, it is no used
    g_tAppMemPoolId = osPoolCreate(&tMemPoolDef);
    if (g_tAppMemPoolId == NULL)
    {
        printf("To create the memory pool for g_tAppMemPoolId is fail.\n");
    }    
    else
    {
        printf("To create the memory pool for g_tAppMemPoolId successful \r\n");
    }
}


/*************************************************************************
* FUNCTION:
*   Main_AppThread_1
*
* DESCRIPTION:
*   the application thread 1
*
* PARAMETERS
*   1. argu     : [In] the input argument
*
* RETURNS
*   none
*
*************************************************************************/
static void Main_AppThread_1(void *argu)
{
    uint32_t ulCount = 0; 
    while (1)
    {
        printf("\033[41;4m demo_app1: %d\033[0m\r\n",ulCount);
        osDelay(2000);      // delay 1000ms (1sec)
        ulCount++;
    }
}

/*************************************************************************
* FUNCTION:
*   Main_AppThread_2
*
* DESCRIPTION:
*   the application thread 2
*
* PARAMETERS
*   1. argu     : [In] the input argument
*
* RETURNS
*   none
*
*************************************************************************/
static void Main_AppThread_2(void *argu)
{
    uint32_t ulCount = 0;
    char AP_ssid_name[32] = WIFI_SSID;
    S_MessageQ tMsg;
    osEvent tEvent;
    S_MessageQ *ptMsgPool;

    while (1)
    {        
        printf("demo_app2: %d, g_execution_count:%d \r\n",ulCount,g_execution_count);
        
        // receive the message from AppMessageQ
        tEvent = osMessageGet(g_tAppMessageQ, osWaitForever);
        if (tEvent.status != osEventMessage)
        {
            printf("To receive the message from AppMessageQ is fail.\n");
            continue;
        }    
        // get the content of message
        ptMsgPool = (S_MessageQ *)tEvent.value.p;

        // means received message from user_app thread 
        if(ptMsgPool->ulEventId == UAER_WIFI_APP_CONNECT_COMPLETE)
        //if(g_AP_connect_complete) 
        {
            g_AP_connect_result = ptMsgPool->bConnectResult;

            // check last time AP connection result 
            if(g_AP_connect_result == true)
            {
                printf("demo_app2: %s connection is success. \r\n",AP_ssid_name);
            }
            else
            {
                printf("demo_app2: %s connection is failed. \r\n",AP_ssid_name);
            }

            // indicate WIFI AP connection procedure is finished. 
            if ((g_execution_count%4) == 2)
            {
//                printf("demo_app2: Enable ROM internal module log output \r\n");
//                // enable ROM internal module log output 
//                Internal_Module_Log_Config(true);

                printf("-----demo_app2: Enable user demo_app1 log output \r\n");
                // enable user demo_app1 log output                 
                App_Log_Config(USER_APP1_LOG,"demo_app1",LOG_ALL_LEVEL);                
            }
            else if ((g_execution_count%4) == 0)
            {
//                printf("demo_app2: Disable ROM internal module log output \r\n");
//                // disable ROM internal module log output
//                Internal_Module_Log_Config(false);

                printf("-----demo_app2: Disable user demo_app1 log output \r\n");
                // disable user demo_app1 log output 
                App_Log_Config(USER_APP1_LOG,"demo_app1",LOG_NONE_LEVEL);                
            }

            // Send message to WIFI user_app thread, let it begin another Scan->connect procedure 

            tMsg.ulEventId = UAER_WIFI_APP_RE_CONNECT;
            wifi_AppMessageQSend(&tMsg, g_tWifiAppMessageQ,g_tWifiAppMemPoolId);  
            
            g_execution_count++;
        }
        osDelay(2000);      // delay 1000ms (1sec)
        ulCount++; 
    }
}

