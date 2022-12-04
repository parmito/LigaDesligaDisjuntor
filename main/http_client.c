/* ESP HTTP Client Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "app_wifi.h"


#include "Sd.h"
#include "Debug.h"
#include "defines.h"
#include "State.h"
#include "http_client.h"

#include "esp_http_client.h"

#define MAX_HTTP_RECV_BUFFER DEFAULT_HTTP_BUF_SIZE
extern char cConfigAndData[DEFAULT_HTTP_BUF_SIZE];
extern unsigned long ulTimestamp;
static char cLocalBuffer[DEFAULT_HTTP_BUF_SIZE];
static char cPostBuffer[DEFAULT_HTTP_BUF_SIZE];
static char cInterTaskBuffer[DEFAULT_HTTP_BUF_SIZE];
/*extern unsigned char ucCurrentStateGsm;*/
sMessageType stHttpCliMsg;

#define NUM_TIMERS 1
static TimerHandle_t pxTimer[ NUM_TIMERS ];

typedef struct{
	const char TimerName[16];
}tstTimerName;

static tstTimerName stTimerName[]=
{
		{"TIMER1"}
};

static unsigned char ucCurrentStateHttpCli = TASKHTTPCLI_IDLING;
static const char *TAG = "HTTP_CLIENT";

/* Root cert for howsmyssl.com, taken from howsmyssl_com_root_cert.pem

   The PEM file was extracted from the output of this command:
   openssl s_client -showcerts -connect www.howsmyssl.com:443 </dev/null

   The CA root cert is the last cert given in the chain of certs.

   To embed it in the app binary, the PEM file is named
   in the component.mk COMPONENT_EMBED_TXTFILES variable.
*/
extern const char api_telegram_org_pem_start[] asm("_binary_api_telegram_org_pem_start");
extern const char api_telegra_org_pem_end[]   asm("_binary_api_telegram_org_pem_end");

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // Write out data
                /*printf("%.*s", evt->data_len, (char*)evt->data);*/
                memcpy(cLocalBuffer,evt->data,evt->data_len);
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}

static void http_rest()
{
    esp_http_client_config_t config = {
        .url = "http://httpbin.org/get",
        .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // GET
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }

    // POST
    const char *post_data = "field1=value1&field2=value2";
    esp_http_client_set_url(client, "http://httpbin.org/post");
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    //PUT
    esp_http_client_set_url(client, "http://httpbin.org/put");
    esp_http_client_set_method(client, HTTP_METHOD_PUT);
    err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP PUT Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP PUT request failed: %s", esp_err_to_name(err));
    }

    //PATCH
    esp_http_client_set_url(client, "http://httpbin.org/patch");
    esp_http_client_set_method(client, HTTP_METHOD_PATCH);
    esp_http_client_set_post_field(client, NULL, 0);
    err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP PATCH Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP PATCH request failed: %s", esp_err_to_name(err));
    }

    //DELETE
    esp_http_client_set_url(client, "http://httpbin.org/delete");
    esp_http_client_set_method(client, HTTP_METHOD_DELETE);
    err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP DELETE Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP DELETE request failed: %s", esp_err_to_name(err));
    }

    //HEAD
    esp_http_client_set_url(client, "http://httpbin.org/get");
    esp_http_client_set_method(client, HTTP_METHOD_HEAD);
    err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP HEAD Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP HEAD request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

