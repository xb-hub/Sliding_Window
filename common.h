
#ifndef __COMMON_H__
#define __COMMON_H__

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

#define MAX_COMMAND_LENGTH 16
#define AUTOMATED_FILENAME 512
#define MAX_BUFFER_SIZE 256
#define SWS 8   // 发送窗口大小
#define RWS 8   // 接受窗口大小
typedef unsigned char uchar_t;

//System configuration information
struct SysConfig_t
{
    float drop_prob;
    float corrupt_prob;
    unsigned char automated;
    char automated_file[AUTOMATED_FILENAME];
};
typedef struct SysConfig_t  SysConfig;

//Command line input information
struct Cmd_t
{
    uint16_t src_id;
    uint16_t dst_id;
    char * message;
    uint8_t is_end;
};
typedef struct Cmd_t Cmd;

//Linked list information
enum LLtype 
{
    llt_string,
    llt_frame,
    llt_integer,
    llt_head
} LLtype;

struct LLnode_t
{
    struct LLnode_t * prev;
    struct LLnode_t * next;
    enum LLtype type;

    void * value;
};
typedef struct LLnode_t LLnode;

#define MAX_FRAME_SIZE 32       // 最大帧长度
#define MAX_DATA_SIZE 23        // 最大数据长度

//TODO: You should change this!
//Remember, your frame can be AT MOST 32 bytes!
#define FRAME_PAYLOAD_SIZE 32

struct Frame_t
{
    uint16_t src_id;            // 发送帧的id
    uint16_t dst_id;            // 接受帧的id
    char data[MAX_DATA_SIZE];   // 数据
    uint8_t seqNum;       // 数据帧序号
    uint8_t ackNum;       // 确认帧，期待发送方所要发送的数据帧序号
    uint8_t is_end;       // 判断是否拆分帧的结尾
    // unsigned char CRC;          // 冗余码
    uint8_t Hcheck;
    uint8_t Lcheck;
};
typedef struct Frame_t Frame;

typedef struct ReceiveWindow_t
{
    unsigned char is_received;              // 当前存放的帧是否被接收
    Frame *frame;                           // 帧内容
} ReceiveWindow;

//Receiver and sender data structures
struct Receiver_t
{
    //DO NOT CHANGE:
    // 1) buffer_mutex
    // 2) buffer_cv
    // 3) input_framelist_head
    // 4) recv_id
    pthread_mutex_t buffer_mutex;
    pthread_cond_t buffer_cv;
    LLnode * input_framelist_head;
    LLnode * buffer_framelist_head;
    unsigned char NFE;                      // 期待的下一帧号
    unsigned char recv_id;                  // 接收方id
    char *data_result[MAX_BUFFER_SIZE];     // 用于存放拼接后的数据结果
    ReceiveWindow RecvQ[RWS];               // 接收方滑动窗口
};

typedef struct SendWindow_t
{
    unsigned char wait_ack;                 // 当前存放的帧是否被确认
    struct timeval * timeout;               // 超时时间
    Frame *frame;                           // 帧
} SendWindow;

struct Sender_t
{
    //DO NOT CHANGE:
    // 1) buffer_mutex
    // 2) buffer_cv
    // 3) input_cmdlist_head
    // 4) input_framelist_head
    // 5) send_id
    pthread_mutex_t buffer_mutex;           // 缓冲区访问互斥量
    pthread_cond_t buffer_cv;               // 缓冲区条件变量
    LLnode * input_cmdlist_head;            // 存放输入命令(命令，发送id，接收id，消息）到循环链表
    LLnode * input_framelist_head;          // 存放帧的循环链表
    LLnode * buffer_framelist_head;         // 数据过大分区存放缓冲区
    unsigned char LAR;                      // 最近确认的帧
    unsigned char LFS;                      // 最近发送的帧
    unsigned char send_id;                  // 发送方id
    SendWindow SendQ[SWS];                  // 发送方滑动窗口，保存已发送但未确认的帧
};

enum SendFrame_DstType 
{
    ReceiverDst,
    SenderDst
} SendFrame_DstType ;

typedef struct Sender_t Sender;
typedef struct Receiver_t Receiver;

//Declare global variables here
//DO NOT CHANGE: 
//   1) glb_senders_array
//   2) glb_receivers_array
//   3) glb_senders_array_length
//   4) glb_receivers_array_length
//   5) glb_sysconfig
//   6) CORRUPTION_BITS
Sender * glb_senders_array;
Receiver * glb_receivers_array;
int glb_senders_array_length;
int glb_receivers_array_length;
SysConfig glb_sysconfig;
int CORRUPTION_BITS;
#endif 
