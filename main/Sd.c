/*
 * Sd.c
 *
 *  Created on: 20/09/2018
 *      Author: danilo
 */
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/dirent.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_system.h"

#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"

#include <esp_system.h>
#include <esp_spi_flash.h>
#include <rom/spi_flash.h>
#include <ff.h>

#include "defines.h"
#include "State.h"
#include "Gsm.h"
#include "Wifi.h"
#include "App.h"
#include "Ble.h"
#include "Sd.h"
#include "http_client.h"

//////////////////////////////////////////////
//
//
//            FUNCTION PROTOTYPES
//
//
//////////////////////////////////////////////
void vTaskSd( void *pvParameters );
unsigned char TaskSd_Writing(sMessageType *psMessage);
unsigned char TaskSd_IgnoreEvent(sMessageType *psMessage);
unsigned char ucSdWriteWifiApName(char* a);
unsigned char ucSdAppSource(char* a);
unsigned char ucSdWriteWifiApPassword(char* a);
unsigned char ucSdWritePeriodLogInSec(char* a);
unsigned char ucSdWriteModemPeriodTxInSec(char* a);
unsigned char ucSdWriteModemApn(char* a);
unsigned char ucSdWriteModemApnLogin(char* a);
unsigned char ucSdWriteModemApnPassword(char* a);
unsigned char ucSdWriteTimeToSleep(char* a);
unsigned char ucSdDoNothing(char* a);
unsigned char ucSdWriteState(char* a);

//////////////////////////////////////////////
//
//
//              VARIABLES
//
//
//////////////////////////////////////////////
unsigned long u32TimeToSleep;
unsigned long u32TimeToWakeup;
unsigned long u32PeriodLog = 10;
unsigned long u32PeriodTx;
static unsigned char boBuzzerStatus;

char cConfigHttpRxBuffer[RX_BUF_SIZE];
char cConfigAndData[RX_BUF_SIZE+1];

static long int liFilePointerPositionBeforeReading = 0;
static long int liFilePointerPositionAfterReading = 0;

tstConfiguration stConfigData =
{
	.u32TimeToSleepInSec = (unsigned long)180,
	.u32PeriodLogInSec = (unsigned long)30,
	.cBuzzerStatus = "ON",
	.cWifiApName= "Deco-Gabi",
	.cWifiApPassword = "poliana90",
	.cHttpDomain = "gpslogger.esy.es",
	.cPageUrl = "http://gpslogger.esy.es/pages/upload/index.php",
	.cModemApn = "vodafone.br",
	.cModemApnLogin = " ",
	.cModemApnPassword = " ",
	.u32ModemPeriodTxInSec = (unsigned long)120,
	.cAppSource ="SRC_WIFI",
	.cState = "ARMED"
};

//////////////////////////////////////////////
//
//
//              VARIABLES
//
//
//////////////////////////////////////////////
unsigned long u32TimeToSleep;
unsigned long u32TimeToWakeup;


char szFilenameToBeRead[RX_BUF_SIZE+1];
char  cConfigAndData[RX_BUF_SIZE+1];
char szFilenameToBeWritten[RX_BUF_SIZE+1];
extern char cGpsDataCopiedToSd[RX_BUF_SIZE+1];
extern unsigned char ucCurrentStateGsm;

char cConfigHttpRxBuffer[RX_BUF_SIZE];
char cConfigUartRxBuffer[RX_BUF_SIZE];
char *ptrRxConfig;


/*extern tstIo stIo;*/
sMessageType stSdMsg;

static const char *SD_TASK_TAG = "SD_TASK";

typedef struct{
    unsigned char ucIndex;
    char cIdentifier[RX_BUF_SIZE_REDUCED];
    char cParam1[RX_BUF_SIZE_REDUCED];
    unsigned char (*ucFuncPtr)(char *a);
}tstReadWriteDataByIdentifier;

tstReadWriteDataByIdentifier const astReadWriteTable[] =
{
    { 1, "CONF:","WIFIAPNAME:",ucSdWriteWifiApName},
    { 2, "CONF:","WIFIAPPASSWORD:",ucSdWriteWifiApPassword},
	{ 3, "CONF:","PERIODLOGINSEC:",ucSdWritePeriodLogInSec},
	{ 4, "CONF:","MODEMPERIODTXINSEC:",ucSdWriteModemPeriodTxInSec},
	{ 5, "CONF:","APPSOURCE:",ucSdAppSource},
	{ 6, "CONF:","MODEMAPN:",ucSdWriteModemApn},
	{ 7, "CONF:","MODEMAPNLOGIN:",ucSdWriteModemApnLogin},
	{ 8, "CONF:","MODEMAPNPASSWORD:",ucSdWriteModemApnPassword},
	{ 9, "CONF:","TIMETOSLEEP:",ucSdWriteTimeToSleep},
	{ 10, "CONF:","STATE:",ucSdWriteState},
    { 255, "END OF ARRAY:","END OF ARRAY",ucSdDoNothing}
};


/*extern tstIo stIo;*/
sMessageType stSdMsg;
static char cDataBuffer[32];
extern tstSensorKeylessCode stSensor[MAX_KEYLESS_SENSOR];
extern tstSensorKeylessCode stKeyless[MAX_KEYLESS_SENSOR];
extern tstSensorKeylessCode stTelephone[MAX_KEYLESS_SENSOR];
/*extern tstSensorKeylessCode stSensorKeylessCode[MAX_KEYLESS_SENSOR];*/

extern unsigned long ulTimestamp;