static void http_auth_basic()
{
    esp_http_client_config_t config = {
        .url = "http://user:passwd@httpbin.org/basic-auth/user/passwd",
        .event_handler = _http_event_handler,
        .auth_type = HTTP_AUTH_TYPE_BASIC,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP Basic Auth Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
}

static void http_auth_basic_redirect()
{
    esp_http_client_config_t config = {
        .url = "http://user:passwd@httpbin.org/basic-auth/user/passwd",
        .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP Basic Auth redirect Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
}

static void http_auth_digest()
{
    esp_http_client_config_t config = {
        .url = "http://user:passwd@httpbin.org/digest-auth/auth/user/passwd/MD5/never",
        .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP Digest Auth Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
}

static void https()
{
    esp_http_client_config_t config = {
        .url = "https://api.telegram.org/bot1868791170:AAFFp1d2bGFFkphKRuhMzGnOTD1oHExlySs/sendMessage?chat_id=-408374433&text=nicole",
        .event_handler = _http_event_handler,
		.cert_pem = api_telegram_org_pem_start,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTPS Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
}

static void http_relative_redirect()
{
    esp_http_client_config_t config = {
        .url = "http://httpbin.org/relative-redirect/3",
        .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP Relative path redirect Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
}

static void http_absolute_redirect()
{
    esp_http_client_config_t config = {
        .url = "http://httpbin.org/absolute-redirect/3",
        .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP Absolute path redirect Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
}

static void http_redirect_to_https()
{
    esp_http_client_config_t config = {
        .url = "http://httpbin.org/redirect-to?url=https%3A%2F%2Fwww.howsmyssl.com",
        .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP redirect to HTTPS Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
}


static void http_download_chunk()
{
    esp_http_client_config_t config = {
        .url = "http://httpbin.org/stream-bytes/8912",
        .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP chunk encoding Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
}

static void http_perform_as_stream_reader()
{
    char *buffer = malloc(MAX_HTTP_RECV_BUFFER + 1);
    if (buffer == NULL) {
        ESP_LOGE(TAG, "Cannot malloc http receive buffer");
    }
    esp_http_client_config_t config = {
        .url = "http://httpbin.org/get",
        .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err;
    if ((err = esp_http_client_open(client, 0)) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        free(buffer);
    }
    int content_length =  esp_http_client_fetch_headers(client);
    int total_read_len = 0, read_len;
    if (total_read_len < content_length && content_length <= MAX_HTTP_RECV_BUFFER) {
        read_len = esp_http_client_read(client, buffer, content_length);
        if (read_len <= 0) {
            ESP_LOGE(TAG, "Error read data");
        }
        buffer[read_len] = 0;
        ESP_LOGD(TAG, "read_len = %d", read_len);
    }
    ESP_LOGI(TAG, "HTTP Stream reader Status = %d, content_length = %d",
                    esp_http_client_get_status_code(client),
                    esp_http_client_get_content_length(client));
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    free(buffer);

}


static unsigned char http_get_timestamp(void)
{
	unsigned char boError = true;

    ESP_LOGI(TAG, "http_get_timestamp\r\n");
#if 0
    memset(cLocalBuffer,0,DEFAULT_HTTP_BUF_SIZE);

    esp_http_client_config_t config = {
          .url = "http://gpslogger.esy.es/pages/upload/timestamp.php",
          .event_handler = _http_event_handler,
      };
      esp_http_client_handle_t client = esp_http_client_init(&config);

      // GET
      esp_err_t err = esp_http_client_perform(client);
      if (err == ESP_OK) {

          ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d, content_data=%s",
                  esp_http_client_get_status_code(client),
                  esp_http_client_get_content_length(client),
				  cLocalBuffer);

          char *ptr = NULL;
          ptr = strstr((const char*)&cLocalBuffer,(const char*)"Timestamp=");

          if(ptr != NULL)
          {
          	ESP_LOGI(TAG,"ptr=%s\r\n",ptr);
  			ptr +=strlen((const char*)("Timestamp="));
  			ulTimestamp = atol(ptr);
  			ESP_LOGI(TAG,"Timestamp OK=%ld\r\n",ulTimestamp);

			stHttpCliMsg.ucSrc = SRC_HTTPCLI;
			stHttpCliMsg.ucDest = SRC_SD;
			stHttpCliMsg.ucEvent = EVENT_SD_OPENING;
			xQueueSend( xQueueSd, ( void * )&stHttpCliMsg, 0);
          }
      }
#endif
    /* Avoiding use of gpslogger.esy.es/pages/upload/timestamp.php
     * this way we will always move forward!
     */
	stHttpCliMsg.ucSrc = SRC_HTTPCLI;
	stHttpCliMsg.ucDest = SRC_SD;
	stHttpCliMsg.ucEvent = EVENT_SD_OPENING;
	xQueueSend( xQueueSd, ( void * )&stHttpCliMsg, 0);

	boError = true;
#if 0
    esp_http_client_config_t config = {
        .url = "http://gpslogger.esy.es/pages/upload/timestamp.php",
        .event_handler = _http_event_handler,
        .is_async = false,
		.timeout_ms = 5000
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err;

    esp_http_client_set_method(client, HTTP_METHOD_GET);

    ESP_LOGI(TAG, "http_get_timestamp\r\n");
    while (1) {
        err = esp_http_client_perform(client);
        if (err != ESP_ERR_HTTP_EAGAIN) {
            break;
        }
    }

    if (err == ESP_OK)
    {
        int read_len = esp_http_client_read(client, cLocalBuffer, 128);
		if (read_len <= 0) {
			ESP_LOGI(TAG, "Error read data");
		}
		cLocalBuffer[read_len] = 0;
		ESP_LOGI(TAG, "read_len = %d", read_len);

        ESP_LOGI(TAG, "HTTPS Status = %d, content_length = %d, content = %s\r\n",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client),
				cLocalBuffer);

        char *ptr = NULL;
        ptr = strstr((const char*)&cLocalBuffer,(const char*)"Timestamp=");

        if(ptr != NULL)
        {
        	ESP_LOGI(TAG,"ptr=%s\r\n",ptr);
			ptr +=strlen((const char*)("Timestamp="));
			ulTimestamp = atol(ptr);
			ESP_LOGI(TAG,"Timestamp OK=%ld\r\n",ulTimestamp);


            stHttpCliMsg.ucSrc = SRC_HTTPCLI;
            stHttpCliMsg.ucDest = SRC_SD;
            stHttpCliMsg.ucEvent = EVENT_SD_OPENING;
            xQueueSend( xQueueSd, ( void * )&stHttpCliMsg, 0);
        }
        else
        {
        	stHttpCliMsg.ucSrc = SRC_HTTPCLI;
        	stHttpCliMsg.ucDest = SRC_HTTPCLI;
        	stHttpCliMsg.ucEvent = EVENT_HTTPCLI_GET_TIMESTAMP;
            xQueueSend( xQueueHttpCli, ( void * )&stHttpCliMsg, 0);

            boError = false;
        }
    }
    else
    {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));

    	stHttpCliMsg.ucSrc = SRC_HTTPCLI;
    	stHttpCliMsg.ucDest = SRC_HTTPCLI;
    	stHttpCliMsg.ucEvent = EVENT_HTTPCLI_GET_TIMESTAMP;
        xQueueSend( xQueueHttpCli, ( void * )&stHttpCliMsg, 0);

        boError = false;
    }
#endif
    /*esp_http_client_cleanup(client);*/

    return(boError);
}
/* Read message from sensors when system is armed*/
static unsigned char http_get_telegram(const char *post_data)
{
	unsigned char boError = true;

    ESP_LOGI(TAG, "http_get_telegram\r\n");

    memset(cLocalBuffer,0,DEFAULT_HTTP_BUF_SIZE);

    esp_http_client_config_t config = {
        .url = post_data,
        .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);


	// GET
	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK) {

		ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d, content_data=%s",
				esp_http_client_get_status_code(client),
				esp_http_client_get_content_length(client),
			  cLocalBuffer);

        char *ptr = NULL;
        ptr = strstr((const char*)&cLocalBuffer[0],(const char*)"\"id\": -408374433");

        if(ptr != NULL)
        {
        	ESP_LOGI(TAG,"ptr=%s\r\n",ptr);

        	stHttpCliMsg.ucSrc = SRC_HTTPCLI;
        	stHttpCliMsg.ucDest = SRC_HTTPCLI;
        	stHttpCliMsg.ucEvent = EVENT_HTTPCLI_GET_TELEGRAM;
        	xQueueSend( xQueueHttpCli, ( void * )&stHttpCliMsg, 0);

        	boError = false;

        }
        else
        {
            stHttpCliMsg.ucSrc = SRC_HTTPCLI;
            stHttpCliMsg.ucDest = SRC_SD;
            stHttpCliMsg.ucEvent = EVENT_SD_MARKING;
        	xQueueSend( xQueueSd, ( void * )&stHttpCliMsg, 0);
        }
    }
    else
    {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));

    	stHttpCliMsg.ucSrc = SRC_HTTPCLI;
    	stHttpCliMsg.ucDest = SRC_HTTPCLI;
    	stHttpCliMsg.ucEvent = EVENT_HTTPCLI_GET_TELEGRAM;
    	xQueueSend( xQueueHttpCli, ( void * )&stHttpCliMsg, 0);

    	boError = false;
    }
    esp_http_client_cleanup(client);

    return(boError);
}

static unsigned char http_get_polling(const char *post_data)
{
	unsigned char boError = true;

    ESP_LOGI(TAG, "http_get_polling\r\n");

    memset(cLocalBuffer,0,DEFAULT_HTTP_BUF_SIZE);

    esp_http_client_config_t config = {
        .url = post_data,
        .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);


	// GET
	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK) {

		ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d, content_data=%s",
		esp_http_client_get_status_code(client),
		esp_http_client_get_content_length(client),
		cLocalBuffer);

        char *ptr = NULL;
        ptr = strstr((const char*)&cLocalBuffer[0],(const char*)"\"id\":1337388095");/* Danilo Franco*/
        if(ptr == NULL)
        {
            ptr = strstr((const char*)&cLocalBuffer[0],(const char*)"\"id\":1163829049");/* Gabriele Biskmap*/
        }
        if(ptr != NULL)
        {
			ESP_LOGI(TAG,"ID OK\r\n");
			ptr = strstr((const char*)&cLocalBuffer[0],(const char*)"\"date\":");
			if(ptr != NULL)
			{
	          	ESP_LOGI(TAG,"ptr=%s\r\n",ptr);
	  			ptr +=strlen((const char*)("\"date\":"));
	  			ulTimestamp = atol(ptr);
	  			ESP_LOGI(TAG,"Timestamp OK=%ld\r\n",ulTimestamp);

			}

			ptr = strstr((const char*)&cLocalBuffer[0],(const char*)"\"text\":");
			if(ptr != NULL)
			{
				/*Example: $PROGRAMMING SENSOR10#*/
				ptr = strstr((const char*)&cLocalBuffer[0],(const char*)"\"text\":\"PROGRAMMING SENSOR");
				if(ptr != NULL){
					ESP_LOGI(TAG,"TEXT OK-> ptr=%s\r\n",ptr);

					memset(cInterTaskBuffer,0,sizeof(cInterTaskBuffer));
					memcpy(cInterTaskBuffer,ptr,strlen(ptr));

					stHttpCliMsg.ucSrc = SRC_HTTPCLI;
					stHttpCliMsg.ucDest = SRC_APP;
					stHttpCliMsg.ucEvent = EVENT_APP_BLE_PROGRAMMING_SENSOR;
					stHttpCliMsg.pcMessageData = (char*)cInterTaskBuffer;
					xQueueSend( xQueueApp, ( void * )&stHttpCliMsg, NULL);

				}
				else
				{
					/*Example: $PROGRAMMING KEYLESS10#*/
					ptr = strstr((const char*)&cLocalBuffer[0],(const char*)"\"text\":\"PROGRAMMING KEYLESS");
					if(ptr != NULL)
					{
						ESP_LOGI(TAG,"TEXT OK-> ptr=%s\r\n",ptr);

						memset(cInterTaskBuffer,0,sizeof(cInterTaskBuffer));
						memcpy(cInterTaskBuffer,ptr,strlen(ptr));

						stHttpCliMsg.ucSrc = SRC_HTTPCLI;
						stHttpCliMsg.ucDest = SRC_APP;
						stHttpCliMsg.ucEvent = EVENT_APP_BLE_PROGRAMMING_KEYLESS;
						stHttpCliMsg.pcMessageData = (char*)cInterTaskBuffer;
						xQueueSend( xQueueApp, ( void * )&stHttpCliMsg, NULL);


					}else
					{
						/*Example: LIGAR DISJUNTOR*/
						ptr = strstr((const char*)&cLocalBuffer[0],(const char*)"\"text\":\"LIGAR DISJUNTOR");
						if(ptr != NULL)
						{
							xTimerStop( pxTimer[ 0 ], 0 );

							ESP_LOGI(TAG,"LIGAR DISJUNTOR\r\n");
							ESP_LOGI(TAG,"TEXT OK-> ptr=%s\r\n",ptr);

							memset(cInterTaskBuffer,0,sizeof(cInterTaskBuffer));
							memcpy(cInterTaskBuffer,ptr,strlen(ptr));

							stHttpCliMsg.ucSrc = SRC_HTTPCLI;
							stHttpCliMsg.ucDest = SRC_APP;
							stHttpCliMsg.ucEvent = EVENT_APP_ARMING;
							stHttpCliMsg.pcMessageData = (char*)cInterTaskBuffer;
							xQueueSend( xQueueApp, ( void * )&stHttpCliMsg, NULL);

						}
						else
						{
							/*Example: LIGAR DISJUNTOR*/
							ptr = strstr((const char*)&cLocalBuffer[0],(const char*)"\"text\":\"DESLIGAR DISJUNTOR");
							if(ptr != NULL)
							{
								xTimerStop( pxTimer[ 0 ], 0 );

								ESP_LOGI(TAG,"DESLIGAR DISJUNTOR\r\n");
								ESP_LOGI(TAG,"TEXT OK-> ptr=%s\r\n",ptr);

								memset(cInterTaskBuffer,0,sizeof(cInterTaskBuffer));
								memcpy(cInterTaskBuffer,ptr,strlen(ptr));

								stHttpCliMsg.ucSrc = SRC_HTTPCLI;
								stHttpCliMsg.ucDest = SRC_APP;
								stHttpCliMsg.ucEvent = EVENT_APP_DISARMING;
								stHttpCliMsg.pcMessageData = (char*)cInterTaskBuffer;
								xQueueSend( xQueueApp, ( void * )&stHttpCliMsg, NULL);

							}
						}
					}

				}
			}
		}
	}
	esp_http_client_cleanup(client);

    return(boError);
}

