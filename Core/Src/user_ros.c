/*
 * user_ros.c
 *
 *  Created on: May 16, 2024
 *      Author: hwabok
 */

#include <std_msgs/msg/string.h>
#include <rcl/rcl.h>
#include "usart.h"

extern rcl_publisher_t publisher_string_scan;
extern rcl_publisher_t publisher_string_pos;
extern UART_HandleTypeDef huart2;

void Publisher_state(){
	std_msgs__msg__String msg;
	char str[20] = "finish";
	msg.data.data = str;
	msg.data.size = strlen(str);
	rcl_publish(&publisher_string_scan, &msg, NULL);
}

/*double type input X,Y */
void Publisher_pos(double x, double y){
	std_msgs__msg__String msg;
	char str[100] = "";
	sprintf(str, "%lf %lf", x,y);
	msg.data.data = str;
	msg.data.size = strlen(str);
	rcl_publish(&publisher_string_pos, &msg, NULL);
}
