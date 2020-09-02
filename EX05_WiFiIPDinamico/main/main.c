/*
	Autor: Prof. Vagner Rodrigues
	Objetivo: Configurando rede WiFi com o ESP32 em modo cliente (Station)
			  Configuração com IP dinâmico
	Disciplina: IoT Aplicada
	Curso: Engenharia da Computação
*/

/* This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/* Inclusão das Bibliotecas */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"

/* Definições e Constantes */
#define TRUE          	1 
#define FALSE		  	0
#define DEBUG         	TRUE
#define LED_R			GPIO_NUM_15 
#define LED_G			GPIO_NUM_12
#define LED_B 			GPIO_NUM_14
#define BUTTON			GPIO_NUM_16 
#define GPIO_OUTPUT_PIN_SEL  	((1ULL<<LED_G) | (1ULL<<LED_B))
#define GPIO_INPUT_PIN_SEL  	(1ULL<<BUTTON)
/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      "Vagner Wi-Fi"
#define EXAMPLE_ESP_WIFI_PASS      "10111321"
#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group; //Cria o objeto do grupo de eventos

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

/* Protótipos de Funções */
void app_main( void );
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void task_ip( void *pvParameter );
void wifi_init_sta( void );

/* Variáveis Globais */
static const char *TAG = "wifi station";
static int s_retry_num = 0;

/*
  Função de callback responsável em receber as notificações durante as etapas de conexão do WiFi.
  Por meio desta função de callback podemos saber o momento em que o WiFi do ESP32 foi inicializado com sucesso
  até quando é recebido o aceite do IP pelo roteador (no caso de Ip dinâmico).
  ref: https://github.com/espressif/esp-idf/tree/c77c4ccf6c43ab09fd89e7c907bf5cf2a3499e3b/examples/wifi/getting_started/station
*/
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		if( DEBUG )
		    ESP_LOGI(TAG, "Tentando conectar ao WiFi...\r\n");
		/*
			O WiFi do ESP32 foi configurado com sucesso. 
			Agora precisamos conectar a rede WiFi local. Portanto, foi chamado a função esp_wifi_connect();
		*/
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
			/*
			Se chegou aqui foi devido a falha de conexão com a rede WiFi.
			Por esse motivo, haverá uma nova tentativa de conexão WiFi pelo ESP32.
			*/
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Tentando reconectar ao WiFi...");
        } else {
			/*
				É necessário apagar o bit para avisar as demais Tasks que a 
				conexão WiFi está offline no momento. 
			*/
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"Falha ao conectar ao WiFi");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		/*
			Conexão efetuada com sucesso. Busca e imprime o IP atribuido. 
		*/
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Conectado! O IP atribuido é:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
		/*
				Seta o bit indicativo para avisar as demais Tasks que o WiFi foi conectado. 
		*/
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

 /* Inicializa o WiFi em modo cliente (Station) */
void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate(); //Cria o grupo de eventos

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Conectado ao AP SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Falha ao conectar ao AP SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "EVENTO INESPERADO");
    }

    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    //vEventGroupDelete(s_wifi_event_group);
}

void task_ip( void *pvParameter )
{
    
    if( DEBUG )
      ESP_LOGI( TAG, "Inicializada task_ip...\r\n" );
	
    while (TRUE) 
    {    
		/* o EventGroup bloqueia a task enquanto a rede WiFi não for configurada */
		xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);	
    
		/* Após configurada a rede WiFi recebemos e imprimimos a informação do IP atribuido */
		tcpip_adapter_ip_info_t ip_info;
	    ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info));  
		
		/* Imprime o IP, bloqueia a task e repete a a impressão a cada 5 segundos */
    	if( DEBUG )
      		ESP_LOGI( TAG, "IP atribuido:  %s\n", ip4addr_ntoa(&ip_info.ip) );
		vTaskDelay( 5000/portTICK_PERIOD_MS );
    }
}

/* Aplicação Principal (Inicia após bootloader) */
void app_main(void)
{
    /*	Sempre que utilizar o WiFi ou o Bluetooth adicione o bloco a seguir para inicializar a NVS (Non-volatile storage).
	    Este espaço de memória reservado armazena dados necessários para a calibração do PHY.	
		Devido ao fato de o ESP não possuir EEPROM é necessário separar um pedaço da memória de programa para armazenar
		dados não voláteis*/
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
	//Configura e inicializa o WiFi.
    wifi_init_sta();
	// Cria a task responsável por imprimir o IP recebido do roteador.
    if(xTaskCreate( task_ip, "task_ip", 2048, NULL, 5, NULL )!= pdTRUE )
	{
		if( DEBUG )
			ESP_LOGI( TAG, "error - nao foi possivel alocar Task_IP.\n" );	
		return;		
	}
}
