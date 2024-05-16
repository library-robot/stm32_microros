/*
 * uart_receive_dma.c
 *
 *  Created on: Feb 8, 2024
 *      Author: Yoo
 */

#include <stdint.h>
#include <string.h>

#include "usart.h"
#include "uart_receive_dma.h"
#define READ_BOOK_MAX_SIZE 100
#define transmitSignal 0x0001
#define transmitSignal2 0x0002
#include "cmsis_os.h"

extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart1;
extern osThreadId tagNumTransmitTHandle;
extern osThreadId defaultTaskHandle;
extern osThreadId motorTaskHandle;
extern osThreadId transmitTask2Handle;
extern osThreadId rfidExecuteTaskHandle;

queue8_t uart_queue;
queue8_t uart2_queue;

void uart_init(){
	HAL_UART_Receive_DMA(&huart1, uart_queue.buf, QUEUE_BUF_MAX) ;
	uart_queue.q_in_index = 0;
	uart_queue.q_out_index = 0;
}

uint32_t uart_available(void){
	uint32_t ret = 0;
	uart_queue.q_in_index = (QUEUE_BUF_MAX - huart1.hdmarx->Instance->NDTR) % QUEUE_BUF_MAX; //원형 큐
//	hdmarx->Instance->CNDTR
	ret = (QUEUE_BUF_MAX + uart_queue.q_in_index - uart_queue.q_out_index) % QUEUE_BUF_MAX; // 버퍼 데이터 개수

	return ret;
}

uint8_t uart_q8_read(void){
	uint8_t ret =0;
	if(uart_queue.q_out_index != uart_queue.q_in_index){
		ret = uart_queue.buf[uart_queue.q_out_index];
		uart_queue.q_out_index = (uart_queue.q_out_index +1) % QUEUE_BUF_MAX;
	}

	return ret;
}

uint8_t rfid_number[READ_BOOK_MAX_SIZE][12] = {0,}; //파싱한 rfid 번호 저장
uint8_t recive_data[24] = {0,}; //RFID 태그 한개에서 receive 한 data 저장
uint8_t book_num = 0;  //책 순서
uint8_t book_byte_num = 0; //책 태그의 바이트 순서
void read_rfid_number(){
	uint8_t i;
	if(uart_available()){ // 데이터 있으면
		uint8_t read_byte =  uart_q8_read(); // 버퍼에서 1byte 읽고
		recive_data[book_byte_num++] = read_byte;
		if(read_byte == 0x7E) { // 마지막 데이터이면
			if(recive_data[1] != 0x01){ //인식이 된 경우 8~19 12byte rfid number
				for( i=8; i<=19; i ++)
					rfid_number[book_num][i-8] = recive_data[i];
				book_num ++;
			}
			book_byte_num = 0;
		}
		if(!uart_available()){ //다 읽었으면
			osSignalSet(tagNumTransmitTHandle, transmitSignal); //전송 이벤트 생성
			//vTaskSuspend(defaultTaskHandle);
			}
		}
}

void transmitData(){
	int i=0;
	while(rfid_number[i][0] != 0){
		//computer
		HAL_UART_Transmit(&huart2, rfid_number[i], sizeof(rfid_number[i]), 500);
		//esp32
		HAL_UART_Transmit(&huart3, rfid_number[i], sizeof(rfid_number[i]), 500);
		i++;
		}
	memset(rfid_number,0,sizeof(rfid_number));
	book_num =0;

}


//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
//	if(huart->Instance == USART2){
//		//buffer write
//		uint16_t q_in_next;
//
//	}
//}

//------------------------------------------------------

void uart2_init(){
	HAL_UART_Receive_DMA(&huart2, uart2_queue.buf, QUEUE_BUF_MAX);
	uart2_queue.q_in_index = 0;
	uart2_queue.q_out_index = 0;
}



uint32_t uart2_available(void){
	uint32_t ret = 0;
	uart2_queue.q_in_index = (QUEUE_BUF_MAX - huart2.hdmarx->Instance->NDTR) % QUEUE_BUF_MAX; //원형 큐
	ret = (QUEUE_BUF_MAX + uart2_queue.q_in_index - uart2_queue.q_out_index) % QUEUE_BUF_MAX; // 버퍼 데이터 개수

	return ret;
}


uint8_t uart2_q8_read(void){
	uint8_t ret =0;
	if(uart2_queue.q_out_index != uart2_queue.q_in_index){
		ret = uart2_queue.buf[uart2_queue.q_out_index];
		uart2_queue.q_out_index = (uart2_queue.q_out_index +1) % QUEUE_BUF_MAX;
	}

	return ret;
}

uint8_t scan_command[20] = {0,}; //파싱한 rfid 번호 저장
uint8_t command_byte_num = 0; //책 태그의 바이트 순서
void read_command(){
	if(uart2_available()){ // 데이터 있으면
		uint8_t read_byte =  uart2_q8_read(); // 버퍼에서 1byte 읽고
		scan_command[command_byte_num++] = read_byte;
		if(read_byte == 0x0A) { // 마지막 데이터이면
			command_byte_num = 0;
		}
		if(!uart2_available()){ //다 읽었으면
			osSignalSet(transmitTask2Handle, transmitSignal2); //전송 이벤트 생성
		}
	}
}

char motor_command1[] = "scan\n";
char motor_command2[] = "stop\n";

void transmitCommand(){
//	HAL_UART_Transmit(&huart2, scan_command, strlen(scan_command), 1);
//	int i=0;
//	while(scan_command[i] != 10){
//		HAL_UART_Transmit(&huart2, scan_command[i], sizeof(scan_command[i]), 1);
//		i++;
//	}
	if(!(strchr(scan_command,motor_command1))){
		HAL_UART_Transmit(&huart2, "ok\r\n", sizeof("ok\r\n"), 1);
		vTaskResume(motorTaskHandle);
		vTaskResume(rfidExecuteTaskHandle);
	}
//	}else if(!(strchr(scan_command,motor_command2))){
//		HAL_UART_Transmit(&huart2, "ok\r\n", sizeof("ok\r\n"), 1);
//		vTaskSuspend(rfidExecuteHandle);
//	}
	memset(scan_command,0,sizeof(scan_command));
}