//////////////////////////////////////////////
//
//
//  SearchReadWriteDataByIdentifierFunctions
//
//
//////////////////////////////////////////////
unsigned char SearchReadWriteDataByIdentifierFunctions(char *data, tstReadWriteDataByIdentifier const *pst)
{
    char *ptr,*ptrData = data;
    unsigned char ucResp = false;
    
    ESP_LOGI(SD_TASK_TAG,"\r\nSearchReadWriteDataByIdentifierFunctions\r\n");

    while(pst->ucIndex != 255)
    {
    	/*ESP_LOGI(SD_TASK_TAG,"pst->ucIndex:%d\r\n",pst->ucIndex);*/
        ptr = strstr(ptrData,pst->cIdentifier);
        if(ptr != NULL)
        {
            ptr = strstr(ptrData,pst->cParam1);
            if(ptr != NULL)
            {
            	ptrData +=strlen(pst->cIdentifier);
				ptrData +=strlen(pst->cParam1);
				/*ESP_LOGI(SD_TASK_TAG,"cParam1:%s\r\n",pst->cParam1);
	        	ESP_LOGI(SD_TASK_TAG,"ptrData:%s\r\n",ptrData);*/

				static char cData[128];
				memset(cData,0,sizeof(cData));
				ptr = cData;
				while(*ptrData != '\r')
				{
					*ptr = *ptrData;
					ptr++;
					ptrData++;
				}
				ptrData++;/*"\r"*/
				ptrData++;/*"\n"*/
				/*ESP_LOGI(SD_TASK_TAG,"Data:%s\r\n",cData);*/
				ucResp = pst->ucFuncPtr(cData);
            }                
        }
        pst++;

    }
    return(ucResp);
}
//////////////////////////////////////////////
//
//
//           TaskSd_ReadWriteConfig
//
//
//////////////////////////////////////////////
unsigned char TaskSd_ReadWriteConfig(sMessageType *psMessage)
{
	unsigned char boError = true;
	tstReadWriteDataByIdentifier const *pst = &astReadWriteTable[0];
	
	ESP_LOGI(SD_TASK_TAG,"\r\nTaskSd_ReadWriteConfig=%s\r\n",psMessage->pcMessageData);

	(void)SearchReadWriteDataByIdentifierFunctions(psMessage->pcMessageData, pst);

	return(boError);
}
//////////////////////////////////////////////
//
//
//  			ucSdWriteConfigFile
//
//
//////////////////////////////////////////////
unsigned char ucSdWriteConfigFile(void)
{
	unsigned char boError = true;
	tstConfiguration *pstConfigData = &stConfigData;
	FILE *f;
	char cLocalBuffer[RX_BUF_SIZE];
	
	// Use POSIX and C standard library functions to work with files.
	// First create a file.
	ESP_LOGI(SD_TASK_TAG, "File will be written=%s",(char*)pstConfigData);
	f = fopen("/spiffs/DATASET.TXT", "w");
	if (f == NULL) {
		ESP_LOGE(SD_TASK_TAG, "Failed to open file for writing");
		return(boError);
    }

    fwrite(pstConfigData,sizeof(tstConfiguration),1,f);
    fclose(f);
	
	/*memset(BleDebugMsg,0x00,sizeof(BleDebugMsg));
	strcpy(BleDebugMsg,"Data Written OK!");

	stSdMsg.ucSrc = SRC_SD;
	stSdMsg.ucDest = SRC_BLE;
	stSdMsg.ucEvent = (int)NULL;
	stSdMsg.pcMessageData = (char*)BleDebugMsg;
	xQueueSend(xQueueBle,( void * )&stSdMsg,NULL);
	*/

    // Open renamed file for reading
     ESP_LOGI(SD_TASK_TAG, "Reading Config file");
     f = fopen("/spiffs/DATASET.TXT", "r");
     if (f == NULL)
     {
 	        ESP_LOGE(SD_TASK_TAG, "Failed to open DATASET.TXT for reading");
 	       return(boError);
 	 }

    memset(cLocalBuffer,0x00,sizeof(tstConfiguration));
  	fread (cLocalBuffer,sizeof(tstConfiguration),1,f);
  	fclose(f);
 	ESP_LOGI(SD_TASK_TAG, "Read from file:%s", cLocalBuffer);

 	if(strlen(cLocalBuffer)> 0){
 		pstConfigData = (tstConfiguration*)(cLocalBuffer);
 	    /* TimeToSleep*/
 		stConfigData.u32TimeToSleepInSec = pstConfigData->u32TimeToSleepInSec;
 		/* Period Log*/
 		stConfigData.u32PeriodLogInSec = pstConfigData->u32PeriodLogInSec;
 	    /* Buzzer Status*/
 		if(strlen(pstConfigData->cBuzzerStatus))
 		{
 			memset(stConfigData.cBuzzerStatus,0x00,sizeof(stConfigData.cBuzzerStatus));
 			strcpy(stConfigData.cBuzzerStatus,pstConfigData->cBuzzerStatus);
 		}
 		/* Wifi ApName*/
 		if(strlen(pstConfigData->cWifiApName))
 		{
 			memset(stConfigData.cWifiApName,0x00,sizeof(stConfigData.cWifiApName));
 			strcpy(stConfigData.cWifiApName,pstConfigData->cWifiApName);
 		}
 		/* Wifi Password*/
 		if(strlen(pstConfigData->cWifiApPassword))
 		{
 			memset(stConfigData.cWifiApPassword,0x00,sizeof(stConfigData.cWifiApPassword));
 			strcpy(stConfigData.cWifiApPassword,pstConfigData->cWifiApPassword);
 		}
 		/* Http Domain*/
 		if(strlen(pstConfigData->cHttpDomain))
 		{
 			memset(stConfigData.cHttpDomain,0x00,sizeof(stConfigData.cHttpDomain));
 			strcpy(stConfigData.cHttpDomain,pstConfigData->cHttpDomain);
 		}
 		/* cPageUrl*/
 		if(strlen(pstConfigData->cPageUrl))
 		{
 			memset(stConfigData.cPageUrl,0x00,sizeof(stConfigData.cPageUrl));
 			strcpy(stConfigData.cPageUrl,pstConfigData->cPageUrl);
 		}
 		/* cModemApn*/
 		if(strlen(pstConfigData->cModemApn))
 		{
 			memset(stConfigData.cModemApn,0x00,sizeof(stConfigData.cModemApn));
 			strcpy(stConfigData.cModemApn,pstConfigData->cModemApn);
 		}
 		/* cModemApnLogin*/
 		if(strlen(pstConfigData->cModemApnLogin))
 		{
 			memset(stConfigData.cModemApnLogin,0x00,sizeof(stConfigData.cModemApnLogin));
 			strcpy(stConfigData.cModemApnLogin,pstConfigData->cModemApnLogin);
 		}
 		/* cModemPassword*/
 		if(strlen(pstConfigData->cModemApnPassword))
 		{
 			memset(stConfigData.cModemApnPassword,0x00,sizeof(stConfigData.cModemApnPassword));
 			strcpy(stConfigData.cModemApnPassword,pstConfigData->cModemApnPassword);
 		}
 	    /* u32ModemPeriodTxInSec*/
 		stConfigData.u32ModemPeriodTxInSec = pstConfigData->u32ModemPeriodTxInSec;

 		/* cAppSource*/
 		if(strlen(pstConfigData->cAppSource))
 		{
 			memset(stConfigData.cAppSource,0x00,sizeof(stConfigData.cAppSource));
 			strcpy(stConfigData.cAppSource,pstConfigData->cAppSource);
 		}

 		/* cState*/
 		if(strlen(pstConfigData->cState))
 		{
 			memset(stConfigData.cState,0x00,sizeof(stConfigData.cState));
 			strcpy(stConfigData.cState,pstConfigData->cState);
 		}

 	    ESP_LOGI(SD_TASK_TAG, "\r\nu32TimeToSleepInSec=%d\r\n",(int)stConfigData.u32TimeToSleepInSec);
 	    ESP_LOGI(SD_TASK_TAG, "\r\nu32PeriodLogInSec=%d\r\n",(int)stConfigData.u32PeriodLogInSec);
 	    ESP_LOGI(SD_TASK_TAG, "\r\ncBuzzerStatus=%s\r\n",stConfigData.cBuzzerStatus);
 	    ESP_LOGI(SD_TASK_TAG, "\r\ncWifiAPName=%s\r\ncWifiPwd=%s\r\n",stConfigData.cWifiApName,stConfigData.cWifiApPassword);
 	    ESP_LOGI(SD_TASK_TAG, "\r\ncHttpDomain=%s\r\n",stConfigData.cHttpDomain);
 	    ESP_LOGI(SD_TASK_TAG, "\r\ncPageUrl=%s\r\n",stConfigData.cPageUrl);
 	    ESP_LOGI(SD_TASK_TAG, "\r\ncApn=%s\r\ncApnLogin=%s\r\ncApnPassword=%s\r\n",stConfigData.cModemApn,stConfigData.cModemApnLogin,stConfigData.cModemApnPassword);
 	    ESP_LOGI(SD_TASK_TAG, "\r\nu32ModemPeriodTxInSec=%d\r\n",(int)stConfigData.u32ModemPeriodTxInSec);
 	    ESP_LOGI(SD_TASK_TAG, "\r\ncState=%s\r\n",stConfigData.cState);
 	}

	return(boError);
}

//////////////////////////////////////////////
//
//
//  			ucSdWriteState
//
//
//////////////////////////////////////////////
unsigned char ucSdWriteState(char* a)
{
	unsigned char boError = true;

	ESP_LOGI(SD_TASK_TAG,"\r\n ucSdWriteState:%s",a);

	memset(stConfigData.cState,0x00,sizeof(stConfigData.cState));
	strcpy(stConfigData.cState,a);

	ucSdWriteConfigFile();

	/*esp_restart();*/
    return (boError);
}



//////////////////////////////////////////////
//
//
//  			ucSdWriteWifiApName
//
//
//////////////////////////////////////////////
unsigned char ucSdWriteWifiApName(char* a)
{
	unsigned char boError = true;

	ESP_LOGI(SD_TASK_TAG,"\r\n ucSdWriteWifiApName:%s",a);

	memset(stConfigData.cWifiApName,0x00,sizeof(stConfigData.cWifiApName));
	strcpy(stConfigData.cWifiApName,a);

	ucSdWriteConfigFile();

	/*esp_restart();*/
    return (boError);
}

//////////////////////////////////////////////
//
//
//  			ucSdWriteModemApn
//
//
//////////////////////////////////////////////
unsigned char ucSdWriteModemApn(char* a)
{
	unsigned char boError = true;

	ESP_LOGI(SD_TASK_TAG,"\r\nucSdWriteModemApn\r\n");

	memset(stConfigData.cModemApn,0x00,sizeof(((tstConfiguration){0}).cModemApn));
	strcpy(stConfigData.cModemApn,a);

	ucSdWriteConfigFile();


	/*esp_restart();*/
    return (boError);
}

//////////////////////////////////////////////
//
//
//  			ucSdWriteModemApnLogin
//
//
//////////////////////////////////////////////
unsigned char ucSdWriteModemApnLogin(char* a)
{
	unsigned char boError = true;

	ESP_LOGI(SD_TASK_TAG,"\r\nucSdWriteModemApnLogin:%s\r\n",a);

	memset(stConfigData.cModemApnLogin,0x00,sizeof(((tstConfiguration){0}).cModemApnLogin));
	strcpy(stConfigData.cModemApnLogin,a);

	ESP_LOGI(SD_TASK_TAG,"\r\n stConfigData.cModemApnLogin:%s\r\n",stConfigData.cModemApnLogin);

	ucSdWriteConfigFile();


	/*esp_restart();*/
    return (boError);
}


//////////////////////////////////////////////
//
//
//  			ucSdWriteModemApnPassword
//
//
//////////////////////////////////////////////
unsigned char ucSdWriteModemApnPassword(char* a)
{
	unsigned char boError = true;
	
	ESP_LOGI(SD_TASK_TAG,"\r\nucSdWriteModemApnPassword\r\n");

	memset(stConfigData.cModemApnPassword,0x00,sizeof(stConfigData.cModemApnPassword));
	strcpy(stConfigData.cModemApnPassword,a);
	   
	ucSdWriteConfigFile();
	
	/*esp_restart();*/
    return (boError);
}