static unsigned char http_post(const char *post_data)
{
	unsigned char boError = true;

#if 0
    esp_http_client_config_t config = {
        .url = "http://gpslogger.esy.es/pages/upload/rio.php",
        .event_handler = _http_event_handler,
        .is_async = false,
        .timeout_ms = 5000,
    };

    ESP_LOGI(TAG, "http_post");

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err;
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    while (1) {
        err = esp_http_client_perform(client);
        if (err != ESP_ERR_HTTP_EAGAIN) {
            break;
        }
    }

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTPS Status = %d, content_length = %d, content_data=%s",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client),
				cLocalBuffer);

        	ESP_LOGI(TAG, "Data Has been Posted!\r\n");
        	stHttpCliMsg.ucSrc = SRC_HTTPCLI;
        	stHttpCliMsg.ucDest = SRC_HTTPCLI;
        	stHttpCliMsg.ucEvent = EVENT_HTTPCLI_GET_TELEGRAM;
        	xQueueSend( xQueueHttpCli, ( void * )&stHttpCliMsg, 0);

    }
#endif
	stHttpCliMsg.ucSrc = SRC_HTTPCLI;
	stHttpCliMsg.ucDest = SRC_HTTPCLI;
	stHttpCliMsg.ucEvent = EVENT_HTTPCLI_GET_TELEGRAM;
	xQueueSend( xQueueHttpCli, ( void * )&stHttpCliMsg, 0);

	boError = true;

    /*esp_http_client_cleanup(client);*/

    return(boError);
}
//////////////////////////////////////////////
//
//
//              TaskHttpCli_Init
//
//
//////////////////////////////////////////////
unsigned char TaskHttpCli_Init(sMessageType *psMessage)
{
    unsigned char boError = true;

    ESP_LOGI(TAG, "<<<<HTTPCLI INIT>>>>\r\n");

	stHttpCliMsg.ucSrc = SRC_HTTPCLI;
	stHttpCliMsg.ucDest = SRC_HTTPCLI;
	stHttpCliMsg.ucEvent = EVENT_HTTPCLI_CONNECTING;
	xQueueSend( xQueueHttpCli, ( void * )&stHttpCliMsg, 0);


    return(boError);
}


