/*
	Autor: Prof. Vagner Rodrigues
	Objetivo: Manipulação das GPIOs utilizando SDK-IDF com RTOS (FreeRTOS)
			  Configurando GPIOs através de descritores
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
void task_GPIO_Control( void *pvParameter );

/* Variáveis Globais */
static const char * TAG = "main: ";
const char * msg[2] = {"Desligado","Ligado"};
 
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

void task_GPIO_Control( void *pvParameter )
{
	 /*  Parâmetros de controle da GPIO da função "gpio_set_direction"
		GPIO_MODE_OUTPUT       			//Saída
		GPIO_MODE_INPUT        			//Entrada
		GPIO_MODE_INPUT_OUTPUT 			//Dreno Aberto
    */
	
	gpio_config_t io_conf;  //Declaração da variável descritora. 
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE; //Desabilita interrupção externa.
    io_conf.mode = GPIO_MODE_OUTPUT; //Configura GPIO como saídas.
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL; //Carrega GPIO configuradas.
    gpio_config( &io_conf );  //Configura GPIO conforme descritor.
  
  
	/*	Parâmetros de controle da resistência interna na função "gpio_set_pull_mode"
		GPIO_PULLUP_ONLY,               // Pad pull up            
		GPIO_PULLDOWN_ONLY,             // Pad pull down          
		GPIO_PULLUP_PULLDOWN,           // Pad pull up + pull down
		GPIO_FLOATING,                  // Pad floating  
	*/
	io_conf.intr_type = GPIO_PIN_INTR_DISABLE;  //Desabilita interrupção externa.
    io_conf.mode = GPIO_MODE_INPUT; //Configura GPIO como entradas.
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL; //Carrega GPIO configuradas.
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; //Desabilita Pull-down das GPIO's.
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE; //Habilita Pull-up das GPIO's.
    gpio_config(&io_conf);    //Configura GPIO conforme descritor.
    

    while ( TRUE ) 
    {
		if (!gpio_get_level(BUTTON))
		{			
			gpio_set_level(LED_G,1);
			gpio_set_level(LED_B,0);			
		}
		else
		{
			gpio_set_level(LED_G,0);
			gpio_set_level(LED_B,1);			
		}
		vTaskDelay( 10 / portTICK_RATE_MS ); //Delay de 10ms liberando scheduler;
	}
}	

/* Aplicação Principal (Inicia após bootloader) */
void app_main( void )
{	
	// Cria a task responsável pelo blink LED. 
	if( (xTaskCreate( task_GPIO_Blink, "task_GPIO_Blink", 2048, NULL, 1, NULL )) != pdTRUE )
    {
      if( DEBUG )
        ESP_LOGI( TAG, "error - Nao foi possivel alocar task_GPIO_Blink.\r\n" );  
      return;   
    } 
	
	// Cria a task responsável pelo blink controle dos LEDs através do botão. 
	if( (xTaskCreate( task_GPIO_Control, "task_GPIO_Control", 2048, NULL, 1, NULL )) != pdTRUE )
    {
      if( DEBUG )
        ESP_LOGI( TAG, "error - Nao foi possivel alocar task_GPIO_Control.\r\n" );  
      return;   
    } 
}