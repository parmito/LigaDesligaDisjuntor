/*
 * app.c
 *
 *  Created on: 13/11/2018
 *      Author: danilo
 */

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <sys/param.h>
#include <dirent.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "esp_system.h"

#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"

#include <esp_system.h>
#include <esp_spi_flash.h>
#include <rom/spi_flash.h>

#include "defines.h"
#include "State.h"
#include "Debug.h"
#include "Gsm.h"
#include "Sd.h"
#include "Ble.h"
#include "App.h"

//////////////////////////////////////////////
//
//
//            FUNCTION PROTOTYPES
//
//
//////////////////////////////////////////////
unsigned char TaskApp_Init(sMessageType *psMessage);
unsigned char TaskApp_RemoteCode(sMessageType *psMessage);
unsigned char TaskApp_RecordingRemote(sMessageType *psMessage);
unsigned char TaskApp_BleData(sMessageType *psMessage);
void vTaskApp( void *pvParameters );

//////////////////////////////////////////////
//
//
//              VARIABLES
//
//
//////////////////////////////////////////////
#define NUM_TIMERS 2
static TimerHandle_t pxTimer[ NUM_TIMERS ];

typedef struct{
	const char TimerName[16];
}tstTimerName;

static tstTimerName stTimerName[]=
{
		{"TIMER1"},
		{"TIMER2"}
};

sMessageType stAppMsg;
static const char *APP_TASK_TAG = "APP_TASK";
static char cDataBuffer[64];
static char cDataBuffer1[64];
tstSensorKeylessCode stSensor[MAX_KEYLESS_SENSOR];
tstSensorKeylessCode stKeyless[MAX_KEYLESS_SENSOR];
tstSensorKeylessCode stTelephone[MAX_KEYLESS_SENSOR];
/*tstSensorKeylessCode stSensorKeylessCode[MAX_KEYLESS_SENSOR];*/
static unsigned char ucCurrentStateApp = TASKAPP_INITIALIZING;

extern tstConfiguration stConfigData;

unsigned long ulTimestamp = 0;
//////////////////////////////////////////////
//
//
//              App_Init
//
//
//////////////////////////////////////////////
void AppInit(void)
{
	/*esp_log_level_set(SD_TASK_TAG, ESP_LOG_INFO);*/

	xQueueApp = xQueueCreate(appQUEUE_LENGTH,			/* The number of items the queue can hold. */
							sizeof( sMessageType ) );	/* The size of each item the queue holds. */


   /* StateAlarm*/
	char *pcChar;
	/*pcChar = strcmp (stConfigData.cState,"UNARMED");*/
	if(strcmp (stConfigData.cState,"UNARMED") == 0)
	{
		ESP_LOGI(APP_TASK_TAG, "UNARMED\r\n");
		stAppMsg.ucEvent = EVENT_APP_INIT;
	}
	else
	{
		/*pcChar = strstr (stConfigData.cState,"ARMED");*/
		if(strcmp (stConfigData.cState,"ARMED") == 0)
		{
			ESP_LOGI(APP_TASK_TAG, "ARMED\r\n");
			stAppMsg.ucEvent = EVENT_APP_ARMING;
		}
	}

	stAppMsg.ucSrc = SRC_APP;
	stAppMsg.ucDest = SRC_APP;

	xQueueSend( xQueueApp, ( void * )&stAppMsg, 0);

    xTaskCreate(vTaskApp, "vTaskApp", 1024*4, NULL, configMAX_PRIORITIES-4, NULL);
	/* Create the queue used by the queue send and queue receive tasks.
	http://www.freertos.org/a00116.html */




}
//////////////////////////////////////////////
//
//
//              TaskApp_Init
//
//
//////////////////////////////////////////////
unsigned char TaskApp_Init(sMessageType *psMessage)
{
	unsigned char boError = true;

	ESP_LOGI(APP_TASK_TAG, "TaskApp_Init\r\n");

	return boError;
}

