// #define _DEBUG_
// #define _MSG_DEBUG_
// # define _TRANS_DEBUG_
#include "sender.h" // sender.h
#include "util.h"

// 初始化sender
void init_sender(Sender *sender, int id)
{
    //TODO: You should fill in this function as necessary
    sender->send_id = id;
    sender->input_cmdlist_head = NULL;
    sender->input_framelist_head = NULL;
    sender->buffer_framelist_head = NULL;
    sender->LAR = -1;
    sender->LFS = -1;
    for (int i = 0; i < SWS; i++) {
        sender->SendQ[i].wait_ack = 0;
        sender->SendQ[i].frame = NULL;
        sender->SendQ[i].timeout = malloc(sizeof(struct timeval));
    }
}

// 计算超时时间
struct timeval *sender_get_next_expiring_timeval(Sender *sender)
{
    //TODO: You should fill in this function so that it returns the next timeout that should occur
    struct timeval* current_time = ( struct timeval*) malloc(sizeof(struct timeval));//当前时间
    gettimeofday(current_time, NULL);
    struct timeval* next_time = ( struct timeval*) malloc(sizeof(struct timeval));//下一个接收的时间
    next_time->tv_sec = -1;//下一个接受时间的初始化
    next_time->tv_usec = -1;
    long diff = __LONG_MAX__;
    if(sender->LAR != sender->LFS)// 对尚未确认的帧（LAR<LFS)获取其下一次超时时间
    {
        for(int i = sender->LAR + 1; i <= sender->LFS; i++)
        {
            struct timeval* current_frame_time = sender->SendQ[i % SWS].timeout; // 获得当前未确认帧LAR的时间戳
            if(current_frame_time == NULL) continue;
            long cur_diff = timeval_usecdiff(current_time,current_frame_time);
            if(cur_diff < diff)
            {
                diff = cur_diff;
                next_time->tv_usec = current_frame_time->tv_usec;
                next_time->tv_sec = current_frame_time->tv_sec;
            }
        }
    }
    if(next_time->tv_sec == -1 || next_time->tv_usec == -1)
    {
        return NULL;
    }
    return next_time;
}


// 处理receiver发来的ack确认
void handle_incoming_acks(Sender *sender,
                          LLnode **outgoing_frames_head_ptr)
{
    //TODO: Suggested steps for handling incoming ACKs
    //    1) Dequeue the ACK from the sender->input_framelist_head
    //    2) Convert the char * buffer to a Frame data type
    //    3) Check whether the frame is corrupted
    //    4) Check whether the frame is for this sender
    //    5) Do sliding window protocol for sender/receiver pair
    int incoming_acks_length = ll_get_length(sender->input_framelist_head);
    while (incoming_acks_length > 0)
    {
    #ifdef _TRANS_DEBUG_
        printf("solve ack\n");
    #endif
        LLnode *ll_inmsg_node = ll_pop_node(&sender->input_framelist_head);
        incoming_acks_length = ll_get_length(sender->input_framelist_head);

        char *raw_char_buf = (char *)ll_inmsg_node->value;
        Frame *inframe = convert_char_to_frame(raw_char_buf);
        free(raw_char_buf);
        // crc冗余检测
        uint16_t crc = CRC16_XMODEM((uint8_t*)inframe, MAX_FRAME_SIZE);
        // uint8_t crc = crc8((uint8_t*)inframe, MAX_FRAME_SIZE);
        if (crc)
        {
        #ifdef _DEBUG_
            printf("ACK Recvive Corrupted\n");
        #endif
            free(ll_inmsg_node);
            continue;
        }
        // Frame *inframe = convert_char_to_frame(raw_char_buf);
        if (inframe->ackNum != -1)
        {
            int length = 0;
            // 滑动窗口
            if (sender->send_id == inframe->dst_id)
            {
                // printf("receive ack: %d\n", inframe->ackNum);
            #ifdef _TRANS_DEBUG_
                printf("receive ack: %d\n", inframe->ackNum);
            #endif
                // 处理收到ack确认的帧
                if(sender->LAR < sender->LFS)
                {
                    for(int i = sender->LAR + 1; i < inframe->ackNum; i++)
                    {
                        sender->SendQ[i % RWS].wait_ack = 0;
                        sender->SendQ[i % RWS].frame = NULL;
                        ++length;
                    }
                }
                sender->LAR = inframe->ackNum - 1;
                // 将缓冲区中的分块数据先加入窗口
                while (length--)
                {
                    LLnode* next_node = ll_pop_node(&sender->buffer_framelist_head);
                    if (next_node == NULL)
                    {
                        return;
                    }
                    char* next_sending_frame = next_node->value;
                    if(next_sending_frame != NULL)
                    {
                        ll_append_node(outgoing_frames_head_ptr, next_sending_frame);
                        Frame * outgoing_frame = (Frame *)malloc(sizeof(Frame));
                        outgoing_frame = convert_char_to_frame(next_sending_frame);
                        sender->SendQ[outgoing_frame->seqNum % SWS].frame = outgoing_frame;
                        sender->SendQ[outgoing_frame->seqNum % SWS].wait_ack = 1;
                        calculate_timeout(sender->SendQ[outgoing_frame->seqNum % RWS].timeout);
                        sender->LFS++;
                    }
                    return;
                }
                
            }
        }
        
    }
}

