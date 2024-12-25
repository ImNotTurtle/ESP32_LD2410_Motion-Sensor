
#ifndef QUEUE_H_
#define QUEUE_H_

#include <stdint.h>
#include <stdbool.h>


typedef struct{
	uint8_t *buffer;
	uint16_t bufferSize;
	uint16_t frontPos;
	uint16_t backPos;
}queue_t, *queue_p;

queue_p Queue_Init(uint16_t bufferSize);
bool Queue_IsEmpty(queue_p queue);
bool Queue_IsFull(queue_p queue);
void Queue_EnQueue(queue_p queue, uint8_t data);
bool Queue_DeQueue(queue_p queue, uint8_t *data);
bool Queue_GetFront(queue_p queue, uint8_t *data);



#endif