//////////////////////////////////////////////
//
//
//              TaskApp_Arming
//
//
//////////////////////////////////////////////
unsigned char TaskApp_Arming(sMessageType *psMessage)
{
	unsigned char boError = true;
	static char cLocalBuffer[64];

	ESP_LOGI(APP_TASK_TAG, "TaskApp_Arming\r\n");
#if 0
	memset(cDataBuffer1,0,sizeof(cDataBuffer1));
	strcpy(cDataBuffer1,"/spiffs/DATASET.TXT,");/*Filename*/
	strcat(cDataBuffer1,"STATE=ARMED\r\n");/*Parameter*/
#endif

	ucSdWriteState("ARMED");
	/*stAppMsg.ucSrc = SRC_APP;
	stAppMsg.ucDest = SRC_SD;
	stAppMsg.ucEvent = EVENT_SD_WRITING_CONFIG;
	stAppMsg.pcMessageData = &cDataBuffer1[0];
	xQueueSend( xQueueSd, ( void * )&stAppMsg, 0);*/

	stAppMsg.ucSrc = SRC_APP;
	stAppMsg.ucDest = SRC_DEBUG;
	stAppMsg.ucEvent = EVENT_IO_OUT1_ARMING;
	xQueueSend( xQueueDebug, ( void * )&stAppMsg, 0);

	memset(cLocalBuffer,0, sizeof(cLocalBuffer));
	strcpy(cLocalBuffer,"DISJUNTOR_LIGADO");

	stAppMsg.ucSrc = SRC_APP;
	stAppMsg.ucDest = SRC_SD;
	stAppMsg.ucEvent = EVENT_SD_WRITING;
	stAppMsg.pcMessageData = cLocalBuffer;
	xQueueSend( xQueueSd, ( void * )&stAppMsg, 0);

	xQueueReset(xQueueApp);


	return(boError);
}

//////////////////////////////////////////////
//
//
//              TaskApp_RemoteCode
//
//
//////////////////////////////////////////////
unsigned char TaskApp_RemoteCode(sMessageType *psMessage)
{
	unsigned char boError = false;
	tstSensorKeylessCode *pst = NULL;

	ESP_LOGI(APP_TASK_TAG, "TaskApp_RemoteCode:%s",psMessage->pcMessageData);

	/*char cDummy[32] = "/spiffs/KEYLESS.TXT,K1,9BD7F55\r\n";
	strcpy(psMessage->pcMessageData,cDummy);
	boError = TaskApp_RecordingRemote(psMessage);*/

	/*char cDummy[64] = "/spiffs/SENSOR.TXT,SENSOR1,8C96625\r\n";//PORTA FRENTE
	strcpy(psMessage->pcMessageData,cDummy);
	boError = TaskApp_RecordingRemote(psMessage);*/

	/*char cDummy[64] = "/spiffs/SENSOR.TXT,SENSOR2,A0BB4A5\r\n";//JANELA FRENTE
	strcpy(psMessage->pcMessageData,cDummy);
	boError = TaskApp_RecordingRemote(psMessage);*/

	/*char cDummy[64] ="/spiffs/SENSOR.TXT,SENSOR3,BF3F265\r\n";//PORTA COZINHA
	strcpy(psMessage->pcMessageData,cDummy);
	boError = TaskApp_RecordingRemote(psMessage);*/


	for(int i = 0; i < MAX_KEYLESS_SENSOR; i++)
	{
		pst = &stKeyless[i];
		if( (strlen(pst->cId) > 0) && (strlen(pst->cCode) >0))
		{
			ESP_LOGI(APP_TASK_TAG, "Keyless[%d]:%s,%s==RemoteCode:%s",i,pst->cId,pst->cCode,psMessage->pcMessageData);

			if(strncmp(pst->cCode,psMessage->pcMessageData,7) == 0)
			{
				ESP_LOGI(APP_TASK_TAG, "Code Matches:%d",ucCurrentStateApp);
				switch(ucCurrentStateApp)
				{
					case TASKAPP_IDLING:
						ESP_LOGI(APP_TASK_TAG, "ARMING\r\n");

						memset(cDataBuffer1,0,sizeof(cDataBuffer1));
	#if 0
						strcpy(cDataBuffer1,"/spiffs/DATASET.TXT,");/*Filename*/
						strcat(cDataBuffer1,"STATE=ARMED\r\n");/*Parameter*/

						stAppMsg.ucSrc = SRC_APP;
						stAppMsg.ucDest = SRC_SD;
						stAppMsg.ucEvent = EVENT_SD_WRITING_CONFIG;
						stAppMsg.pcMessageData = &cDataBuffer1[0];
						xQueueSend( xQueueSd, ( void * )&stAppMsg, 0);
	#endif

						memset(cDataBuffer1,0,sizeof(cDataBuffer1));
						strcpy(cDataBuffer1,"CONF:");
						strcat(cDataBuffer1,"STATE:");
						strcat(cDataBuffer1,"ARMED");
						strcat(cDataBuffer1,"\r\n");
						ESP_LOGI(APP_TASK_TAG, "%s\r\n", cDataBuffer1);

						stAppMsg.ucSrc = SRC_HTTPSRV;
						stAppMsg.ucDest = SRC_SD;
						stAppMsg.ucEvent = EVENT_SD_READWRITE_CONFIG;
						stAppMsg.pcMessageData = (char*)cDataBuffer1;
						xQueueSend( xQueueSd, ( void * )&stAppMsg, 0);

						stAppMsg.ucSrc = SRC_APP;
						stAppMsg.ucDest = SRC_DEBUG;
						stAppMsg.ucEvent = EVENT_IO_OUT1_ARMING;
						xQueueSend( xQueueDebug, ( void * )&stAppMsg, 0);

						vTaskDelay( 750/portTICK_PERIOD_MS );
						xQueueReset(xQueueApp);
						boError = true;
						return(boError);

					break;

					case TASKAPP_ALARMING:
						ESP_LOGI(APP_TASK_TAG, "IDLING\r\n");

						memset(cDataBuffer1,0,sizeof(cDataBuffer1));
						strcpy(cDataBuffer1,"CONF:");
						strcat(cDataBuffer1,"STATE:");
						strcat(cDataBuffer1,"UNARMED");
						strcat(cDataBuffer1,"\r\n");
						ESP_LOGI(APP_TASK_TAG, "%s\r\n", cDataBuffer1);

						stAppMsg.ucSrc = SRC_HTTPSRV;
						stAppMsg.ucDest = SRC_SD;
						stAppMsg.ucEvent = EVENT_SD_READWRITE_CONFIG;
						stAppMsg.pcMessageData = (char*)cDataBuffer1;
						xQueueSend( xQueueSd, ( void * )&stAppMsg, 0);

						stAppMsg.ucSrc = SRC_APP;
						stAppMsg.ucDest = SRC_DEBUG;
						stAppMsg.ucEvent = EVENT_IO_OUT1_ARMING;
						xQueueSend( xQueueDebug, ( void * )&stAppMsg, 0);


						stAppMsg.ucSrc = SRC_APP;
						stAppMsg.ucDest = SRC_DEBUG;
						stAppMsg.ucEvent = EVENT_IO_OUT1_DISARMING;
						xQueueSend( xQueueDebug, ( void * )&stAppMsg, 0);

						vTaskDelay( 750/portTICK_PERIOD_MS );

						xQueueReset(xQueueApp);
						boError = true;
						return(boError);
					break;

					default:
					break;
				}
			}
		}
	}
	return(boError);
}

