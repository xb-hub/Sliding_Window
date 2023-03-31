#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <math.h>
#include <sys/time.h>
#include "common.h"

//Linked list functions
int ll_get_length(LLnode *);
void ll_append_node(LLnode **, void *);
LLnode * ll_pop_node(LLnode **);
void ll_destroy_node(LLnode *);

//Print functions
void print_cmd(Cmd *);
void print_frame(Frame *);

//Time functions
long timeval_usecdiff(struct timeval *, 
                      struct timeval *);

//TODO: Implement these functions
char * convert_frame_to_char(Frame *);
Frame * convert_char_to_frame(char *);

// 冗余检测
uint16_t CRC16_XMODEM(uint8_t *data, uint32_t datalen);
uint8_t crc8(uint8_t *data, size_t size);
// char get_bit (char byte, int pos);
void append_crc (uint8_t* data, size_t size);
// int is_corrupted(char* data, size_t size);

// void ll_split_head(LLnode **head_ptr, int payload_size);
void ll_split_head(LLnode **head_ptr, LLnode **buffer_head_ptr, int payload_size);

void calculate_timeout(struct timeval *timeout);
#endif