void handle_input_cmds(Sender *sender,
                       LLnode **outgoing_frames_head_ptr)
{
    //TODO: Suggested steps for handling input cmd
    //    1) Dequeue the Cmd from sender->input_cmdlist_head
    //    2) Convert to Frame
    //    3) Set up the frame according to the sliding window protocol
    //    4) Compute CRC and add CRC to Frame

    //Recheck the command queue length to see if stdin_thread dumped a command on us
    // ll_split_head(&sender->input_cmdlist_head, &sender->buffer_cmdlist_head, FRAME_PAYLOAD_SIZE - 1);
    int input_cmd_length = ll_get_length(sender->input_cmdlist_head);
    while (input_cmd_length > 0)
    {
        //Pop a node off and update the input_cmd_length
        // 获取发送者id，接受者id和消息内容
        LLnode *ll_input_cmd_node = ll_pop_node(&sender->input_cmdlist_head);
        input_cmd_length = ll_get_length(sender->input_cmdlist_head);

        //Cast to Cmd type and free up the memory for the node
        Cmd *outgoing_cmd = (Cmd *)ll_input_cmd_node->value;
        free(ll_input_cmd_node);

        //DUMMY CODE: Add the raw char buf to the outgoing_frames list
        //NOTE: You should not blindly send this message out!
        //      Ask yourself: Is this message actually going to the right receiver (recall that default behavior of send is to broadcast to all receivers)?
        //                    Does the receiver have enough space in in it's input queue to handle this message?
        //                    Were the previous messages sent to this receiver ACTUALLY delivered to the receiver?
        int msg_length = strlen(outgoing_cmd->message);
        // printf("test");
        // 数据长度大于最大数据容量，进行数据分块，否则直接进行发送
        if (msg_length > MAX_DATA_SIZE && ll_get_length(sender->buffer_framelist_head) == 0)
        {
            //Do something about messages that exceed the frame size
            // printf("<SEND_%d>: sending messages of length greater than %d is not implemented\n", sender->send_id, MAX_FRAME_SIZE);
            // 按照最大数据容量进行数据分块，并存入帧。
            int length = strlen(outgoing_cmd->message) / (MAX_DATA_SIZE - 1);
            int block = strlen(outgoing_cmd->message) % (MAX_DATA_SIZE - 1) == 0 ? length : length + 1;
            for(int i = 0; i < block; i++)
            {
                char *msg = outgoing_cmd->message;
                Frame * outgoing_frame = (Frame *) malloc (sizeof(Frame));
                memset(outgoing_frame->data, '\0', sizeof(outgoing_frame->data));
                strncpy(outgoing_frame->data, msg + (i * (MAX_DATA_SIZE - 1)), MAX_DATA_SIZE - 1);
                outgoing_frame->src_id = outgoing_cmd->src_id;
                outgoing_frame->dst_id = outgoing_cmd->dst_id;
                outgoing_frame->seqNum = sender->LFS + 1;
                outgoing_frame->ackNum = -1;
                outgoing_frame->is_end = i == block - 1 ? 1 : 0;
                uint16_t crc = CRC16_XMODEM((uint8_t*)outgoing_frame, MAX_FRAME_SIZE - 2);
                outgoing_frame->Hcheck = crc >> 8;
                outgoing_frame->Lcheck = crc & 0x00FF;
                // uint8_t crc = crc8((uint8_t*)outgoing_frame, MAX_FRAME_SIZE - 1);
                // outgoing_frame->CRC = crc;
            #ifdef _MSG_DEBUG_
                printf("send str: %s\n", outgoing_frame->data);
            #endif
                // print_frame(outgoing_frame);
                char *outgoing_charbuf = convert_frame_to_char(outgoing_frame);
                // append_crc(outgoing_charbuf, MAX_FRAME_SIZE);
                // 如果滑动窗口能够装下数据，这直接加入滑动窗口，否则加入缓冲区，等待确认后发送
                if(sender->LFS - sender->LAR < SWS)
                {
                #ifdef _TRANS_DEBUG_
                    printf("send data: %d: %s\n",outgoing_frame->seqNum, outgoing_frame->data);
                #endif
                    ll_append_node(outgoing_frames_head_ptr, outgoing_charbuf);
                    int index = outgoing_frame->seqNum % SWS;
                    calculate_timeout(sender->SendQ[index].timeout);
                    sender->SendQ[index].frame = outgoing_frame;
                    sender->SendQ[index].wait_ack = 1; 
                    sender->LFS++;
                }
                else
                {
                    ll_append_node(&sender->buffer_framelist_head, outgoing_charbuf);
                }
            }
        }
        else
        {
            //This is probably ONLY one step you want
            // 设置帧的内容
            Frame *outgoing_frame = (Frame *)malloc(sizeof(Frame));
            strcpy(outgoing_frame->data, outgoing_cmd->message);
            outgoing_frame->src_id = outgoing_cmd->src_id;
            outgoing_frame->dst_id = outgoing_cmd->dst_id;
            outgoing_frame->seqNum = sender->LFS + 1;
            // printf("seqnum : %d\n", outgoing_frame->seqNum);
            outgoing_frame->ackNum = -1;
            outgoing_frame->is_end = 1;
            uint16_t crc = CRC16_XMODEM((uint8_t*)outgoing_frame, MAX_FRAME_SIZE - 2);
            outgoing_frame->Hcheck = crc >> 8;
            outgoing_frame->Lcheck = crc & 0x00FF;
            // uint8_t crc = crc8((uint8_t*)outgoing_frame, MAX_FRAME_SIZE - 1);
            // outgoing_frame->CRC = crc;
            //At this point, we don't need the outgoing_cmd
            free(outgoing_cmd->message);
            free(outgoing_cmd);

            //Convert the message to the outgoing_charbuf，add crc and send
            char *outgoing_charbuf = convert_frame_to_char(outgoing_frame);
            ll_append_node(outgoing_frames_head_ptr, outgoing_charbuf);
        #ifdef _DEBUG_
            printf("send data: %d\n", outgoing_frame->seqNum);
        #endif
            // printf("send data: %d\n", outgoing_frame->seqNum);
            // 添加到滑动窗口中，计算超时时间
            int index = outgoing_frame->seqNum % SWS;
            sender->SendQ[index].wait_ack = 1;
            calculate_timeout(sender->SendQ[index].timeout);
            sender->SendQ[index].frame = outgoing_frame;
            sender->LFS++;
        }
    }
}

