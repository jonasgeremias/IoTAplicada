/*
	Autor: Prof. Vagner Rodrigues
	Objetivo: Manipulação das GPIOs utilizando SDK-IDF com RTOS (FreeRTOS)
			  Controlando GPIO por Tasks
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
	gpio_pad_select_gpio( LED_G );
	gpio_pad_select_gpio( LED_B ); 	
	gpio_set_direction( LED_G, GPIO_MODE_OUTPUT );	
	gpio_set_direction( LED_B, GPIO_MODE_OUTPUT );	
	
	/*	Parâmetros de controle da resistência interna na função "gpio_set_pull_mode"
		GPIO_PULLUP_ONLY,               // Pad pull up            
		GPIO_PULLDOWN_ONLY,             // Pad pull down          
		GPIO_PULLUP_PULLDOWN,           // Pad pull up + pull down
		GPIO_FLOATING,                  // Pad floating  
	*/
	gpio_pad_select_gpio( BUTTON ); 
	gpio_set_direction( BUTTON, GPIO_MODE_INPUT );
	gpio_set_pull_mode( BUTTON, GPIO_PULLUP_ONLY );
    bool estado = 0;   

    while ( TRUE ) 
    {
		if (!gpio_get_level(BUTTON))
		{
			if( DEBUG )
				ESP_LOGI(TAG, "Botão Pressionado");
			gpio_set_level(LED_G,1);
			gpio_set_level(LED_B,0);			
		}
		else
		{
			gpio_set_level(LED_G,0);
			gpio_set_level(LED_B,1);			
		}			
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