//////////////////////////////////////////////
//
//
//        TaskApp_FindOutRemoteCode
//
//
//////////////////////////////////////////////
unsigned char TaskApp_FindOutRemoteCode(sMessageType *psMessage)
{
	unsigned char boError = false;
	tstSensorKeylessCode *pst = NULL;

	ESP_LOGI(APP_TASK_TAG, "TaskApp_FindOutRemoteCode:%s",psMessage->pcMessageData);

	for(int i = 0; i < MAX_KEYLESS_SENSOR; i++)
	{
		pst = &stKeyless[i];

		if(strncmp(pst->cCode,psMessage->pcMessageData,7) == 0)
		{
			stAppMsg.ucSrc = SRC_APP;
			stAppMsg.ucDest = SRC_APP;
			stAppMsg.ucEvent = EVENT_APP_DISARMING;
			xQueueSend( xQueueApp, ( void * )&stAppMsg, 0);
			ESP_LOGI(APP_TASK_TAG, "KEYLESS CODE");

			boError = true;
			return(boError);
		}
		else
		{
			pst = &stSensor[i];
			if(strncmp(pst->cCode,psMessage->pcMessageData,7) == 0)
			{
				stAppMsg.ucSrc = SRC_APP;
				stAppMsg.ucDest = SRC_APP;
				stAppMsg.ucEvent = EVENT_APP_SENSOR_CODE;
				stAppMsg.pcMessageData = pst->cId;

				xQueueSend( xQueueApp, ( void * )&stAppMsg, 0);
				ESP_LOGI(APP_TASK_TAG, "SENSOR CODE:%s",pst->cId);

				boError = true;
				return(boError);
			}
		}
	}

	return(boError);
}


