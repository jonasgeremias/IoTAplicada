/*
	Autor: Prof. Vagner Rodrigues
	Objetivo: Manipulação das GPIOs utilizando SDK-IDF com RTOS (FreeRTOS)
			  Utilizando interrupção externa
	Disciplina: IoT Aplicada
	Curso: Engenharia da Computação
*/

/* Inclusão das Bibliotecas */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

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

/* Protótipos de Funções */
void app_main( void );
void task_GPIO_Blink( void *pvParameter );
static void IRAM_ATTR gpio_isr_handler( void *arg );

/* Variáveis Globais */
static const char * TAG = "main: ";
const char * msg[2] = {"Desligado","Ligado"};
volatile int contador=0; 
 
void task_GPIO_Blink( void *pvParameter )
{
	 /*  Parâmetros de controle da GPIO da função "gpio_set_direction"
		GPIO_MODE_OUTPUT       			//Saída
		GPIO_MODE_INPUT        			//Entrada
		GPIO_MODE_INPUT_OUTPUT 			//Dreno Aberto
    */
	gpio_pad_select_gpio( LED_R ); 	
	gpio_set_direction( LED_R, GPIO_MODE_OUTPUT );	
	bool estado = 0; 
	
    while ( TRUE ) 
    {		
		estado = !estado;
        if( DEBUG )
            ESP_LOGI(TAG, "Led Red: %s", msg[estado] );
        gpio_set_level( LED_R, estado ); 				
		
        vTaskDelay( 2000 / portTICK_RATE_MS ); //Delay de 2000ms liberando scheduler;
	}
}	

/* ISR (função de callback) esta função de IRQ será chamada sempre que acontecer uma interrupção. */
static void IRAM_ATTR gpio_isr_handler( void* arg )
{	
	//Verifica qual botão ativou a interrupção.
	if( BUTTON == (uint32_t) arg )
	{			
		gpio_set_level(LED_G,contador%2);				
	}
	contador++;	
}

/* Aplicação Principal (Inicia após bootloader) */
void app_main( void )
{	
	/*  Parâmetros de controle da GPIO da função "gpio_set_direction"
		GPIO_MODE_OUTPUT       			//Saída
		GPIO_MODE_INPUT        			//Entrada
		GPIO_MODE_INPUT_OUTPUT 			//Dreno Aberto
    */	
	gpio_config_t output_conf = {
		.intr_type = GPIO_PIN_INTR_DISABLE, //Desabilita interrupção externa.
		.mode = GPIO_MODE_OUTPUT, //Configura GPIO como saídas.
		.pin_bit_mask = GPIO_OUTPUT_PIN_SEL //Carrega GPIO configuradas.
	};
    gpio_config( &output_conf );  //Configura GPIO conforme descritor.
    
	/*	Parâmetros de controle da resistência interna na função "gpio_set_pull_mode"
		GPIO_PULLUP_ONLY,               // Pad pull up            
		GPIO_PULLDOWN_ONLY,             // Pad pull down          
		GPIO_PULLUP_PULLDOWN,           // Pad pull up + pull down
		GPIO_FLOATING,                  // Pad floating  
	*/
	/*
		GPIO_INTR_DISABLE = 0,     		// Desabilita interrupção da GPIO                            
		GPIO_INTR_POSEDGE = 1,     		// Habilita interrupção na borda de subida                  
		GPIO_INTR_NEGEDGE = 2,     		// Habilita interrupção na borda de descida               
		GPIO_INTR_ANYEDGE = 3,     		// Habilita interrupção em ambas as borda 
	*/
	gpio_config_t input_conf = {
		.intr_type = GPIO_INTR_NEGEDGE,  //Habilita interrupção na borda de descida 
		.mode = GPIO_MODE_INPUT, //Configura GPIO como entradas.
		.pin_bit_mask = GPIO_INPUT_PIN_SEL, //Carrega GPIO configuradas.
		.pull_down_en = GPIO_PULLDOWN_DISABLE, //Desabilita Pull-down das GPIO's.
		.pull_up_en = GPIO_PULLUP_ENABLE //Habilita Pull-up das GPIO's.
    };
	gpio_config(&input_conf);    //Configura GPIO conforme descritor.

	//Habilita a interrupção externa da(s) GPIO's. 
	//Ao utilizar a função gpio_install_isr_service todas as interrupções de GPIO do descritor vão chamar a mesma 
	//interrupção. A função de callback que será chamada para cada interrupção é definida em gpio_isr_handler_add. 
	gpio_install_isr_service(0);

	//Registra a interrupção externa do BUTTON
    gpio_isr_handler_add( BUTTON, gpio_isr_handler, (void*) BUTTON ); 	

	// Cria a task responsável pelo blink LED. 
	if( (xTaskCreate( task_GPIO_Blink, "task_GPIO_Blink", 2048, NULL, 1, NULL )) != pdTRUE )
    {
      if( DEBUG )
        ESP_LOGI( TAG, "error - Nao foi possivel alocar task_GPIO_Blink.\r\n" );  
      return;   
    }	
}