void handle_timedout_frames(Sender *sender,
                            LLnode **outgoing_frames_head_ptr)
{
    //TODO: Suggested steps for handling timed out datagrams
    //    1) Iterate through the sliding window protocol information you maintain for each receiver
    //    2) Locate frames that are timed out and add them to the outgoing frames
    //    3) Update the next timeout field on the outgoing frames
    struct timeval curr_time;
    gettimeofday(&curr_time,NULL);
    // 判断是否超时，如果超时，从超时帧开始重新发送
    for(int i = (uint8_t)(sender->LAR + 1); i <= sender->LFS; i++){
        if(sender->SendQ[i % SWS].wait_ack == 1  &&
                ((sender->SendQ[i % SWS].timeout->tv_sec < curr_time.tv_sec) ||
                    (sender->SendQ[i % SWS].timeout->tv_sec == curr_time.tv_sec &&
                     sender->SendQ[i % SWS].timeout->tv_usec < curr_time.tv_usec))){
        // #ifdef _DEBUG_
        //     printf("retrans range: %d - %d\n", i, sender->LFS);
        // #endif
            // 重新发送超时帧以及之后的帧
            for(int j = i; j <= sender->LFS; j++)
            {
            #ifdef _DEBUG_
                printf("resend frame: %d\n", j); 
            #endif
                // Frame * resend_frame = (Frame *) malloc(sizeof(Frame));
                // memcpy(resend_frame, sender->SendQ[j % SWS].frame,sizeof(Frame));
                char * resend_charbuf = convert_frame_to_char(sender->SendQ[j % SWS].frame);
                ll_append_node(outgoing_frames_head_ptr,resend_charbuf);
                // free(resend_frame);
                calculate_timeout(sender->SendQ[j % SWS].timeout);
            }
            break;
        }
    }
}