//////////////////////////////////////////////
//
//
//  			ucSdWriteTimeToSleep
//
//
//////////////////////////////////////////////
unsigned char ucSdWriteTimeToSleep(char* a)
{
	unsigned char boError = true;
	unsigned long u32;

	ESP_LOGI(SD_TASK_TAG,"\r\nucSdWriteTimeToSleep\r\n");

	u32  = atol(a);

	stConfigData.u32TimeToSleepInSec = u32;

	ucSdWriteConfigFile();

	return (boError);
}


//////////////////////////////////////////////
//
//
//  			ucSdAppSource
//
//
//////////////////////////////////////////////
unsigned char ucSdAppSource(char* a)
{
	unsigned char boError = true;

	ESP_LOGI(SD_TASK_TAG,"\r\nucSdAppSource\r\n");

	memset(stConfigData.cAppSource,0x00,sizeof(stConfigData.cAppSource));
	strcpy(stConfigData.cAppSource,a);

	ucSdWriteConfigFile();

	/*esp_restart();*/
    return (boError);
}

//////////////////////////////////////////////
//
//
//  			ucSdWriteWifiApPassword
//
//
//////////////////////////////////////////////
unsigned char ucSdWriteWifiApPassword(char* a)
{
	unsigned char boError = true;
	

	ESP_LOGI(SD_TASK_TAG,"\r\nucSdWriteWifiApPassword:%s",a);

	memset(stConfigData.cWifiApPassword,0x00,sizeof(stConfigData.cWifiApPassword));
	strcpy(stConfigData.cWifiApPassword,a);	
	   
	ucSdWriteConfigFile();
	
	/*esp_restart();*/
    return (boError);
}

//////////////////////////////////////////////
//
//
//  			ucSdWritePeriodLogInSec
//
//
//////////////////////////////////////////////
unsigned char ucSdWritePeriodLogInSec(char* a)
{
	unsigned char boError = true;
	unsigned long u32;


	ESP_LOGI(SD_TASK_TAG,"\r\nucSdWritePeriodLogInSec\r\n");

	u32  = atol(a);

	stConfigData.u32PeriodLogInSec = u32;

	ucSdWriteConfigFile();

    return (boError);
}
//////////////////////////////////////////////
//
//
//  	ucSdWriteModemPeriodTxInSec
//
//
//////////////////////////////////////////////
unsigned char ucSdWriteModemPeriodTxInSec(char* a)
{
	unsigned char boError = true;
	unsigned long u32;

	ESP_LOGI(SD_TASK_TAG,"\r\nucSdWriteModemPeriodTxInSec\r\n");

	u32  = atol(a);

	stConfigData.u32ModemPeriodTxInSec = u32;

	ucSdWriteConfigFile();

    return (boError);
}

//////////////////////////////////////////////
//
//
//  			ucSdDoNothing
//
//
//////////////////////////////////////////////
unsigned char ucSdDoNothing(char* a)
{
	unsigned char boError = true;
	char *ptr;
	ptr = a;
    return (boError);
}



//////////////////////////////////////////////
//
//
//              Sd_Init
//
//
//////////////////////////////////////////////
void SdInit(void)
{
	/*esp_log_level_set(SD_TASK_TAG, ESP_LOG_INFO);*/

	xQueueSd = xQueueCreate(sdQUEUE_LENGTH,			/* The number of items the queue can hold. */
							sizeof( sMessageType ) );	/* The size of each item the queue holds. */


    xTaskCreate(vTaskSd, "vTaskSd", 1024*4, NULL, configMAX_PRIORITIES, NULL);
	/* Create the queue used by the queue send and queue receive tasks.
	http://www.freertos.org/a00116.html */

	stSdMsg.ucSrc = SRC_SD;
	stSdMsg.ucDest = SRC_SD;
	stSdMsg.ucEvent = EVENT_SD_INIT;
	xQueueSend( xQueueSd, ( void * )&stSdMsg, 0);
}