//////////////////////////////////////////////
//
//
//              TaskHttpCli_Connecting
//
//
//////////////////////////////////////////////
unsigned char TaskHttpCli_Connecting(sMessageType *psMessage)
{
    unsigned char boError = true;

    ESP_LOGI(TAG, "<<<<HTTPCLI CONNECTING>>>>\r\n");

    esp_wifi_connect();
    /*app_wifi_wait_connected();*/

	stHttpCliMsg.ucSrc = SRC_HTTPCLI;
	stHttpCliMsg.ucDest = SRC_DEBUG;
	stHttpCliMsg.ucEvent = EVENT_IO_GSM_CONNECTING;
	xQueueSend( xQueueDebug, ( void * )&stHttpCliMsg, 0);


    return(boError);
}

//////////////////////////////////////////////
//
//
//              TaskHttpCli_Connected
//
//
//////////////////////////////////////////////
unsigned char TaskHttpCli_Connected(sMessageType *psMessage)
{
    unsigned char boError = true;

    ESP_LOGI(TAG, "<<<<HTTPCLI CONNECTED>>>>\r\n");

    /*stHttpCliMsg.ucSrc = SRC_HTTPCLI;
    stHttpCliMsg.ucDest = SRC_SD;
    stHttpCliMsg.ucEvent = EVENT_SD_OPENING;
    xQueueSend( xQueueSd, ( void * )&stHttpCliMsg, 0);*/

    stHttpCliMsg.ucSrc = SRC_HTTPCLI;
    stHttpCliMsg.ucDest = SRC_HTTPCLI;
    stHttpCliMsg.ucEvent = EVENT_HTTPCLI_GET_TIMESTAMP;
    xQueueSend( xQueueHttpCli, ( void * )&stHttpCliMsg, 0);
    return(boError);
}