//////////////////////////////////////////////
//
//
//        TaskApp_Disarming
//
//
//////////////////////////////////////////////
unsigned char TaskApp_Disarming(sMessageType *psMessage)
{
	static char cLocalBuffer[64];
	unsigned char boError = true;

	ESP_LOGI(APP_TASK_TAG, "TaskApp_Disarming\r\n");
	ESP_LOGI(APP_TASK_TAG, "DISARMING\r\n");

#if 0
	memset(cDataBuffer1,0,sizeof(cDataBuffer1));
	strcpy(cDataBuffer1,"/spiffs/DATASET.TXT,");/*Filename*/
	strcat(cDataBuffer1,"STATE=UNARMED\r\n");/*Parameter*/
#endif

	ucSdWriteState("UNARMED");

	/*stAppMsg.ucSrc = SRC_APP;
	stAppMsg.ucDest = SRC_SD;
	stAppMsg.ucEvent = EVENT_SD_WRITING_CONFIG;
	stAppMsg.pcMessageData = &cDataBuffer1[0];
	xQueueSend( xQueueSd, ( void * )&stAppMsg, 0);*/

	stAppMsg.ucSrc = SRC_APP;
	stAppMsg.ucDest = SRC_DEBUG;
	stAppMsg.ucEvent = EVENT_IO_OUT1_DISARMING;
	xQueueSend( xQueueDebug, ( void * )&stAppMsg, 0);

	memset(cLocalBuffer,0, sizeof(cLocalBuffer));
	strcpy(cLocalBuffer,"DISJUNTOR_DESLIGADO");

	stAppMsg.ucSrc = SRC_APP;
	stAppMsg.ucDest = SRC_SD;
	stAppMsg.ucEvent = EVENT_SD_WRITING;
	stAppMsg.pcMessageData = cLocalBuffer;
	xQueueSend( xQueueSd, ( void * )&stAppMsg, 0);
	xQueueReset(xQueueApp);


	return(boError);
}

//////////////////////////////////////////////
//
//
//        TaskApp_SensorCode
//
//
//////////////////////////////////////////////
unsigned char TaskApp_SensorCode(sMessageType *psMessage)
{
	unsigned char boError = true;

	ESP_LOGI(APP_TASK_TAG, "TaskApp_SensorCode:%s\r\n",psMessage->pcMessageData);
	ESP_LOGI(APP_TASK_TAG, "ALARMING\r\n");

	stAppMsg.ucSrc = SRC_APP;
	stAppMsg.ucDest = SRC_DEBUG;
	stAppMsg.ucEvent = EVENT_IO_OUT1_ALARMING;
	xQueueSend( xQueueDebug, ( void * )&stAppMsg, 0);

	/*stAppMsg.ucSrc = SRC_APP;
	stAppMsg.ucDest = SRC_GSM;
	stAppMsg.ucEvent = EVENT_GSM_ALARMING;
	stAppMsg.pcMessageData = psMessage->pcMessageData;
	xQueueSend( xQueueGsm, ( void * )&stAppMsg, 0);*/

	stAppMsg.ucSrc = SRC_APP;
	stAppMsg.ucDest = SRC_SD;
	stAppMsg.ucEvent = EVENT_SD_WRITING;
	stAppMsg.pcMessageData = psMessage->pcMessageData;
	xQueueSend( xQueueSd, ( void * )&stAppMsg, 0);
	xQueueReset(xQueueApp);


	xTimerStart( pxTimer[ 0 ], 0 );

	return(boError);
}


//////////////////////////////////////////////
//
//
//        TaskApp_BleProgrammingKeyless
//
//
//////////////////////////////////////////////
unsigned char TaskApp_BleProgrammingKeyless(sMessageType *psMessage)
{
	//PORTA FRENTE
	/*char cDummy[64] = "/spiffs/KEYLESS.TXT,KEYLESS1,8LC96625\r\n";*/
	unsigned char boError = true;
	ESP_LOGI(APP_TASK_TAG, "TaskApp_BleProgrammingKeyless\r\n");

	char *pch = NULL;

	if(psMessage->pcMessageData != NULL ){
		ESP_LOGI(APP_TASK_TAG,"psMessage->pcMessageData=%s\r\n",psMessage->pcMessageData);

		pch = strtok(psMessage->pcMessageData," ");
		if(pch != NULL )
		{
			ESP_LOGI(APP_TASK_TAG,"pch=%s\r\n",pch);
			pch = strtok(NULL,"#");
			if(pch != NULL )
			{
				ESP_LOGI(APP_TASK_TAG,"pch=%s\r\n",pch);

				memset(cDataBuffer1,0,sizeof(cDataBuffer1));
				strcpy(cDataBuffer1,"/spiffs/KEYLESS.TXT,");
				strcat(cDataBuffer1,pch);
				strcat(cDataBuffer1,",");

				ESP_LOGI(APP_TASK_TAG, "Keyless:%s",cDataBuffer1);
#if SRC_BLE
				memset(cDataBuffer,0,sizeof(cDataBuffer));
				strcpy(cDataBuffer,"FORCE SENSOR SENDING CODE");

				stAppMsg.ucSrc = SRC_APP;
				stAppMsg.ucDest = SRC_BLE;
				stAppMsg.ucEvent = (int)NULL;
				stAppMsg.pcMessageData = &cDataBuffer[0];

				xQueueSend(xQueueBle,( void * )&stAppMsg,NULL);
#endif
				stAppMsg.ucSrc = SRC_APP;
				stAppMsg.ucDest = SRC_DEBUG;
				stAppMsg.ucEvent = EVENT_IO_OUT1_ALARMING;
				xQueueSend( xQueueDebug, ( void * )&stAppMsg, 0);

				xTimerStart( pxTimer[ 1 ], 0 );
			}
		}
	}
	return boError;
}