//////////////////////////////////////////////
//
//
//              TaskSd_Init
//
//
//////////////////////////////////////////////
unsigned char TaskSd_Init(sMessageType *psMessage)
{
	unsigned char boError = true;

	char cLocalBuffer[RX_BUF_SIZE];
	tstConfiguration *pstConfigData = &stConfigData;

    ESP_LOGI(SD_TASK_TAG, "INIT\r\n");
    ESP_LOGI(SD_TASK_TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(SD_TASK_TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(SD_TASK_TAG, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(SD_TASK_TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        boError = false;
        return(boError);
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE(SD_TASK_TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGI(SD_TASK_TAG, "Partition size: total: %d, used: %d", total, used);
    }

    /*********************************************
     *				DELETE FILE
     *********************************************/
#if 0
    if (remove("/spiffs/KEYLESS.TXT") == 0)
    	ESP_LOGI(SD_TASK_TAG,"Deleted successfully");
     else
    	 ESP_LOGI(SD_TASK_TAG,"Unable to delete the file");
#endif

#if 1
    /*********************************************
     *		READ FILES INSIDE FOLDER
     *********************************************/
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir ("/spiffs/")) != NULL)
    {
		/* print all the files and directories within directory */
		while ((ent = readdir (dir)) != NULL)
		{
		  ESP_LOGI(SD_TASK_TAG,"%s\n", ent->d_name);
		}
		closedir (dir);
    }
    else
    {
      /* could not open directory */
    	ESP_LOGE(SD_TASK_TAG, "Error opening directory");
        boError = false;
        return(boError);
    }
#endif

    FILE *f;

#if 1
    /*********************************************
     *				DELETE FILE
     *********************************************/
    if (remove("/spiffs/CONFIG.TXT") == 0)
    	ESP_LOGI(SD_TASK_TAG,"Deleted successfully");
     else
		ESP_LOGI(SD_TASK_TAG,"Unable to delete the file");
#endif


		/*********************************************
		*			READING KEYLESS FILE			 *
		*********************************************/
	    // Open renamed file for reading
	    ESP_LOGI(SD_TASK_TAG, "Reading Keyless file");
	    f = fopen("/spiffs/KEYLESS.TXT", "r");
	    if (f == NULL)
	    {
		    fclose(f);
			ESP_LOGE(SD_TASK_TAG, "Failed to open file for reading");
		    f = fopen("/spiffs/KEYLESS.TXT", "w");
		    if (f == NULL) {
		        ESP_LOGE(SD_TASK_TAG, "Failed to open file for writing");
		    }
		    fclose(f);
	    }
	    else{
		    memset(cLocalBuffer,0,RX_BUF_SIZE);
			fread (cLocalBuffer,1,RX_BUF_SIZE,f);
			fclose(f);
	    }

		ESP_LOGI(SD_TASK_TAG, "Read from file:%s", cLocalBuffer);
		char *pch;
		int i = 0;

		pch = strtok(cLocalBuffer,",\r\n");
		if(pch != NULL)
		{
			strcpy(stKeyless[i].cId,pch);
			ESP_LOGI(SD_TASK_TAG, "stKeyless[%d].cId:%s",i,stKeyless[i].cId);

			while ((pch != NULL) && (i < MAX_KEYLESS_SENSOR))
			{
				/*ESP_LOGI(SD_TASK_TAG, "p:%s",pch);*/

				pch = strtok(NULL,",\r\n");
				if(pch == NULL) break;
				strcpy(stKeyless[i].cCode,pch);
				ESP_LOGI(SD_TASK_TAG, "stKeyless[%d].cCode:%s",i,stKeyless[i].cCode);

				i++;

				pch = strtok(NULL,",\r\n");
				if(pch == NULL) break;
				strcpy(stKeyless[i].cId,pch);
				ESP_LOGI(SD_TASK_TAG, "stKeyless[%d].cId:%s",i,stKeyless[i].cId);
			}
		}

	    /*********************************************
	     *			READING SENSOR FILE
	     *********************************************/
	    // Open renamed file for reading
	    ESP_LOGI(SD_TASK_TAG, "Reading Sensor file");
	    f = fopen("/spiffs/SENSOR.TXT", "r");
	    if (f == NULL)
	    {
		    fclose(f);
			ESP_LOGE(SD_TASK_TAG, "Failed to open file for reading");
		    f = fopen("/spiffs/SENSOR.TXT", "w");
		    if (f == NULL) {
		        ESP_LOGE(SD_TASK_TAG, "Failed to open file for writing");
		    }
		    fclose(f);
	    }
	    else{
		    memset(cLocalBuffer,0,RX_BUF_SIZE);
			fread (cLocalBuffer,1,RX_BUF_SIZE,f);
			fclose(f);
	    }

		ESP_LOGI(SD_TASK_TAG, "Read from file: %s", cLocalBuffer);


		i = 0;
		pch = strtok(cLocalBuffer,",\r\n");
		if(pch != NULL)
		{
			strcpy(stSensor[i].cId,pch);
			ESP_LOGI(SD_TASK_TAG, "stSensor[%d].cId:%s",i,stSensor[i].cId);

			while ((pch != NULL) && (i < MAX_KEYLESS_SENSOR))
			{
				/*ESP_LOGI(SD_TASK_TAG, "p:%s",pch);*/

				pch = strtok(NULL,",\r\n");
				if(pch == NULL) break;
				strcpy(stSensor[i].cCode,pch);
				ESP_LOGI(SD_TASK_TAG, "stSensor[%d].cCode:%s",i,stSensor[i].cCode);

				i++;

				pch = strtok(NULL,",\r\n");
				if(pch == NULL) break;
				strcpy(stSensor[i].cId,pch);
				ESP_LOGI(SD_TASK_TAG, "stSensor[%d].cId:%s",i,stSensor[i].cId);
			}
		}


	    /*********************************************
	     *			READING TELEPHONE FILE
	     *********************************************/
	    // Open renamed file for reading
	    ESP_LOGI(SD_TASK_TAG, "Reading Telephone file");
	    f = fopen("/spiffs/TELEPHONE.TXT", "r");
	    if (f == NULL)
	    {
		    fclose(f);
			ESP_LOGE(SD_TASK_TAG, "Failed to open file for reading");
		    f = fopen("/spiffs/TELEPHONE.TXT", "w");
		    if (f == NULL) {
		        ESP_LOGE(SD_TASK_TAG, "Failed to open file for writing");
		    }
		    fclose(f);
	    }
	    else{
		    memset(cLocalBuffer,0,RX_BUF_SIZE);
			fread (cLocalBuffer,1,RX_BUF_SIZE,f);
			fclose(f);
	    }

		ESP_LOGI(SD_TASK_TAG, "Read from file: %s", cLocalBuffer);
		i = 0;
		pch = strtok(cLocalBuffer,",\r\n");
		if(pch != NULL)
		{
			strcpy(stTelephone[i].cId,pch);
			ESP_LOGI(SD_TASK_TAG, "stTelephone[%d].cId:%s",i,stTelephone[i].cId);

			while ((pch != NULL) && (i < MAX_KEYLESS_SENSOR))
			{
				pch = strtok(NULL,",\r\n");
				if(pch == NULL) break;
				strcpy(stTelephone[i].cCode,pch);
				ESP_LOGI(SD_TASK_TAG, "stTelephone[%d].cCode:%s",i,stTelephone[i].cCode);

				i++;

				pch = strtok(NULL,",\r\n");
				if(pch == NULL) break;
				strcpy(stTelephone[i].cId,pch);
				ESP_LOGI(SD_TASK_TAG, "stTelephone[%d].cId:%s",i,stTelephone[i].cId);
			}

		}

	#if 0
	    /*********************************************
	     *			UNMOUNT SPIFFS
	     *********************************************/
	    // All done, unmount partition and disable SPIFFS
	    esp_vfs_spiffs_unregister(NULL);
	    ESP_LOGI(SD_TASK_TAG, "SPIFFS unmounted");
	#endif

    /*********************************************
     *			READING CONFIG FILE
     *********************************************/
    // Open renamed file for reading
    ESP_LOGI(SD_TASK_TAG, "Reading Config file");
    f = fopen("/spiffs/DATASET.TXT", "r");
    if (f == NULL)
    {
	    fclose(f);
		ESP_LOGE(SD_TASK_TAG, "Failed to open file for reading");
	    f = fopen("/spiffs/DATASET.TXT", "w");
	    if (f == NULL) {
	        ESP_LOGE(SD_TASK_TAG, "Failed to open file for writing");
	    }
    }

    memset(cLocalBuffer,0x00,sizeof(tstConfiguration));
	fread (cLocalBuffer,sizeof(tstConfiguration),1,f);
	fclose(f);
	if(strlen(cLocalBuffer) == 0)
	{
		ESP_LOGE(SD_TASK_TAG, "File is empty");
		/*********************************************
		 *			WRITING CONFIG FILE
		 *********************************************/
		boError = ucSdWriteConfigFile();
		if (boError != true)
		{
			return(boError);
		}
	}


    f = fopen("/spiffs/DATASET.TXT", "r");
    if (f != NULL)
    {
        memset(cLocalBuffer,0x00,sizeof(tstConfiguration));
    	i = fread (cLocalBuffer,sizeof(tstConfiguration),1,f);
    	fclose(f);
    	if(strlen(cLocalBuffer)> 0)
    	{
    		ESP_LOGI(SD_TASK_TAG, "Reading inside DATASET.TXT");

    		pstConfigData = (tstConfiguration*)(cLocalBuffer);
    	    /* TimeToSleep*/
    		stConfigData.u32TimeToSleepInSec = pstConfigData->u32TimeToSleepInSec;
    		/* Period Log*/
    		stConfigData.u32PeriodLogInSec = pstConfigData->u32PeriodLogInSec;
    	    /* Buzzer Status*/
    		if(strlen(pstConfigData->cBuzzerStatus))
    		{
    			memset(stConfigData.cBuzzerStatus,0x00,sizeof(stConfigData.cBuzzerStatus));
    			strcpy(stConfigData.cBuzzerStatus,pstConfigData->cBuzzerStatus);
    		}
    		/* Wifi ApName*/
    		if(strlen(pstConfigData->cWifiApName))
    		{
    			memset(stConfigData.cWifiApName,0x00,sizeof(stConfigData.cWifiApName));
    			strcpy(stConfigData.cWifiApName,pstConfigData->cWifiApName);
    		}
    		/* Wifi Password*/
    		if(strlen(pstConfigData->cWifiApPassword))
    		{
    			memset(stConfigData.cWifiApPassword,0x00,sizeof(stConfigData.cWifiApPassword));
    			strcpy(stConfigData.cWifiApPassword,pstConfigData->cWifiApPassword);
    		}
    		/* Http Domain*/
    		if(strlen(pstConfigData->cHttpDomain))
    		{
    			memset(stConfigData.cHttpDomain,0x00,sizeof(stConfigData.cHttpDomain));
    			strcpy(stConfigData.cHttpDomain,pstConfigData->cHttpDomain);
    		}
    		/* cPageUrl*/
    		if(strlen(pstConfigData->cPageUrl))
    		{
    			memset(stConfigData.cPageUrl,0x00,sizeof(stConfigData.cPageUrl));
    			strcpy(stConfigData.cPageUrl,pstConfigData->cPageUrl);
    		}
    		/* cModemApn*/
    		if(strlen(pstConfigData->cModemApn))
    		{
    			memset(stConfigData.cModemApn,0x00,sizeof(stConfigData.cModemApn));
    			strcpy(stConfigData.cModemApn,pstConfigData->cModemApn);
    		}
    		/* cModemApnLogin*/
    		if(strlen(pstConfigData->cModemApnLogin))
    		{
    			memset(stConfigData.cModemApnLogin,0x00,sizeof(stConfigData.cModemApnLogin));
    			strcpy(stConfigData.cModemApnLogin,pstConfigData->cModemApnLogin);
    		}
    		/* cModemPassword*/
    		if(strlen(pstConfigData->cModemApnPassword))
    		{
    			memset(stConfigData.cModemApnPassword,0x00,sizeof(stConfigData.cModemApnPassword));
    			strcpy(stConfigData.cModemApnPassword,pstConfigData->cModemApnPassword);
    		}
    	    /* u32ModemPeriodTxInSec*/
    		stConfigData.u32ModemPeriodTxInSec = pstConfigData->u32ModemPeriodTxInSec;

    		/* cAppSource*/
    		if(strlen(pstConfigData->cAppSource))
    		{
    			memset(stConfigData.cAppSource,0x00,sizeof(stConfigData.cAppSource));
    			strcpy(stConfigData.cAppSource,pstConfigData->cAppSource);
    		}

    		/* cState*/
    		if(strlen(pstConfigData->cState))
    		{
    			memset(stConfigData.cState,0x00,sizeof(stConfigData.cState));
    			strcpy(stConfigData.cState,pstConfigData->cState);
    		}


    	    ESP_LOGI(SD_TASK_TAG, "\r\nu32TimeToSleepInSec=%d\r\n",(int)stConfigData.u32TimeToSleepInSec);
    	    ESP_LOGI(SD_TASK_TAG, "\r\nu32PeriodLogInSec=%d\r\n",(int)stConfigData.u32PeriodLogInSec);
    	    ESP_LOGI(SD_TASK_TAG, "\r\ncBuzzerStatus=%s\r\n",stConfigData.cBuzzerStatus);
    	    ESP_LOGI(SD_TASK_TAG, "\r\ncWifiAPName=%s\r\ncWifiPwd=%s\r\n",stConfigData.cWifiApName,stConfigData.cWifiApPassword);
    	    ESP_LOGI(SD_TASK_TAG, "\r\ncHttpDomain=%s\r\n",stConfigData.cHttpDomain);
    	    ESP_LOGI(SD_TASK_TAG, "\r\ncPageUrl=%s\r\n",stConfigData.cPageUrl);
    	    ESP_LOGI(SD_TASK_TAG, "\r\ncApn=%s\r\ncApnLogin=%s\r\ncApnPassword=%s\r\n",stConfigData.cModemApn,stConfigData.cModemApnLogin,stConfigData.cModemApnPassword);
    	    ESP_LOGI(SD_TASK_TAG, "\r\nu32ModemPeriodTxInSec=%d\r\n",(int)stConfigData.u32ModemPeriodTxInSec);
    	    ESP_LOGI(SD_TASK_TAG, "\r\ncState=%s\r\n",stConfigData.cState);
    	}
    }
	stSdMsg.ucSrc = SRC_SD;
	stSdMsg.ucDest = SRC_SD;
	stSdMsg.ucEvent = EVENT_SD_OPENING;
	xQueueSend( xQueueSd, ( void * )&stSdMsg, 0);


    return(boError);
}

//////////////////////////////////////////////
//
//
//              TaskSd_Opening
//
//
//////////////////////////////////////////////
unsigned char TaskSd_Opening(sMessageType *psMessage)
{
	unsigned char boError = true;

	FILE *f;
	char cLocalBuffer[1024];
	#if DEBUG_SDCARD
    ESP_LOGI(SD_TASK_TAG, "<<<<OPENING>>>>\r\n\r\n<<<<>>>>\r\n");
	#endif


    /*********************************************
     *		READ FILES INSIDE FOLDER
     *********************************************/
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir ("/spiffs/")) != NULL)
    {
		/* print all the files and directories within directory */
		for(;;)
		{
			if((ent = readdir (dir)) != NULL)
			{
				ESP_LOGI(SD_TASK_TAG,"%s,FileNumber=%d\n", ent->d_name,ent->d_ino);

				if(strstr(ent->d_name,"DATA_") != NULL)
				{
					memset(cLocalBuffer,0,RX_BUF_SIZE);

					strcpy(cLocalBuffer,"/spiffs/");
					strcat(cLocalBuffer,ent->d_name);

					f = fopen((const char*)cLocalBuffer, "r");
					fclose(f);
				    if(f == NULL )
				    {
						ESP_LOGE(SD_TASK_TAG, "Failed to open file for reading");
						boError = false;
				    }
				    else
				    {
						strcpy(szFilenameToBeRead,cLocalBuffer);

						liFilePointerPositionBeforeReading = 0;
						liFilePointerPositionAfterReading = 0;

						stSdMsg.ucSrc = SRC_SD;
						stSdMsg.ucDest = SRC_SD;
						stSdMsg.ucEvent = EVENT_SD_READING;
						xQueueSend( xQueueSd, ( void * )&stSdMsg, 0);
				    }
					break;
				}
			}
			else
			{
				ESP_LOGE(SD_TASK_TAG,"No more DATA files\n");
		        boError = false;
#if 0
				stSdMsg.ucSrc = SRC_SD;
				stSdMsg.ucDest = SRC_GSM;
				stSdMsg.ucEvent = EVENT_GSM_ENDING;
				xQueueSend( xQueueGsm, ( void * )&stSdMsg, 0);

				stSdMsg.ucSrc = SRC_SD;
				stSdMsg.ucDest = SRC_WIFI;
				stSdMsg.ucEvent = EVENT_WIFI_ENDING;
				xQueueSend( xQueueWifi, ( void * )&stSdMsg, 0);
#endif

				stSdMsg.ucSrc = SRC_SD;
				stSdMsg.ucDest = SRC_HTTPCLI;
				stSdMsg.ucEvent = EVENT_HTTPCLI_ENDING;
				xQueueSend( xQueueHttpCli, ( void * )&stSdMsg, 0);

				break;
			}
		}
		closedir (dir);
    }
    else
    {
      /* could not open directory */
    	ESP_LOGE(SD_TASK_TAG, "Error opening directory");
        boError = false;
    }
	return(boError);
}

