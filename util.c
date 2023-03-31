#include "util.h"

//Linked list functions
int ll_get_length(LLnode *head)
{
    LLnode *tmp;
    int count = 1;
    if (head == NULL)
        return 0;
    else
    {
        tmp = head->next;
        while (tmp != head)
        {
            count++;
            tmp = tmp->next;
        }
        return count;
    }
}

// 循环链表
void ll_append_node(LLnode **head_ptr,
                    void *value)
{
    LLnode *prev_last_node;
    LLnode *new_node;
    LLnode *head;

    if (head_ptr == NULL)
    {
        return;
    }

    //Init the value pntr
    head = (*head_ptr);
    new_node = (LLnode *)malloc(sizeof(LLnode));
    new_node->value = value;

    //The list is empty, no node is currently present
    if (head == NULL)
    {
        (*head_ptr) = new_node;
        new_node->prev = new_node;
        new_node->next = new_node;
    }
    else
    {
        //Node exists by itself
        prev_last_node = head->prev;
        head->prev = new_node;
        prev_last_node->next = new_node;
        new_node->next = head;
        new_node->prev = prev_last_node;
    } // 头插法插入节点
}

LLnode *ll_pop_node(LLnode **head_ptr)
{
    LLnode *last_node;
    LLnode *new_head;
    LLnode *prev_head;

    prev_head = (*head_ptr);
    if (prev_head == NULL)
    {
        return NULL;
    }
    last_node = prev_head->prev;
    new_head = prev_head->next;

    //We are about to set the head ptr to nothing because there is only one thing in list
    if (last_node == prev_head)
    {
        (*head_ptr) = NULL;
        prev_head->next = NULL;
        prev_head->prev = NULL;
        return prev_head;
    }
    else
    {
        (*head_ptr) = new_head;
        last_node->next = new_head;
        new_head->prev = last_node;

        prev_head->next = NULL;
        prev_head->prev = NULL;
        return prev_head;
    }
}

void ll_destroy_node(LLnode *node)
{
    if (node->type == llt_string)
    {
        free((char *)node->value);
    }
    free(node);
}

//Compute the difference in usec for two timeval objects
long timeval_usecdiff(struct timeval *start_time,
                      struct timeval *finish_time)
{
    long usec;
    usec = (finish_time->tv_sec - start_time->tv_sec) * 1000000;
    usec += (finish_time->tv_usec - start_time->tv_usec);
    return usec;
}

void calculate_timeout(struct timeval *timeout)
{
    gettimeofday(timeout, NULL); //use this to get the current time
    timeout->tv_usec += 100000;  //0.1s
    if (timeout->tv_usec >= 1000000)
    {
        timeout->tv_usec -= 1000000; //1s
        timeout->tv_sec += 1;
    }
}

//Print out messages entered by the user
void print_cmd(Cmd *cmd)
{
    fprintf(stderr, "src=%d, dst=%d, message=%s\n",
            cmd->src_id,
            cmd->dst_id,
            cmd->message);
}

void print_frame(Frame *frame)
{
    fprintf(stderr, "src=%d, dst=%d, data=%s, is_end=%d\n",
            frame->src_id,
            frame->dst_id,
            frame->data,
            frame->is_end);
}

char *convert_frame_to_char(Frame *frame)
{
    //TODO: You should implement this as necessary
    char *char_buffer = (char *)malloc(sizeof(Frame));
    memset(char_buffer,
           '\0',
           sizeof(Frame));
    memcpy(char_buffer,
           frame,
           sizeof(Frame));
    return char_buffer;
}

Frame *convert_char_to_frame(char *char_buf)
{
    //TODO: You should implement this as necessary
    Frame *frame = (Frame *)malloc(sizeof(Frame));
    memset(frame,
           0,
           sizeof(Frame));
    memcpy(frame,
           char_buf,
           sizeof(Frame));
    return frame;
}

// 计算冗余码
uint16_t CRC16_XMODEM(uint8_t *data, uint32_t datalen) {
    uint16_t wCRCin = 0x0000;
    uint16_t wCPoly = 0x1021;

    while (datalen--) {
        wCRCin ^= (*(data++) << 8);
        for (int i = 0; i < 8; i++) {
            if (wCRCin & 0x8000)
                wCRCin = (wCRCin << 1) ^ wCPoly;
            else
                wCRCin = wCRCin << 1;
        }
    }
    return (wCRCin);
}

uint8_t crc8(uint8_t *data, size_t size) {
  uint8_t crc = 0x00;
  uint8_t poly = 0x07;

  while (size--) {
    crc ^= *data++;
    for (int bit = 0; bit < 8; bit++) {
      if (crc & 0x80) {
        crc = (crc << 1) ^ poly;
      } else {
        crc <<= 1;
      }
    }
  }

  return crc;
}

// void ll_split_head(LLnode **head_ptr, LLnode **buffer_head_ptr, int payload_size)
// {
//     if (head_ptr == NULL || *head_ptr == NULL)
//     {
//         return;
//     }
//     //get the message from the head of the linked list
//     LLnode *head = *head_ptr;
//     Cmd *head_cmd = (Cmd *)head->value;
//     char *msg = head_cmd->message;
//     //do not need to split
//     if (strlen(msg) < payload_size)
//     {
//         return;
//     }
//     // printf("split frame!\n");
//     Cmd *next_cmd;
//     int count = 1;
//     for (int i = payload_size; i < strlen(msg); i += payload_size)
//     {
//         //TODO: malloc here
//         next_cmd = (Cmd *)malloc(sizeof(Cmd));
//         char *cmd_msg = (char *)malloc((payload_size + 1) * sizeof(char)); // One extra byte for NULL character
//         memset(cmd_msg, 0, (payload_size + 1) * sizeof(char));
//         strncpy(cmd_msg, msg + i, payload_size);
//         cmd_msg[payload_size] = '\0';
//         //TODO: fill the next_cmd
//         next_cmd->dst_id = head_cmd->dst_id;
//         next_cmd->src_id = head_cmd->src_id;
//         next_cmd->message = cmd_msg;
//         next_cmd->is_end = (i + payload_size) > strlen(msg) ? 1 : 0;
//         //TODO: fill the next node and add it to the linked list
//         count > SWS ? ll_append_node(buffer_head_ptr, next_cmd) : ll_append_node(head_ptr, next_cmd);
//         count++;
//     }
//     msg[payload_size] = '\0'; //cut the original msg
//     // printf("%s\n", msg);
// }