//////////////////////////////////////////////
//
//
//       TaskApp_BleProgrammingSensor
//
//
//////////////////////////////////////////////
unsigned char TaskApp_BleProgrammingSensor(sMessageType *psMessage)
{

	//PORTA FRENTE
	/*char cDummy[64] = "/spiffs/SENSOR.TXT,SENSOR1,8C96625\r\n";*/
	unsigned char boError = true;

	ESP_LOGI(APP_TASK_TAG, "TaskApp_BleProgrammingSensor\r\n");

	char *pch = NULL;

	if(psMessage->pcMessageData != NULL ){
		ESP_LOGI(APP_TASK_TAG,"psMessage->pcMessageData=%s\r\n",psMessage->pcMessageData);

		pch = strtok(psMessage->pcMessageData," ");
		if(pch != NULL )
		{
			ESP_LOGI(APP_TASK_TAG,"pch=%s\r\n",pch);
			pch = strtok(NULL,"#");
			if(pch != NULL )
			{
				ESP_LOGI(APP_TASK_TAG,"pch=%s\r\n",pch);

				memset(cDataBuffer1,0,sizeof(cDataBuffer1));
				strcpy(cDataBuffer1,"/spiffs/SENSOR.TXT,");
				strcat(cDataBuffer1,pch);
				strcat(cDataBuffer1,",");

				ESP_LOGI(APP_TASK_TAG, "Sensor:%s",cDataBuffer1);

				/*memset(cDataBuffer,0,sizeof(cDataBuffer));
				strcpy(cDataBuffer,"FORCE SENSOR SENDING CODE");

				stAppMsg.ucSrc = SRC_APP;
				stAppMsg.ucDest = SRC_BLE;
				stAppMsg.ucEvent = (int)NULL;
				stAppMsg.pcMessageData = &cDataBuffer[0];

				xQueueSend(xQueueBle,( void * )&stAppMsg,NULL);*/

				stAppMsg.ucSrc = SRC_APP;
				stAppMsg.ucDest = SRC_DEBUG;
				stAppMsg.ucEvent = EVENT_IO_OUT1_ALARMING;
				xQueueSend( xQueueDebug, ( void * )&stAppMsg, 0);

				xTimerStart( pxTimer[ 1 ], 0 );
			}
		}
	}

	return boError;
}

//////////////////////////////////////////////
//
//
//       TaskApp_BleProgrammingTelephone
//
//
//////////////////////////////////////////////
unsigned char TaskApp_BleProgrammingTelephone(sMessageType *psMessage)
{
	unsigned char boError = true;

	ESP_LOGI(APP_TASK_TAG, "TaskApp_BleProgrammingTelephone\r\n");

	char *pch;

	pch = strtok(psMessage->pcMessageData," ");
	pch = strtok(NULL,":");

	memset(cDataBuffer1,0,sizeof(cDataBuffer1));
	strcpy(cDataBuffer1,"/spiffs/TELEPHONE.TXT,");
	strcat(cDataBuffer1,pch);
	strcat(cDataBuffer1,",");

	pch = strtok(NULL,"#");
	strcat(cDataBuffer1,pch);

	ESP_LOGI(APP_TASK_TAG, "TaskApp_RecordingTelephone");
	ESP_LOGI(APP_TASK_TAG, "Telephone:%s",cDataBuffer1);

	stAppMsg.ucSrc = SRC_APP;
	stAppMsg.ucDest = SRC_SD;
	stAppMsg.ucEvent = EVENT_SD_PROGRAMMING;
	stAppMsg.pcMessageData = &cDataBuffer1[0];
	xQueueSend( xQueueSd, ( void * )&stAppMsg, 0);

	return boError;
}