//////////////////////////////////////////////
//
//
//              TaskHttpCli_GetTimestamp
//
//
//////////////////////////////////////////////
unsigned char TaskHttpCli_GetTimestamp(sMessageType *psMessage)
{
    unsigned char boError = true;

    ESP_LOGI(TAG, "<<<<HTTPCLI GET TIMESTAMP>>>>\r\n");

    boError = http_get_timestamp();

	stHttpCliMsg.ucSrc = SRC_HTTPCLI;
	stHttpCliMsg.ucDest = SRC_DEBUG;
	stHttpCliMsg.ucEvent = EVENT_IO_GSM_COMMUNICATING;
	xQueueSend( xQueueDebug, ( void * )&stHttpCliMsg, 0);

	xTimerStart( pxTimer[ 0 ], 0 );

    return(boError);
}

//////////////////////////////////////////////
//
//
//              TaskHttpCli_Post
//
//
//////////////////////////////////////////////
unsigned char TaskHttpCli_Post(sMessageType *psMessage)
{
    unsigned char boError = true;

    memset(cPostBuffer,0,DEFAULT_HTTP_BUF_SIZE);

    ESP_LOGI(TAG, "<<<<HTTPCLI POSTING>>>>\r\n");
    ESP_LOGI(TAG, "%s\r\n",cConfigAndData);

    sprintf(cPostBuffer,"%s",cConfigAndData);

    boError = http_post(cPostBuffer);

	stHttpCliMsg.ucSrc = SRC_HTTPCLI;
	stHttpCliMsg.ucDest = SRC_DEBUG;
	stHttpCliMsg.ucEvent = EVENT_IO_GSM_COMMUNICATING;
	xQueueSend( xQueueDebug, ( void * )&stHttpCliMsg, 0);

    return(boError);
}


