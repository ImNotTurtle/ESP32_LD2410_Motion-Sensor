#include "Queue.hpp"
#include <stdlib.h>


queue_p Queue_Init(uint16_t bufferSize){
	queue_p queue = (queue_p)malloc(1 * sizeof(queue_t));
	queue->buffer = (uint8_t*)malloc(bufferSize * sizeof(uint8_t));
	queue->bufferSize = bufferSize;
	queue->frontPos = 0;
	queue->backPos = 0;
	return queue;
}
bool Queue_IsEmpty(queue_p queue){
	return queue->frontPos == queue->backPos;
}
bool Queue_IsFull(queue_p queue){
	if(queue->backPos > queue->frontPos){
		return queue->backPos - queue->frontPos == queue->bufferSize;
	}
	else{
		return queue->frontPos - queue->backPos == queue->bufferSize;
	}
	return false;
}
void Queue_EnQueue(queue_p queue, uint8_t data){
	queue->buffer[queue->backPos++] = data;
	if(queue->backPos >= queue->bufferSize){
		queue->backPos = 0;
	}
}
bool Queue_DeQueue(queue_p queue, uint8_t *data){
	if(Queue_IsEmpty(queue)){
		return false;
	}

	if(data != NULL){
		*data = queue->buffer[queue->frontPos];
	}
	queue->frontPos++;

	if(queue->frontPos >= queue->bufferSize){
		queue->frontPos = 0;
	}
	return true;
}
bool Queue_GetFront(queue_p queue, uint8_t *data){
	if(Queue_IsEmpty(queue)){
		return false;
	}
	*data = queue->buffer[queue->frontPos];
	return true;
}