void *run_sender(void *input_sender)
{
    struct timespec time_spec;
    struct timeval curr_timeval;
    const int WAIT_SEC_TIME = 0;
    const long WAIT_USEC_TIME = 100000;
    Sender *sender = (Sender *)input_sender;
    LLnode *outgoing_frames_head;
    struct timeval *expiring_timeval;
    long sleep_usec_time, sleep_sec_time;

    //This incomplete sender thread, at a high level, loops as follows:
    //1. Determine the next time the thread should wake up
    //2. Grab the mutex protecting the input_cmd/inframe queues
    //3. Dequeues messages from the input queue and adds them to the outgoing_frames list
    //4. Releases the lock
    //5. Sends out the messages

    pthread_cond_init(&sender->buffer_cv, NULL);
    pthread_mutex_init(&sender->buffer_mutex, NULL);

    while (1)
    {
        outgoing_frames_head = NULL;

        //Get the current time
        gettimeofday(&curr_timeval,
                     NULL);

        //time_spec is a data structure used to specify when the thread should wake up
        //The time is specified as an ABSOLUTE (meaning, conceptually, you specify 9/23/2010 @ 1pm, wakeup)
        time_spec.tv_sec = curr_timeval.tv_sec;
        time_spec.tv_nsec = curr_timeval.tv_usec * 1000;

        //Check for the next event we should handle
        expiring_timeval = sender_get_next_expiring_timeval(sender);

        //Perform full on timeout
        if (expiring_timeval == NULL)
        {
            time_spec.tv_sec += WAIT_SEC_TIME;
            time_spec.tv_nsec += WAIT_USEC_TIME * 1000;
        }
        else
        {
            //Take the difference between the next event and the current time
            sleep_usec_time = timeval_usecdiff(&curr_timeval,
                                               expiring_timeval);

            //Sleep if the difference is positive
            if (sleep_usec_time > 0)
            {
                sleep_sec_time = sleep_usec_time / 1000000;
                sleep_usec_time = sleep_usec_time % 1000000;
                time_spec.tv_sec += sleep_sec_time;
                time_spec.tv_nsec += sleep_usec_time * 1000;
            }
        }

        //Check to make sure we didn't "overflow" the nanosecond field
        if (time_spec.tv_nsec >= 1000000000)
        {
            time_spec.tv_sec++;
            time_spec.tv_nsec -= 1000000000;
        }

        //*****************************************************************************************
        //NOTE: Anything that involves dequeing from the input frames or input commands should go
        //      between the mutex lock and unlock, because other threads CAN/WILL access these structures
        //*****************************************************************************************
        pthread_mutex_lock(&sender->buffer_mutex);

        //Check whether anything has arrived
        int input_cmd_length = ll_get_length(sender->input_cmdlist_head);
        int inframe_queue_length = ll_get_length(sender->input_framelist_head);

        //Nothing (cmd nor incoming frame) has arrived, so do a timed wait on the sender's condition variable (releases lock)
        //A signal on the condition variable will wakeup the thread and reaquire the lock
        if (input_cmd_length == 0 &&
            inframe_queue_length == 0)
        {
            pthread_cond_timedwait(&sender->buffer_cv,
                                   &sender->buffer_mutex,
                                   &time_spec);
        }
        // printf("wake up!\n");
        //Implement this
        handle_incoming_acks(sender,
                             &outgoing_frames_head);

        //Implement this
        handle_input_cmds(sender,
                          &outgoing_frames_head);

        pthread_mutex_unlock(&sender->buffer_mutex);

        //Implement this
        handle_timedout_frames(sender,
                               &outgoing_frames_head);

        //CHANGE THIS AT YOUR OWN RISK!
        //Send out all the frames
        int ll_outgoing_frame_length = ll_get_length(outgoing_frames_head);

        while (ll_outgoing_frame_length > 0)
        {
            LLnode *ll_outframe_node = ll_pop_node(&outgoing_frames_head);
            char *char_buf = (char *)ll_outframe_node->value;

            //Don't worry about freeing the char_buf, the following function does that
            send_msg_to_receivers(char_buf);

            //Free up the ll_outframe_node
            free(ll_outframe_node);

            ll_outgoing_frame_length = ll_get_length(outgoing_frames_head);
        }
    }
    pthread_exit(NULL);
    return 0;
}