//////////////////////////////////////////////
//
//
//              TaskHttpCli_GetTelegram
//
//
//////////////////////////////////////////////
unsigned char TaskHttpCli_GetTelegram(sMessageType *psMessage)
{
    unsigned char boError = true;

    memset(cPostBuffer,0,DEFAULT_HTTP_BUF_SIZE);

	char cTempBuffer[256];
    memset(cTempBuffer,0,256);

    ESP_LOGI(TAG, "<<<<HTTPCLI GET TELEGRAM>>>>\r\n");
    ESP_LOGI(TAG, "%s\r\n",cConfigAndData);

    strncpy(cTempBuffer,cConfigAndData,strlen(cConfigAndData));
    char *ptr = NULL;

    ptr = strtok(cTempBuffer,",");
    if(ptr !=NULL)
    {
    	ptr = strtok (NULL, ",");
    }

    ESP_LOGI(TAG, "%s\r\n",ptr);

    long int liTimestamp;
	struct tm  ts;
	static char cFormatTime[32];
    memset(cFormatTime,0,sizeof(cFormatTime));

    if(strcmp(ptr,"SENSOR1") == 0)
    {
        ptr = strtok (NULL, ",");
        liTimestamp = atol(ptr);
        ts = *localtime((time_t*)&liTimestamp);
        ts.tm_hour = ts.tm_hour - 3;
        strftime(cFormatTime, sizeof(cFormatTime), "%d/%m/%Y-%H:%M:%S", &ts);

    	sprintf(cPostBuffer,"https://api.telegram.org/bot1868791170:AAFFp1d2bGFFkphKRuhMzGnOTD1oHExlySs/sendMessage?chat_id=-1337388095&text=%s,%s","JANELA_FRENTE",cFormatTime);
    }
    else
    {
        if(strcmp(ptr,"SENSOR2") == 0)
        {

            ptr = strtok (NULL, ",");
            liTimestamp = atol(ptr);
            ts = *localtime((time_t*)&liTimestamp);
            ts.tm_hour = ts.tm_hour - 3;
            strftime(cFormatTime, sizeof(cFormatTime), "%d/%m/%Y-%H:%M:%S", &ts);

        	sprintf(cPostBuffer,"https://api.telegram.org/bot1868791170:AAFFp1d2bGFFkphKRuhMzGnOTD1oHExlySs/sendMessage?chat_id=-1337388095&text=%s,%s","PORTA_FRENTE",cFormatTime);
        }
        else
        {
            if(strcmp(ptr,"SENSOR3") == 0)
            {
                ptr = strtok (NULL, ",");
                liTimestamp = atol(ptr);
                ts = *localtime((time_t*)&liTimestamp);
                ts.tm_hour = ts.tm_hour - 3;
                strftime(cFormatTime, sizeof(cFormatTime), "%d/%m/%Y-%H:%M:%S", &ts);

            	sprintf(cPostBuffer,"https://api.telegram.org/bot1868791170:AAFFp1d2bGFFkphKRuhMzGnOTD1oHExlySs/sendMessage?chat_id=-1337388095&text=%s,%s","PORTA_COZINHA",cFormatTime);
            }
            else
            {
                if(strcmp(ptr,"SENSOR4") == 0)
                {
                    ptr = strtok (NULL, ",");
                    liTimestamp = atol(ptr);
                    ts = *localtime((time_t*)&liTimestamp);
                    ts.tm_hour = ts.tm_hour - 3;
                    strftime(cFormatTime, sizeof(cFormatTime), "%d/%m/%Y-%H:%M:%S", &ts);
                	sprintf(cPostBuffer,"https://api.telegram.org/bot1868791170:AAFFp1d2bGFFkphKRuhMzGnOTD1oHExlySs/sendMessage?chat_id=-1337388095&text=%s,%s","PORTAO_BASCULANTE",cFormatTime);
                }
                else
                {
                    if(strcmp(ptr,"SENSOR5") == 0)
                    {
                        ptr = strtok (NULL, ",");
                        liTimestamp = atol(ptr);
                        ts = *localtime((time_t*)&liTimestamp);
                        ts.tm_hour = ts.tm_hour - 3;
                        strftime(cFormatTime, sizeof(cFormatTime), "%d/%m/%Y-%H:%M:%S", &ts);
                    	sprintf(cPostBuffer,"https://api.telegram.org/bot1868791170:AAFFp1d2bGFFkphKRuhMzGnOTD1oHExlySs/sendMessage?chat_id=-1337388095&text=%s,%s","PORTAOZINHO",cFormatTime);
                    }
                    else
                    {
                        if(strcmp(ptr,"DISJUNTOR_LIGADO") == 0)
                        {
                            ptr = strtok (NULL, ",");
                            liTimestamp = atol(ptr);
                            ts = *localtime((time_t*)&liTimestamp);
                            ts.tm_hour = ts.tm_hour - 3;
                            strftime(cFormatTime, sizeof(cFormatTime), "%d/%m/%Y-%H:%M:%S", &ts);
                        	sprintf(cPostBuffer,"https://api.telegram.org/bot1868791170:AAFFp1d2bGFFkphKRuhMzGnOTD1oHExlySs/sendMessage?chat_id=-452102234&text=DISJUNTOR_LIGADO,%s",cFormatTime);
                        }
                        else
                        {
                            if(strcmp(ptr,"DISJUNTOR_DESLIGADO") == 0)
                             {
                                 ptr = strtok (NULL, ",");
                                 liTimestamp = atol(ptr);
                                 ts = *localtime((time_t*)&liTimestamp);
                                 ts.tm_hour = ts.tm_hour - 3;
                                 strftime(cFormatTime, sizeof(cFormatTime), "%d/%m/%Y-%H:%M:%S", &ts);
                             	sprintf(cPostBuffer,"https://api.telegram.org/bot1868791170:AAFFp1d2bGFFkphKRuhMzGnOTD1oHExlySs/sendMessage?chat_id=-452102234&text=DISJUNTOR_DESLIGADO,%s",cFormatTime);
                             }
                             else
                             {
                            		ptr = strtok (NULL, ",");
									liTimestamp = atol(ptr);
									ts = *localtime((time_t*)&liTimestamp);
									ts.tm_hour = ts.tm_hour - 3;
									strftime(cFormatTime, sizeof(cFormatTime), "%d/%m/%Y-%H:%M:%S", &ts);
									sprintf(cPostBuffer,"https://api.telegram.org/bot1868791170:AAFFp1d2bGFFkphKRuhMzGnOTD1oHExlySs/sendMessage?chat_id=-1337388095&text=%s,%s","NOVO_SENSOR",cFormatTime);
                             }
                        }
                    }
                }
            }
        }
    }
    boError = http_get_telegram(cPostBuffer);

	stHttpCliMsg.ucSrc = SRC_HTTPCLI;
	stHttpCliMsg.ucDest = SRC_DEBUG;
	stHttpCliMsg.ucEvent = EVENT_IO_GSM_COMMUNICATING;
	xQueueSend( xQueueDebug, ( void * )&stHttpCliMsg, 0);

	return(boError);
}




//////////////////////////////////////////////
//
//
//              TaskHttpCli_Polling
//
//
//////////////////////////////////////////////
unsigned char TaskHttpCli_Polling(sMessageType *psMessage)
{
    unsigned char boError = true;

    memset(cPostBuffer,0,DEFAULT_HTTP_BUF_SIZE);

    ESP_LOGI(TAG, "<<<<HTTPCLI POLLING>>>>\r\n");
    ESP_LOGI(TAG, "%s\r\n",cConfigAndData);

   	sprintf(cPostBuffer,"https://api.telegram.org/bot1868791170:AAFFp1d2bGFFkphKRuhMzGnOTD1oHExlySs/getUpdates?offset=-1");

    boError = http_get_polling(cPostBuffer);

	return(boError);
}