//////////////////////////////////////////////
//
//
//              TaskSd_Writing
//
//
//////////////////////////////////////////////
unsigned char TaskSd_Writing(sMessageType *psMessage)
{
	unsigned char boError = true;
	FILE *f;
	struct tm  ts;
	char cLocalBuffer[32];


#if DEBUG_SDCARD
    ESP_LOGI(SD_TASK_TAG, "<<<<WRITING>>>>\r\n");
#endif

	// Format time, "ddd yyyy-mm-dd hh:mm:ss zzz"
	ts = *localtime((time_t*)&ulTimestamp);
	strftime(cLocalBuffer, sizeof(cLocalBuffer), "%Y%m%d%H", &ts);

	memset(szFilenameToBeWritten,0x00,sizeof(szFilenameToBeWritten));
	strcpy(szFilenameToBeWritten,"/spiffs/DATA_");
	strcat(szFilenameToBeWritten,cLocalBuffer);
	strcat(szFilenameToBeWritten,".TXT");


#if 0
    f = fopen((const char*)szFilenameToBeWritten, "w+" );
#if DEBUG_SDCARD
    ESP_LOGI(SD_TASK_TAG, "DATA GPS:%s",cGpsDataCopiedToSd);
#endif
	fprintf(f,"%s",(const char*)"DANILO");
	fclose(f);

    f = fopen((const char*)szFilenameToBeWritten, "r" );
	char* cLocalBuffer = (char*) malloc(RX_BUF_SIZE+1);
	memset(cLocalBuffer,0,RX_BUF_SIZE+1);
	fread (cLocalBuffer,1,RX_BUF_SIZE,f);
	/*fgets (cLocalBuffer,RX_BUF_SIZE,f);*/

#if DEBUG_SDCARD
    ESP_LOGI(SD_TASK_TAG, "DATA:%s",cLocalBuffer);
#endif
	free(cLocalBuffer);
	(void)fclose(f);
#endif

	unsigned char mac[6];
	(void)esp_efuse_mac_get_default(&mac[0]);
	ESP_LOGI(SD_TASK_TAG, "MAC=%02x:%02x:%02x:%02x:%02x:%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);


	f = fopen(szFilenameToBeWritten, "a" );
	if(f != NULL)
	{
#if DEBUG_SDCARD
    ESP_LOGI(SD_TASK_TAG, "Reading for update...");
#endif

		if(strlen(psMessage->pcMessageData)>0)
		{
		    /* Format File: R=SENSORX,timestamp\r\n*/
			int i = fprintf(f,"R=%02X%02X%02X%02X%02X%02X,%s,%ld\r\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],psMessage->pcMessageData,ulTimestamp);
			if(i > 0)
			{
				#if DEBUG_SDCARD
				ESP_LOGI(SD_TASK_TAG, "Writing OK!");
				#endif
			}
			else
			{
				#if DEBUG_SDCARD
				ESP_LOGI(SD_TASK_TAG, "Writing NOK!");
				#endif
			}
		}
		fclose(f);
	}
	else
	{
		f = fopen(szFilenameToBeWritten, "w");
		if(f != NULL)
		{
			if(strlen(psMessage->pcMessageData)>0)
			{
				int i = fprintf(f,"R=%02X%02X%02X%02X%02X%02X,%s,%ld\r\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],psMessage->pcMessageData,ulTimestamp);
				if(i > 0)
				{
					#if DEBUG_SDCARD
					ESP_LOGI(SD_TASK_TAG, "Writing new file");
					#endif
				}
				else
				{
					#if DEBUG_SDCARD
					ESP_LOGE(SD_TASK_TAG, "Error writing new file");
					#endif
				}
			}
			fclose(f);
		}
	}
	return(boError);
}

//////////////////////////////////////////////
//
//
//              TaskSd_Programming
//
//
//////////////////////////////////////////////
unsigned char TaskSd_Programming(sMessageType *psMessage)
{
	unsigned char boError = true;
	FILE *f;

#if DEBUG_SDCARD
    ESP_LOGI(SD_TASK_TAG, "<<<<PROGRAMMING>>>>");
#endif


#if 0
    f = fopen((const char*)szFilenameToBeWritten, "w+" );
#if DEBUG_SDCARD
    ESP_LOGI(SD_TASK_TAG, "DATA GPS:%s",cGpsDataCopiedToSd);
#endif
	fprintf(f,"%s",(const char*)"DANILO");
	fclose(f);

    f = fopen((const char*)szFilenameToBeWritten, "r" );
	char* cLocalBuffer = (char*) malloc(RX_BUF_SIZE+1);
	memset(cLocalBuffer,0,RX_BUF_SIZE+1);
	fread (cLocalBuffer,1,RX_BUF_SIZE,f);
	/*fgets (cLocalBuffer,RX_BUF_SIZE,f);*/

#if DEBUG_SDCARD
    ESP_LOGI(SD_TASK_TAG, "DATA:%s",cLocalBuffer);
#endif
	free(cLocalBuffer);
	(void)fclose(f);
#endif

	char *pch;
	char cDataId[16],cDataCode[16],cFilename[48];
	tstSensorKeylessCode *pst = NULL;

	pch = strtok(psMessage->pcMessageData,",");
	strcpy(cFilename,pch);

	pch = strtok(NULL,",");
	strcpy(cDataId,pch);

	pch = strtok(NULL,"\r\n");
	strcpy(cDataCode,pch);
#if DEBUG_SDCARD
		ESP_LOGI(SD_TASK_TAG, "Filename:%s",cFilename);
		ESP_LOGI(SD_TASK_TAG, "Id:%s",cDataId);
		ESP_LOGI(SD_TASK_TAG, "Code:%s",cDataCode);
#endif


	for(int i = 0; i < MAX_KEYLESS_SENSOR; i++)
	{
		if( strstr(cFilename,"KEYLESS.TXT") != NULL)
		{
			pst = &stKeyless[i];
		}
		else
		{
			if( strstr(cFilename,"SENSOR.TXT") != NULL)
			{
				pst = &stSensor[i];
			}
			else
			{
				if( strstr(cFilename,"TELEPHONE.TXT") != NULL)
				{
					pst = &stTelephone[i];
				}
			}
		}

		#if DEBUG_SDCARD
			ESP_LOGI(SD_TASK_TAG, "Code[%d]:%s",i,pst->cCode);
		#endif

		if(strncmp(pst->cCode,cDataCode,7) == 0)
		{
			#if DEBUG_SDCARD
				ESP_LOGI(SD_TASK_TAG, "Code already recorded\r\n");
			#endif
				memset(cDataBuffer,0,sizeof(cDataBuffer));
				strcpy(cDataBuffer,"CODE ALREADY EXISTS");

				/*stSdMsg.ucSrc = SRC_SD;
				stSdMsg.ucDest = SRC_BLE;
				stSdMsg.ucEvent = (int)NULL;
				stSdMsg.pcMessageData = &cDataBuffer[0];
				xQueueSend(xQueueBle,( void * )&stSdMsg,NULL);*/

				stSdMsg.ucSrc = SRC_SD;
				stSdMsg.ucDest = SRC_APP;
				stSdMsg.ucEvent = EVENT_APP_REMOTE_RECORDED;
				stSdMsg.u32MessageData = 0;/* Not Recorded*/
				xQueueSend(xQueueApp,( void * )&stSdMsg,NULL);

				return(boError);
		}
	}

	f = fopen(cFilename, "a" );
	if(f != NULL)
	{
#if DEBUG_SDCARD
    ESP_LOGI(SD_TASK_TAG, "Reading for update...");
#endif

		if(strlen(cDataCode)>0)
		{
			int i = fprintf(f,"%s,%s\r\n",cDataId,cDataCode);
			if(i > 0)
			{
				#if DEBUG_SDCARD
				ESP_LOGI(SD_TASK_TAG, "Programming OK!");
				#endif

				/*memset(cDataBuffer,0,sizeof(cDataBuffer));
				strcpy(cDataBuffer,"RECORDED SUCCESFULLY!");

				stSdMsg.ucSrc = SRC_SD;
				stSdMsg.ucDest = SRC_BLE;
				stSdMsg.ucEvent = (int)NULL;
				stSdMsg.pcMessageData = &cDataBuffer[0];
				xQueueSend(xQueueBle,( void * )&stSdMsg,NULL);*/

				stSdMsg.ucSrc = SRC_SD;
				stSdMsg.ucDest = SRC_APP;
				stSdMsg.ucEvent = EVENT_APP_REMOTE_RECORDED;
				stSdMsg.u32MessageData = 1;/* Recorded Successfully*/
				xQueueSend(xQueueApp,( void * )&stSdMsg,NULL);
			}
			else
			{
				#if DEBUG_SDCARD
				ESP_LOGI(SD_TASK_TAG, "Programming NOK!");
				#endif
			}
		}
		fclose(f);
	}
	else
	{
		f = fopen(cFilename, "r");
		if(f != NULL)
		{
			if(strlen(cDataCode)>0)
			{
				int i = fprintf(f,"%s,%s\r\n",cDataId,cDataCode);
				if(i > 0)
				{
					#if DEBUG_SDCARD
					ESP_LOGI(SD_TASK_TAG, "Programming new file OK");
					#endif

					memset(cDataBuffer,0,sizeof(cDataBuffer));
					strcpy(cDataBuffer,"RECORDED SUCCESFULLY!");

					/*stSdMsg.ucSrc = SRC_SD;
					stSdMsg.ucDest = SRC_BLE;
					stSdMsg.ucEvent = (int)NULL;
					stSdMsg.pcMessageData = &cDataBuffer[0];
					xQueueSend(xQueueBle,( void * )&stSdMsg,NULL);*/

					stSdMsg.ucSrc = SRC_SD;
					stSdMsg.ucDest = SRC_APP;
					stSdMsg.ucEvent = EVENT_APP_REMOTE_RECORDED;
					stSdMsg.u32MessageData = 1;/* Recorded Successfully*/
					xQueueSend(xQueueApp,( void * )&stSdMsg,NULL);

				}
				else
				{
					#if DEBUG_SDCARD
					ESP_LOGE(SD_TASK_TAG, "Error programming new file");
					#endif
				}
			}
            //fflush(f);    // flushing or repositioning required
			fclose(f);
		}
		else
		{
			#if DEBUG_SDCARD
			ESP_LOGI(SD_TASK_TAG, "Strange Error, pls investigate");
			#endif
		}
	}


    /*********************************************
     *			READING  FILE
     *********************************************/
    // Open renamed file for reading
	char* cLocalBuffer = (char*) malloc(RX_BUF_SIZE+1);
    f = fopen(cFilename, "r");
    if (f == NULL)
    {
		ESP_LOGE(SD_TASK_TAG, "Failed to open file for reading");
		boError = false;
		free(cLocalBuffer);
		return(boError);
    }

    memset(cLocalBuffer,0,RX_BUF_SIZE+1);
	fread (cLocalBuffer,1,RX_BUF_SIZE,f);
	fclose(f);
	ESP_LOGI(SD_TASK_TAG, "Read from file:%s", cLocalBuffer);

	int i = 0;
	pch = strtok(cLocalBuffer,",\r\n");
	if(pch != NULL)
	{
		if( strstr(cFilename,"KEYLESS.TXT") != NULL)
		{
			strcpy(stKeyless[i].cId,pch);
			ESP_LOGI(SD_TASK_TAG, "stKeyless[%d].cId:%s",i,stKeyless[i].cId);
		}
		else
		{
			if( strstr(cFilename,"SENSOR.TXT") != NULL)
			{
				strcpy(stSensor[i].cId,pch);
				ESP_LOGI(SD_TASK_TAG, "stSensor[%d].cId:%s",i,stSensor[i].cId);
			}
			else
			{
				if( strstr(cFilename,"TELEPHONE.TXT") != NULL)
				{
					strcpy(stTelephone[i].cId,pch);
					ESP_LOGI(SD_TASK_TAG, "stTelephone[%d].cId:%s",i,stTelephone[i].cId);
				}
			}
		}
	}
	while ((pch != NULL) && (i < MAX_KEYLESS_SENSOR))
	{
		/*ESP_LOGI(SD_TASK_TAG, "p:%s",pch);*/

		pch = strtok(NULL,",\r\n");
		if(pch == NULL) break;

		if( strstr(cFilename,"KEYLESS.TXT") != NULL)
		{
			strcpy(stKeyless[i].cCode,pch);
			ESP_LOGI(SD_TASK_TAG, "stKeyless[%d].cCode:%s",i,stKeyless[i].cCode);

			i++;

			pch = strtok(NULL,",\r\n");
			if(pch == NULL) break;
			strcpy(stKeyless[i].cId,pch);
			ESP_LOGI(SD_TASK_TAG, "stKeyless[%d].cId:%s",i,stKeyless[i].cId);
		}
		else
		{
			if( strstr(cFilename,"SENSOR.TXT") != NULL)
			{
				strcpy(stSensor[i].cCode,pch);
				ESP_LOGI(SD_TASK_TAG, "stSensor[%d].cCode:%s",i,stSensor[i].cCode);

				i++;

				pch = strtok(NULL,",\r\n");
				if(pch == NULL) break;
				strcpy(stSensor[i].cId,pch);
				ESP_LOGI(SD_TASK_TAG, "stSensor[%d].cId:%s",i,stSensor[i].cId);
			}
			else
			{
				if( strstr(cFilename,"TELEPHONE.TXT") != NULL)
				{
					strcpy(stTelephone[i].cCode,pch);
					ESP_LOGI(SD_TASK_TAG, "stTelephone[%d].cCode:%s",i,stTelephone[i].cCode);

					i++;

					pch = strtok(NULL,",\r\n");
					if(pch == NULL) break;
					strcpy(stTelephone[i].cId,pch);
					ESP_LOGI(SD_TASK_TAG, "stTelephone[%d].cId:%s",i,stTelephone[i].cId);
				}
			}
		}
	}

	free(cLocalBuffer);
	return(boError);
}

//////////////////////////////////////////////
//
//
//              TaskSd_Reading
//
//
//////////////////////////////////////////////
unsigned char TaskSd_Reading(sMessageType *psMessage)
{
	unsigned char boError = true;

	char* cLocalBuffer = (char*) malloc(RX_BUF_SIZE+1);
	FILE *f;

#if DEBUG_SDCARD
	ESP_LOGI(SD_TASK_TAG, "<<<<READING>>>>\r\n%s\r\n<<<<>>>>\r\n",szFilenameToBeRead);
#endif


	f = fopen(szFilenameToBeRead, "r");
	if(f != NULL)
	{
		#if DEBUG_SDCARD
			ESP_LOGI(SD_TASK_TAG, "Opening for Reading OK!");
		#endif

		for(;;)
		{
			fseek(f,liFilePointerPositionBeforeReading,SEEK_SET);

			#if DEBUG_SDCARD
			ESP_LOGI(SD_TASK_TAG, "PositionBeforeReading=%ld\r\n",liFilePointerPositionBeforeReading);
			#endif

			/*Reading file until finds \n or RX_BUF_SIZE, whichever happens first*/
			memset(cLocalBuffer,0,RX_BUF_SIZE+1);
			int i;
			if((fgets ( cLocalBuffer, RX_BUF_SIZE, f )) != NULL )
			{

				liFilePointerPositionAfterReading =  ftell (f);

				#if DEBUG_SDCARD
				ESP_LOGI(SD_TASK_TAG, "PositionAfterReading=%ld\r\n",liFilePointerPositionAfterReading);
				#endif

				i = strlen(cLocalBuffer);

				ESP_LOGI(SD_TASK_TAG, "Content of file with %d Bytes: %s",i, cLocalBuffer);
				/* Verify if file is OK*/
				if((cLocalBuffer[0] == 'R') && (cLocalBuffer[1] == '=') && (cLocalBuffer[i-1] == '\n') && (cLocalBuffer[i-2] == '\r'))
				{
					memset(cConfigAndData,0,RX_BUF_SIZE+1);
					strcpy(cConfigAndData,cLocalBuffer);
#if 0
					stSdMsg.ucSrc = SRC_SD;
					stSdMsg.ucDest = SRC_GSM;
					stSdMsg.ucEvent = EVENT_GSM_SEND_HTTPPREPAREDATA;
					stSdMsg.pcMessageData = "";
					xQueueSend( xQueueGsm, ( void * )&stSdMsg, 0);

					stSdMsg.ucSrc = SRC_SD;
					stSdMsg.ucDest = SRC_WIFI;
					stSdMsg.ucEvent = EVENT_WIFI_SEND_START_PAYLOAD/*EVENT_WIFI_SEND_CIPSTATUS*/;
					stSdMsg.pcMessageData = "";
					xQueueSend( xQueueWifi, ( void * )&stSdMsg, 0);
#endif

					stSdMsg.ucSrc = SRC_SD;
					stSdMsg.ucDest = SRC_HTTPCLI;
					stSdMsg.ucEvent = EVENT_HTTPCLI_POST;
					stSdMsg.pcMessageData = "";
					xQueueSend( xQueueHttpCli, ( void * )&stSdMsg, 0);


					fclose(f);
					break;
				}
				else
				{
					liFilePointerPositionBeforeReading = liFilePointerPositionAfterReading;
				}
			}
			else
			{
				#if DEBUG_SDCARD
					ESP_LOGE(SD_TASK_TAG, "No possible to retrieve data");
				#endif

				if(feof(f))
				{
					ESP_LOGI(SD_TASK_TAG, "End of File");

					int ret = remove(szFilenameToBeRead);

					if(ret == 0)
					{
						#if DEBUG_SDCARD
						ESP_LOGE(SD_TASK_TAG, "File deleted successfully");
						#endif
					 }
					else
					{
						#if DEBUG_SDCARD
						ESP_LOGE(SD_TASK_TAG, "Error: unable to delete the file");
						#endif
					}
#if 0
					stSdMsg.ucSrc = SRC_SD;
					stSdMsg.ucDest = SRC_GSM;
					stSdMsg.ucEvent = EVENT_GSM_ENDING/*EVENT_GSM_LIST_SMS_MSG*/;
					xQueueSend( xQueueGsm, ( void * )&stSdMsg, 0);


					stSdMsg.ucSrc = SRC_SD;
					stSdMsg.ucDest = SRC_WIFI;
					stSdMsg.ucEvent = EVENT_WIFI_ENDING/*EVENT_GSM_LIST_SMS_MSG*/;
					xQueueSend( xQueueWifi, ( void * )&stSdMsg, 0);
#endif
					stSdMsg.ucSrc = SRC_SD;
					stSdMsg.ucDest = SRC_HTTPCLI;
					stSdMsg.ucEvent = EVENT_HTTPCLI_ENDING;
					xQueueSend( xQueueHttpCli, ( void * )&stSdMsg, 0);

				}

				fclose(f);
				break;
			}
		}
	}
	else
	{
		#if DEBUG_SDCARD
			ESP_LOGE(SD_TASK_TAG, "No more files...");
		#endif

		stSdMsg.ucSrc = SRC_SD;
		stSdMsg.ucDest = SRC_GSM;
		stSdMsg.ucEvent = EVENT_GSM_ENDING/*EVENT_GSM_LIST_SMS_MSG*/;
		xQueueSend( xQueueGsm, ( void * )&stSdMsg, 0);

		stSdMsg.ucSrc = SRC_SD;
		stSdMsg.ucDest = SRC_WIFI;
		stSdMsg.ucEvent = EVENT_WIFI_ENDING;
		xQueueSend( xQueueWifi, ( void * )&stSdMsg, 0);


		stSdMsg.ucSrc = SRC_SD;
		stSdMsg.ucDest = SRC_SD;
		stSdMsg.ucEvent = EVENT_SD_OPENING;
		xQueueSend( xQueueSd, ( void * )&stSdMsg, 0);



	}
	free(cLocalBuffer);

	return(boError);
}

//////////////////////////////////////////////
//
//
//              TaskSd_Marking
//
//
//////////////////////////////////////////////
unsigned char TaskSd_Marking(sMessageType *psMessage)
{
	unsigned char boError = true;
	FILE *f;

#if DEBUG_SDCARD
	ESP_LOGI(SD_TASK_TAG, "<<<<MARKING>>>>\r\n");
#endif

	if((f = fopen(szFilenameToBeRead, "r+")) != NULL)
	{
		fseek(f,liFilePointerPositionBeforeReading,SEEK_SET);
		fputs("*",f);
		liFilePointerPositionBeforeReading = liFilePointerPositionAfterReading;
		fclose(f);

	}

	stSdMsg.ucSrc = SRC_SD;
	stSdMsg.ucDest = SRC_SD;
	stSdMsg.ucEvent = EVENT_SD_READING;
	xQueueSend( xQueueSd, ( void * )&stSdMsg, 0);

	return(boError);
}

//////////////////////////////////////////////
//
//
//              TaskSd_Erasing
//
//
//////////////////////////////////////////////
unsigned char TaskSd_Erasing(sMessageType *psMessage)
{
	unsigned char boError = true;
	FILE *f;

#if DEBUG_SDCARD
    ESP_LOGI(SD_TASK_TAG, "<<<<ERASING>>>>");
#endif

	char *pch;
	char cDataId[16],cDataCode[16],cFilename[48];
	tstSensorKeylessCode *pst = NULL;

	pch = strtok(psMessage->pcMessageData,",");
	strcpy(cFilename,pch);

	pch = strtok(NULL,",");
	strcpy(cDataId,pch);

	pch = strtok(NULL,"\r\n");
	strcpy(cDataCode,pch);
#if DEBUG_SDCARD
		ESP_LOGI(SD_TASK_TAG, "Filename:%s",cFilename);
		ESP_LOGI(SD_TASK_TAG, "Id:%s",cDataId);
		ESP_LOGI(SD_TASK_TAG, "Code:%s",cDataCode);
#endif

	f = fopen(cFilename, "w" );
	fseek ( f , 0 , SEEK_SET );
	if(f != NULL)
	{
		ESP_LOGI(SD_TASK_TAG, "Arquivo aberto\r\n");

		for(int i = 0; i < MAX_KEYLESS_SENSOR; i++)
		{
			if( strstr(cFilename,"KEYLESS.TXT") != NULL)
			{
				pst = &stKeyless[i];
			}
			else
			{
				if( strstr(cFilename,"SENSOR.TXT") != NULL)
				{
					pst = &stSensor[i];
				}
				else
				{
					if( strstr(cFilename,"TELEPHONE.TXT") != NULL)
					{
						pst = &stTelephone[i];
					}
				}
			}

			#if DEBUG_SDCARD
				ESP_LOGI(SD_TASK_TAG, "Code[%d]:%s",i,pst->cCode);
			#endif

			if(strncmp(pst->cCode,cDataCode,13) == 0)
			{
#if 0
				memset(cDataBuffer,0,sizeof(cDataBuffer));
				strcpy(cDataBuffer,"CODE EXISTS AND WILL BE ERASED");

				stSdMsg.ucSrc = SRC_SD;
				stSdMsg.ucDest = SRC_BLE;
				stSdMsg.ucEvent = (int)NULL;
				stSdMsg.pcMessageData = &cDataBuffer[0];
				xQueueSend(xQueueBle,( void * )&stSdMsg,NULL);


				stSdMsg.ucSrc = SRC_SD;
				stSdMsg.ucDest = SRC_APP;
				stSdMsg.ucEvent = EVENT_APP_REMOTE_RECORDED;
				stSdMsg.u32MessageData = 0;/* Not Recorded*/
				xQueueSend(xQueueApp,( void * )&stSdMsg,NULL);
#endif

				#if DEBUG_SDCARD
				ESP_LOGI(SD_TASK_TAG, "Code Found!");
				#endif

				memset(cDataBuffer,0,sizeof(cDataBuffer));
				strcpy(cDataBuffer,"CODE EXISTS AND WILL BE ERASED");
#if SRC_BLE
				stSdMsg.ucSrc = SRC_SD;
				stSdMsg.ucDest = SRC_BLE;
				stSdMsg.ucEvent = (int)NULL;
				stSdMsg.pcMessageData = &cDataBuffer[0];
				xQueueSend(xQueueBle,( void * )&stSdMsg,NULL);
#endif
				strcpy(pst->cId,"TEL ERASED");
				strcpy(pst->cCode,"5511000000000");
				int i = fprintf(f,"%s,%s\r\n",pst->cId,pst->cCode);
				if(i > 0)
				{
					#if DEBUG_SDCARD
					ESP_LOGI(SD_TASK_TAG, "Erasing Undefined code!");
					#endif
				}
			}
			else
			{
				if(strlen(pst->cCode) > 0)
				{
					int i = fprintf(f,"%s,%s\r\n",pst->cId,pst->cCode);
					if(i > 0)
					{
						#if DEBUG_SDCARD
						ESP_LOGI(SD_TASK_TAG, "Erasing OK!");
						#endif
					}
				}
			}
		}
		fclose(f);
	}


    /*********************************************
     *			READING FILE
     *********************************************/
    // Open renamed file for reading
	char* cLocalBuffer = (char*) malloc(RX_BUF_SIZE+1);
    f = fopen(cFilename, "r");
    if (f == NULL)
    {
		ESP_LOGE(SD_TASK_TAG, "Failed to open file for reading");
		boError = false;
		free(cLocalBuffer);
		return(boError);
    }

    memset(cLocalBuffer,0,RX_BUF_SIZE+1);
	fread (cLocalBuffer,1,RX_BUF_SIZE,f);
	fclose(f);


	int i = 0;
	pch = strtok(cLocalBuffer,",");
	pch = strtok(NULL,"\r\n");
	while ((pch != NULL) && (i < MAX_KEYLESS_SENSOR))
	{
		if( strstr(cFilename,"KEYLESS.TXT") != NULL)
		{
			pst = &stKeyless[i];
		}
		else
		{
			if( strstr(cFilename,"SENSOR.TXT") != NULL)
			{
				pst = &stSensor[i];
			}
			else
			{
				if( strstr(cFilename,"TELEPHONE.TXT") != NULL)
				{
					pst = &stTelephone[i];
				}

			}
		}
		strcpy(pst->cCode,pch);
		ESP_LOGI(SD_TASK_TAG, "Reading Code[%d]:%s",i, pst->cCode);
		pch = strtok(cLocalBuffer,"\r\n");
		pch = strtok(NULL,",");
		i++;
	}

	free(cLocalBuffer);
	return(boError);
}

//////////////////////////////////////////////
//
//
//              TaskSd_WritingConfig
//
//
//////////////////////////////////////////////
unsigned char TaskSd_WritingConfig(sMessageType *psMessage)
{
	unsigned char boError = true;
	FILE *pFile;
	char cLocalBuffer[RX_BUF_SIZE];

#if DEBUG_SDCARD
    ESP_LOGI(SD_TASK_TAG, "<<<<TaskSd_WritingConfig>>>>\r\n");
#endif

	char *pch;
	char cToken[16],cData[64],cFilename[48];

	pch = strtok(psMessage->pcMessageData,",");
	strcpy(cFilename,pch);

	pch = strtok(NULL,"=");
	strcpy(cToken,pch);

	pch = strtok(NULL,"\r\n");
	strcpy(cData,pch);
#if DEBUG_SDCARD
		ESP_LOGI(SD_TASK_TAG, "Filename:%s",cFilename);
		ESP_LOGI(SD_TASK_TAG, "Token:%s",cToken);
		ESP_LOGI(SD_TASK_TAG, "Data:%s",cData);
#endif

	pFile = fopen(cFilename, "r");
	if(pFile != NULL)
	{
	    memset(cLocalBuffer,0,RX_BUF_SIZE);
	    fread(cLocalBuffer,sizeof(tstConfiguration),1,pFile);
	    if(strlen(cLocalBuffer)> 0){

			ESP_LOGI(SD_TASK_TAG, "cLocalBuffer:%s",(char*)cLocalBuffer);

			pch=strstr(cLocalBuffer,(const char*)cToken);
			if(pch != NULL)
			{
				pch +=strlen(cToken);
				pch ++;
				pch =strtok(pch,"\r");/* Token*/
				strcpy(pch,cData);
			}

			fprintf(pFile,"%s",cLocalBuffer);
			fclose(pFile);
	    }
	}

    /*********************************************
     *			READING FILE
     *********************************************/
    // Open renamed file for reading
    pFile = fopen(cFilename, "r");
    if (pFile == NULL)
    {
		ESP_LOGE(SD_TASK_TAG, "Failed to open file for reading");
		boError = false;
		return(boError);
    }else{
        memset(cLocalBuffer,0,RX_BUF_SIZE);
    	fread (cLocalBuffer,1,RX_BUF_SIZE,pFile);
    	fclose(pFile);
    	if(strlen(cLocalBuffer) >0)
    	{
    		ESP_LOGI(SD_TASK_TAG, "Reading Buffer:%s",cLocalBuffer);
    	}
    }


	return(boError);
}

//////////////////////////////////////////////
//
//
//              TaskSd_IgnoreEvent
//
//
//////////////////////////////////////////////
unsigned char TaskSd_IgnoreEvent(sMessageType *psMessage)
{
	unsigned char boError = false;
	return(boError);
}


static sStateMachineType const gasTaskSd_Initializing[] =
{
        /* Event                            Action routine          Success state               Failure state*/
        //  State specific transitions
        {EVENT_SD_INIT,                     TaskSd_Init,            	TASKSD_INITIALIZING,        TASKSD_INITIALIZING },
		{EVENT_SD_OPENING, 					TaskSd_Opening,			 	TASKSD_INITIALIZING,		TASKSD_INITIALIZING	},
		{EVENT_SD_WRITING, 					TaskSd_Writing,	    	 	TASKSD_INITIALIZING,		TASKSD_INITIALIZING	},
		{EVENT_SD_PROGRAMMING, 				TaskSd_Programming,    	 	TASKSD_INITIALIZING,		TASKSD_INITIALIZING	},
		{EVENT_SD_READING, 					TaskSd_Reading,				TASKSD_INITIALIZING,		TASKSD_INITIALIZING	},
	   	{EVENT_SD_MARKING, 					TaskSd_Marking,				TASKSD_INITIALIZING,		TASKSD_INITIALIZING },
	   	{EVENT_SD_ERASING, 					TaskSd_Erasing,				TASKSD_INITIALIZING,		TASKSD_INITIALIZING },
	   	{EVENT_SD_WRITING_CONFIG,			TaskSd_WritingConfig,		TASKSD_INITIALIZING,		TASKSD_INITIALIZING },
		{EVENT_SD_READWRITE_CONFIG,			TaskSd_ReadWriteConfig,		TASKSD_INITIALIZING,		TASKSD_INITIALIZING },
	    {EVENT_SD_NULL,                     TaskSd_IgnoreEvent,			TASKSD_INITIALIZING,		TASKSD_INITIALIZING }
};

static sStateMachineType const * const gpasTaskSd_StateMachine[] =
{
    gasTaskSd_Initializing
};

static unsigned char ucCurrentStateSd = TASKSD_INITIALIZING;

void vTaskSd( void *pvParameters )
{

	for( ;; )
	{
	    /*ESP_LOGI(SD_TASK_TAG, "Running...");*/

		if( xQueueReceive( xQueueSd, &( stSdMsg ),0 ) )
		{
            (void)eEventHandler ((unsigned char)SRC_SD,gpasTaskSd_StateMachine[ucCurrentStateSd], &ucCurrentStateSd, &stSdMsg);
		}

		vTaskDelay(500/portTICK_PERIOD_MS);
	}
}