//////////////////////////////////////////////
//
//
//       TaskApp_BleErasingTelephone
//
//
//////////////////////////////////////////////
unsigned char TaskApp_BleErasingTelephone(sMessageType *psMessage)
{
	unsigned char boError = true;

	ESP_LOGI(APP_TASK_TAG, "TaskApp_BleErasingTelephone\r\n");

	char *pch;

	pch = strtok(psMessage->pcMessageData," ");
	pch = strtok(NULL,":");

	memset(cDataBuffer1,0,sizeof(cDataBuffer1));
	strcpy(cDataBuffer1,"/spiffs/TELEPHONE.TXT,");
	strcat(cDataBuffer1,pch);
	strcat(cDataBuffer1,",");

	pch = strtok(NULL,"#");
	strcat(cDataBuffer1,pch);

	ESP_LOGI(APP_TASK_TAG, "TaskApp_ErasingTelephone");
	ESP_LOGI(APP_TASK_TAG, "Telephone:%s",cDataBuffer1);

	stAppMsg.ucSrc = SRC_APP;
	stAppMsg.ucDest = SRC_SD;
	stAppMsg.ucEvent = EVENT_SD_ERASING;
	stAppMsg.pcMessageData = &cDataBuffer1[0];
	xQueueSend( xQueueSd, ( void * )&stAppMsg, 0);

	return boError;
}

//////////////////////////////////////////////
//
//
//       TaskApp_BleErasingKeyless
//
//
//////////////////////////////////////////////
unsigned char TaskApp_BleErasingKeyless(sMessageType *psMessage)
{
	unsigned char boError = true;

	ESP_LOGI(APP_TASK_TAG, "TaskApp_BleErasingKeyless\r\n");

    if (remove("/spiffs/KEYLESS.TXT") == 0)
    	ESP_LOGI(APP_TASK_TAG,"Deleted successfully");
     else
    	 ESP_LOGI(APP_TASK_TAG,"Unable to delete the file");

	return boError;
}


//////////////////////////////////////////////
//
//
//        TaskApp_RecordingRemote
//
//
//////////////////////////////////////////////
unsigned char TaskApp_RecordingRemote(sMessageType *psMessage)
{
	unsigned char boError = true;

	ESP_LOGI(APP_TASK_TAG, "TaskApp_RecordingRemote\r\n");

	strcat(cDataBuffer1,psMessage->pcMessageData);
	ESP_LOGI(APP_TASK_TAG, "Remote Code:%s",cDataBuffer1);

	stAppMsg.ucSrc = SRC_APP;
	stAppMsg.ucDest = SRC_SD;
	stAppMsg.ucEvent = EVENT_SD_PROGRAMMING;
	stAppMsg.pcMessageData = &cDataBuffer1[0];
	xQueueSend( xQueueSd, ( void * )&stAppMsg, 0);

	return boError;
}
//////////////////////////////////////////////
//
//
//      TaskApp_RemoteProgrammingRecorded
//
//
//////////////////////////////////////////////
unsigned char TaskApp_RemoteProgrammingRecorded(sMessageType *psMessage)
{
	unsigned char boError = true;

	ESP_LOGI(APP_TASK_TAG, "TaskApp_RemoteProgrammingRecorded:%d",(int)psMessage->u32MessageData);

	stAppMsg.ucSrc = SRC_APP;
	stAppMsg.ucDest = SRC_DEBUG;
	stAppMsg.ucEvent = EVENT_IO_OUT1_DISARMING;
	xQueueSend( xQueueDebug, ( void * )&stAppMsg, 0);

	return boError;
}
//////////////////////////////////////////////
//
//
//       TaskApp_BleProgrammingSensorId
//
//
//////////////////////////////////////////////
#if 0
unsigned char TaskApp_BleProgrammingSensorId(sMessageType *psMessage)
{
	unsigned char boError = true;
	char *pch;

	ESP_LOGI(APP_TASK_TAG, "TaskApp_BleProgrammingSensorId\r\n");

	pch = strtok(psMessage->pcMessageData,"#");
	pch = strtok(NULL," ");

	memset(cDataBuffer,0,sizeof(cDataBuffer));
	strcpy(cDataBuffer,"/spiffs/SENSOR.TXT,");
	strcat(cDataBuffer,pch);
	strcat(cDataBuffer,",");



	ESP_LOGI(APP_TASK_TAG, "Sensor:%s",cDataBuffer);

	return boError;
}
#endif
//////////////////////////////////////////////
//
//
//        TaskApp_RemoteProgrammingSensor
//
//
//////////////////////////////////////////////
unsigned char TaskApp_RemoteProgrammingSensor(sMessageType *psMessage)
{
	unsigned char boError = true;

	strcat(cDataBuffer,psMessage->pcMessageData);

	ESP_LOGI(APP_TASK_TAG, "TaskApp_RemoteProgrammingSensor");
	ESP_LOGI(APP_TASK_TAG, "Sensor Code:%s",psMessage->pcMessageData);

	stAppMsg.ucSrc = SRC_APP;
	stAppMsg.ucDest = SRC_SD;
	stAppMsg.ucEvent = EVENT_SD_PROGRAMMING;
	stAppMsg.pcMessageData = &cDataBuffer[0];
	xQueueSend( xQueueSd, ( void * )&stAppMsg, 0);


	return boError;
}