//////////////////////////////////////////////
//
//
//              TaskHttpCli_Posted
//
//
//////////////////////////////////////////////
unsigned char TaskHttpCli_Posted(sMessageType *psMessage)
{
    unsigned char boError = true;

    ESP_LOGE(TAG, "<<<<HTTP POSTED>>>>\r\n");

	stHttpCliMsg.ucSrc = SRC_HTTPCLI;
	stHttpCliMsg.ucDest = SRC_SD;
	stHttpCliMsg.ucEvent = EVENT_SD_MARKING;
	xQueueSend( xQueueSd, ( void * )&stHttpCliMsg, 0);

	stHttpCliMsg.ucSrc = SRC_HTTPCLI;
	stHttpCliMsg.ucDest = SRC_DEBUG;
	stHttpCliMsg.ucEvent = EVENT_IO_GSM_UPLOAD_DONE;
	xQueueSend( xQueueDebug, ( void * )&stHttpCliMsg, 0);

    return(boError);
}

//////////////////////////////////////////////
//
//
//              TaskHttpCli_Disconnected
//
//
//////////////////////////////////////////////
unsigned char TaskHttpCli_Disconnected(sMessageType *psMessage)
{
    unsigned char boError = true;

    ESP_LOGI(TAG, "<<<<HTTP DISCONNECTED>>>>\r\n");

	stHttpCliMsg.ucSrc = SRC_HTTPCLI;
	stHttpCliMsg.ucDest = SRC_HTTPCLI;
	stHttpCliMsg.ucEvent = EVENT_HTTPCLI_INIT;
	xQueueSend( xQueueHttpCli, ( void * )&stHttpCliMsg, 0);

    return(boError);
}

//////////////////////////////////////////////
//
//
//              TaskHttpCli_Ending
//
//
//////////////////////////////////////////////
unsigned char TaskHttpCli_Ending(sMessageType *psMessage)
{
	unsigned char boError = true;

    ESP_LOGI(TAG, "<<<<HTTPCLI:>ENDING>>>>\r\n");

	vTaskDelay(2000/portTICK_PERIOD_MS);

	stHttpCliMsg.ucSrc = SRC_HTTPCLI;
	stHttpCliMsg.ucDest = SRC_SD;
	stHttpCliMsg.ucEvent = EVENT_SD_OPENING;
	xQueueSend( xQueueSd, ( void * )&stHttpCliMsg, 0);

	xTimerStart( pxTimer[ 0 ], 0 );

	return(boError);
}

//////////////////////////////////////////////
//
//
//              TaskHttpCli_IgnoreEvent
//
//
//////////////////////////////////////////////
unsigned char TaskHttpCli_IgnoreEvent(sMessageType *psMessage)
{
    unsigned char boError = false;
    return(boError);
}

//////////////////////////////////////////////
//
//
//             Http State Machine
//
//
//////////////////////////////////////////////
static sStateMachineType const gasTaskHttpCli_Idling[] =
{
    /* Event        Action routine      Next state */
    //  State specific transitions
	{EVENT_HTTPCLI_INIT,      		TaskHttpCli_Init, 	            TASKHTTPCLI_INITIALIZING,        			TASKHTTPCLI_IDLING						},
	{EVENT_HTTPCLI_DISCONNECTED,  	TaskHttpCli_Disconnected, 	    TASKHTTPCLI_IDLING,           				TASKHTTPCLI_IDLING        				},
    {EVENT_HTTPCLI_NULL,      		TaskHttpCli_IgnoreEvent,        TASKHTTPCLI_IDLING,							TASKHTTPCLI_IDLING						}
};

static sStateMachineType const gasTaskHttpCli_Initializing[] =
{
    /* Event        Action routine      Next state */
    //  State specific transitions
	{EVENT_HTTPCLI_CONNECTING,     	TaskHttpCli_Connecting, 	    TASKHTTPCLI_INITIALIZING,           		TASKHTTPCLI_INITIALIZING        		},
	{EVENT_HTTPCLI_CONNECTED,  		TaskHttpCli_Connected, 	        TASKHTTPCLI_SYNCING,           				TASKHTTPCLI_INITIALIZING        		},
	{EVENT_HTTPCLI_DISCONNECTED,  	TaskHttpCli_Disconnected, 	    TASKHTTPCLI_IDLING,           				TASKHTTPCLI_IDLING        				},
    {EVENT_HTTPCLI_NULL,      		TaskHttpCli_IgnoreEvent,        TASKHTTPCLI_INITIALIZING,					TASKHTTPCLI_INITIALIZING				}
};

static sStateMachineType const gasTaskHttpCli_Syncing[] =
{
    /* Event        Action routine      Next state */
    //  State specific transitions
	{EVENT_HTTPCLI_GET_TIMESTAMP,	TaskHttpCli_GetTimestamp, 	    TASKHTTPCLI_COMMUNICATING,           		TASKHTTPCLI_SYNCING        				},
	{EVENT_HTTPCLI_DISCONNECTED,  	TaskHttpCli_Disconnected, 	    TASKHTTPCLI_IDLING,           				TASKHTTPCLI_IDLING        				},
    {EVENT_HTTPCLI_NULL,      		TaskHttpCli_IgnoreEvent,        TASKHTTPCLI_SYNCING,						TASKHTTPCLI_SYNCING						}
};

static sStateMachineType const gasTaskHttpCli_Communicating[] =
{
    /* Event        Action routine      Next state */
    //  State specific transitions

	{EVENT_HTTPCLI_POST,      		TaskHttpCli_Post, 	            TASKHTTPCLI_COMMUNICATING,           		TASKHTTPCLI_COMMUNICATING        				},
	{EVENT_HTTPCLI_GET_TELEGRAM,    TaskHttpCli_GetTelegram,        TASKHTTPCLI_COMMUNICATING,           		TASKHTTPCLI_COMMUNICATING        				},
	{EVENT_HTTPCLI_POSTED,     		TaskHttpCli_Posted, 	        TASKHTTPCLI_COMMUNICATING,           		TASKHTTPCLI_COMMUNICATING        				},
	{EVENT_HTTPCLI_DISCONNECTED,  	TaskHttpCli_Disconnected, 	    TASKHTTPCLI_IDLING,           				TASKHTTPCLI_IDLING        				        },
	{EVENT_HTTPCLI_ENDING,  		TaskHttpCli_Ending, 	    	TASKHTTPCLI_COMMUNICATING,           		TASKHTTPCLI_COMMUNICATING        				},

	{EVENT_HTTPCLI_POLLING,  		TaskHttpCli_Polling, 	    	TASKHTTPCLI_COMMUNICATING,           		TASKHTTPCLI_COMMUNICATING        				},


    {EVENT_HTTPCLI_NULL,      		TaskHttpCli_IgnoreEvent,        TASKHTTPCLI_COMMUNICATING,					TASKHTTPCLI_COMMUNICATING						}
};

static sStateMachineType const * const gpasTaskHttpCli_StateMachine[] =
{
	gasTaskHttpCli_Idling,
	gasTaskHttpCli_Initializing,
	gasTaskHttpCli_Syncing,
	gasTaskHttpCli_Communicating
};

/* Define a callback function that will be used by multiple timer
 instances.  The callback function does nothing but count the number
 of times the associated timer expires, and stop the timer once the
 timer has expired 10 times.  The count is saved as the ID of the
 timer. */

 void vTimerCallbackHttp( TimerHandle_t xTimer )
 {
	 const uint32_t ulMaxExpiryCountBeforeStopping = 1;
	 uint32_t ulCount;

    /* Optionally do something if the pxTimer parameter is NULL. */
    configASSERT( xTimer );


    /* The number of times this timer has expired is saved as the
    timer's ID.  Obtain the count. */
    ulCount = ( uint32_t ) pvTimerGetTimerID( xTimer );

    /* Increment the count, then test to see if the timer has expired
    ulMaxExpiryCountBeforeStopping yet. */
    ulCount++;

	ESP_LOGI(TAG,"pcTimerGetTimerName =%s\r\n",pcTimerGetTimerName(xTimer));

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
    		stHttpCliMsg.ucSrc = SRC_HTTPCLI;
    		stHttpCliMsg.ucDest = SRC_HTTPCLI;
    		stHttpCliMsg.ucEvent = EVENT_HTTPCLI_POLLING;
    		xQueueSend( xQueueHttpCli, ( void * )&stHttpCliMsg, 0);
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

static void http_task(void *pvParameters)
{
	TickType_t elapsed_time;
	uint32_t EventBits_t;
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
			vTimerCallbackHttp
		);
	}

	for( ;; )
	{
		elapsed_time = xTaskGetTickCount();

		EventBits_t = app_wifi_get_status();
		if(EventBits_t & 0x00000001)
		{
				/*CONNECTED*/
		    	ESP_LOGI(TAG, "HTTP CONN!\r\n");
				app_wifi_clear_connected_bit();

				stHttpCliMsg.ucSrc = SRC_HTTPCLI;
				stHttpCliMsg.ucDest = SRC_HTTPCLI;
				stHttpCliMsg.ucEvent = EVENT_HTTPCLI_CONNECTED;
				xQueueSend( xQueueHttpCli, ( void * )&stHttpCliMsg, 0);
		}
		else
		{
			if(EventBits_t & 0x00000002)
			{
				/*DISCONNECTED*/
			    ESP_LOGI(TAG, "HTTP DISCON\r\n");
				app_wifi_clear_disconnected_bit();

				stHttpCliMsg.ucSrc = SRC_HTTPCLI;
				stHttpCliMsg.ucDest = SRC_HTTPCLI;
				stHttpCliMsg.ucEvent = EVENT_HTTPCLI_DISCONNECTED;
				xQueueSend( xQueueHttpCli, ( void * )&stHttpCliMsg, 0);
			}
		}

		if( xQueueReceive( xQueueHttpCli, &(stHttpCliMsg ),0 ))
		{
			(void)eEventHandler ((unsigned char)SRC_HTTPCLI,gpasTaskHttpCli_StateMachine[ucCurrentStateHttpCli], &ucCurrentStateHttpCli, &stHttpCliMsg);
		}

		vTaskDelayUntil(&elapsed_time, 1000 / portTICK_PERIOD_MS);
	}
}

void Http_Init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

	xQueueHttpCli = xQueueCreate(httcliQUEUE_LENGTH,			/* The number of items the queue can hold. */
								sizeof( sMessageType ) );	/* The size of each item the queue holds. */

	stHttpCliMsg.ucSrc = SRC_HTTPCLI;
	stHttpCliMsg.ucDest = SRC_HTTPCLI;
	stHttpCliMsg.ucEvent = EVENT_HTTPCLI_INIT;
	xQueueSend( xQueueHttpCli, ( void * )&stHttpCliMsg, 0);

    app_wifi_initialise();

    xTaskCreate(&http_task, "http_task", 4096, NULL, configMAX_PRIORITIES-4, NULL);

}