//////////////////////////////////////////////
//
//
//        		TaskApp_Rearm
//
//
//////////////////////////////////////////////
unsigned char TaskApp_Rearm(sMessageType *psMessage)
{
	unsigned char boError = true;

	ESP_LOGI(APP_TASK_TAG, "TaskApp_Rearm\r\n");

	stAppMsg.ucSrc = SRC_APP;
	stAppMsg.ucDest = SRC_DEBUG;
	stAppMsg.ucEvent = EVENT_IO_OUT1_DISARMING;
	xQueueSend( xQueueDebug, ( void * )&stAppMsg, 0);

	xQueueReset(xQueueApp);

	return(boError);
}


//////////////////////////////////////////////
//
//
//              TaskApp_IgnoreEvent
//
//
//////////////////////////////////////////////
unsigned char TaskApp_IgnoreEvent(sMessageType *psMessage)
{
	unsigned char boError = false;
	return(boError);
}

static sStateMachineType const gasTaskApp_Initializing[] =
{
        /* Event                            Action routine          Success state               Failure state*/
        //  State specific transitions
        {EVENT_APP_INIT,                     TaskApp_Init,          TASKAPP_IDLING,   			TASKAPP_INITIALIZING },
        {EVENT_APP_ARMING,       	         TaskApp_Arming,		TASKAPP_ARMING,			 	TASKAPP_INITIALIZING },
	    {EVENT_APP_NULL,                     TaskApp_IgnoreEvent,	TASKAPP_INITIALIZING,		TASKAPP_INITIALIZING }
};

static sStateMachineType const gasTaskApp_Idling[] =
{
        /* Event                            	Action routine          		Success state               		Failure state*/
        //  State specific transitions
        {EVENT_APP_REMOTE_CODE,             	TaskApp_RemoteCode,				TASKAPP_ARMING,			 			TASKAPP_IDLING },
        {EVENT_APP_BLE_PROGRAMMING_KEYLESS,		TaskApp_BleProgrammingKeyless,	TASKAPP_RECORDING_REMOTE,  			TASKAPP_IDLING },
        {EVENT_APP_BLE_ERASING_KEYLESS,			TaskApp_BleErasingKeyless,		TASKAPP_IDLING,			  			TASKAPP_IDLING },
		{EVENT_APP_ARMING,       	         	TaskApp_Arming,					TASKAPP_ARMING,			 			TASKAPP_IDLING },
	    {EVENT_APP_NULL,                     	TaskApp_IgnoreEvent,			TASKAPP_IDLING,						TASKAPP_IDLING }
};

static sStateMachineType const gasTaskApp_RecordingRemote[] =
{
        /* Event                            		Action routine          						Success state               			Failure state*/
        //  State specific transitions
        {EVENT_APP_REMOTE_CODE,              		TaskApp_RecordingRemote,    					TASKAPP_WAITING_RECORDED_REMOTE,		TASKAPP_IDLING },
        {EVENT_APP_REMOTE_RECORDED,    				TaskApp_RemoteProgrammingRecorded,				TASKAPP_IDLING,							TASKAPP_IDLING },
	    {EVENT_APP_NULL,                     		TaskApp_IgnoreEvent,							TASKAPP_RECORDING_REMOTE,				TASKAPP_RECORDING_REMOTE }
};

static sStateMachineType const gasTaskApp_WaitingFeedbackRecordedRemote[] =
{
        /* Event                            		Action routine          						Success state               			Failure state*/
        //  State specific transitions
        {EVENT_APP_REMOTE_RECORDED,    				TaskApp_RemoteProgrammingRecorded,				TASKAPP_IDLING,							TASKAPP_IDLING },
	    {EVENT_APP_NULL,                     		TaskApp_IgnoreEvent,							TASKAPP_WAITING_RECORDED_REMOTE,		TASKAPP_WAITING_RECORDED_REMOTE }
};

static sStateMachineType const gasTaskApp_Arming[] =
{
        /* Event                            		Action routine          						Success state               			Failure state*/
        //  State specific transitions
        {EVENT_APP_REMOTE_CODE,             		TaskApp_FindOutRemoteCode,						TASKAPP_ARMING,  		 				TASKAPP_ARMING },
        {EVENT_APP_DISARMING,             			TaskApp_Disarming,								TASKAPP_IDLING,  		 				TASKAPP_ARMING },
	    {EVENT_APP_NULL,                     		TaskApp_IgnoreEvent,							TASKAPP_ARMING,							TASKAPP_ARMING }
};




static sStateMachineType const * const gpasTaskApp_StateMachine[] =
{
    gasTaskApp_Initializing,
    gasTaskApp_Idling,
    gasTaskApp_RecordingRemote,
    gasTaskApp_WaitingFeedbackRecordedRemote,
    gasTaskApp_Arming
};

/* Define a callback function that will be used by multiple timer
 instances.  The callback function does nothing but count the number
 of times the associated timer expires, and stop the timer once the
 timer has expired 10 times.  The count is saved as the ID of the
 timer. */

 void vTimerCallbackApp( TimerHandle_t xTimer )
 {
	 const uint32_t ulMaxExpiryCountBeforeStopping = 15;
	 uint32_t ulCount;

    /* Optionally do something if the pxTimer parameter is NULL. */
    configASSERT( xTimer );


    /* The number of times this timer has expired is saved as the
    timer's ID.  Obtain the count. */
    ulCount = ( uint32_t ) pvTimerGetTimerID( xTimer );

    /* Increment the count, then test to see if the timer has expired
    ulMaxExpiryCountBeforeStopping yet. */
    ulCount++;

	ESP_LOGI(APP_TASK_TAG,"pcTimerGetTimerName =%s\r\n",pcTimerGetTimerName(xTimer));

    /* If the timer has expired 10 times then stop it from running. */
    if( ulCount >= ulMaxExpiryCountBeforeStopping )
    {
        /* Do not use a block time if calling a timer API function
        from a timer callback function, as doing so could cause a
        deadlock! */
        xTimerStop( xTimer, 0 );

        ulCount = 0;
        vTimerSetTimerID( xTimer, ( void * ) ulCount );

        if( strcmp(pcTimerGetTimerName(xTimer),stTimerName[0].TimerName) == 0)
        {
			stAppMsg.ucSrc = SRC_APP;
			stAppMsg.ucDest = SRC_APP;
			stAppMsg.ucEvent = EVENT_APP_REARM;

			xQueueSend( xQueueApp, ( void * )&stAppMsg, 0);
        }else
        {
            if(strcmp(pcTimerGetTimerName(xTimer),stTimerName[1].TimerName) == 0)
            {
    			stAppMsg.ucSrc = SRC_APP;
    			stAppMsg.ucDest = SRC_APP;
    			stAppMsg.ucEvent = EVENT_APP_REMOTE_RECORDED;

    			xQueueSend( xQueueApp, ( void * )&stAppMsg, 0);
            }
        }
    }
    else
    {
       /* Store the incremented count back into the timer's ID field
       so it can be read back again the next time this software timer
       expires. */
       vTimerSetTimerID( xTimer, ( void * ) ulCount );
    }
 }



void vTaskApp( void *pvParameters )
{
	TickType_t elapsed_time;
	static unsigned int ui = 0;
	long x;

	for( x = 0; x < NUM_TIMERS; x++ )
	{
		pxTimer[ x ] = xTimerCreate
		(
			/* Just a text name, not used by the RTOS
			kernel. */
			stTimerName[x].TimerName,
			/* The timer period in ticks, must be
			greater than 0. */
			1000 / portTICK_PERIOD_MS,
			/* The timers will auto-reload themselves
			when they expire. */
			pdTRUE,
			/* The ID is used to store a count of the
			number of times the timer has expired, which
			is initialised to 0. */
			( void * ) x,
			/* Each timer calls the same callback when
			it expires. */
			vTimerCallbackApp
		);
	}

	for( ;; )
	{
		elapsed_time = xTaskGetTickCount();
	    /*ESP_LOGI(APP_TASK_TAG, "Running...");*/

		if( xQueueReceive( xQueueApp, &( stAppMsg ),0 ) )
		{
            (void)eEventHandler ((unsigned char)SRC_APP,gpasTaskApp_StateMachine[ucCurrentStateApp], &ucCurrentStateApp, &stAppMsg);
		}

		vTaskDelayUntil(&elapsed_time, 250 / portTICK_PERIOD_MS);

		if(++ui >= 4)
		{
			ulTimestamp++;
			ui = 0;
			ESP_LOGI(APP_TASK_TAG,"Timestamp=%ld\r\n",ulTimestamp);
			ESP_LOGI(APP_TASK_TAG,"State=%d\r\n",ucCurrentStateApp);
		}
	}
}

