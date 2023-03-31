// #define _DEBUG_
// #define _MSG_DEBUG_
#include "receiver.h" // comment receiver.h

// 初始化receiver
void init_receiver(Receiver *receiver,
                   int id)
{
    receiver->recv_id = id;
    receiver->input_framelist_head = NULL;
    receiver->buffer_framelist_head = NULL;
    receiver->NFE = 0;
    for (int i = 0; i < RWS; i++) {
        receiver->RecvQ[i].is_received = 0;
    }
}

// 处理sender发送来的数据
void handle_incoming_msgs(Receiver *receiver,
                          LLnode **outgoing_frames_head_ptr)
{
    //TODO: Suggested steps for handling incoming frames
    //    1) Dequeue the Frame from the sender->input_framelist_head
    //    2) Convert the char * buffer to a Frame data type
    //    3) Check whether the frame is corrupted
    //    4) Check whether the frame is for this receiver
    //    5) Do sliding window protocol for sender/receiver pair

    int incoming_msgs_length = ll_get_length(receiver->input_framelist_head);
    while (incoming_msgs_length > 0)
    {
        //Pop a node off the front of the link list and update the count
        LLnode *ll_inmsg_node = ll_pop_node(&receiver->input_framelist_head);
        incoming_msgs_length = ll_get_length(receiver->input_framelist_head);

        //DUMMY CODE: Print the raw_char_buf
        //NOTE: You should not blindly print messages!
        //      Ask yourself: Is this message really for me?
        //                    Is this message corrupted?
        //                    Is this an old, retransmitted message?
        char *raw_char_buf = (char *)ll_inmsg_node->value;
        Frame *inframe = convert_char_to_frame(raw_char_buf);
        //Free raw_char_buf
        free(raw_char_buf);
        uint16_t crc = CRC16_XMODEM((uint8_t*)inframe, MAX_FRAME_SIZE);
        // uint8_t crc = crc8((uint8_t*)inframe, MAX_FRAME_SIZE);
        if (crc)
        {   
        #ifdef _DEBUG_
            printf("Data Receive Corrupted: %d\n", inframe->seqNum);
        #endif
            free(ll_inmsg_node);
        }
        else
        {
            // 如果帧是发给当前接收方，处理帧
            if (receiver->recv_id == inframe->dst_id)
            {
                unsigned char seqNo = inframe->seqNum;
            // #ifdef _DEBUG_
            //     printf("%d - %d\n", seqNo,  receiver->NFE);
            // #endif
                // 如果接受到期待的新帧，接收帧
                if(seqNo == receiver->NFE)
                {
                #ifdef _DEBUG_
                    printf("receive data: %d\n", seqNo);
                #endif
                    // 如果帧没有被接受过或当前位置存放了后续帧，替换。
                    if(receiver->RecvQ[seqNo % SWS].is_received == 0 || 
                        (receiver->RecvQ[seqNo % SWS].is_received == 1 && seqNo < receiver->RecvQ[seqNo % SWS].frame->seqNum) )
                    {
                        char* result = (char *) receiver->data_result;

                        receiver->RecvQ[seqNo % SWS].is_received = 1;
                        receiver->RecvQ[seqNo % SWS].frame = inframe;
                        // 遍历滑动窗口，找到下一个想要获取的帧序号
                        int i;
                        for(i = receiver->NFE; i < receiver->NFE + SWS; i++)
                        {
                            if(receiver->RecvQ[i % SWS].is_received == 1 && receiver->RecvQ[i % SWS].frame->seqNum == i)
                            {
                                receiver->RecvQ[i % SWS].is_received = 0;  // 空出窗口准备接受帧
                                receiver->NFE++;
                                // 数据拼接
                                if(receiver->RecvQ[i % SWS].frame->is_end == 0)
                                {
                                #ifdef _MSG_DEBUG_
                                    printf("receive data: %s\n", inframe->data);
                                #endif
                                    strcat(result,receiver->RecvQ[i % SWS].frame->data);
                                }
                                // 接收的完整的帧，输出并滑动接收方窗口
                                else
                                {
                                    strcat(result,receiver->RecvQ[i % SWS].frame->data);
                                    printf("<RECV%d>:[%s]\n", receiver->recv_id, result);
                                    for(int i = 0; i < MAX_BUFFER_SIZE; i ++)
                                        receiver->data_result[i] = NULL;
                                    // result = NULL;
                                }
                            }
                            else
                            {
                                break;
                            }
                        }
                    #ifdef _DEBUG_
                        printf("send ack: %d\n", i);
                    #endif
                        // 发送ack确认
                        Frame *outgoing_frame = (Frame *)malloc(sizeof(Frame));
                        outgoing_frame->dst_id = inframe->src_id;
                        outgoing_frame->ackNum = i;
                        outgoing_frame->src_id = receiver->recv_id;
                        uint16_t crc = CRC16_XMODEM((uint8_t*)outgoing_frame, MAX_FRAME_SIZE - 2);
                        outgoing_frame->Hcheck = crc >> 8;
                        outgoing_frame->Lcheck = crc & 0x00FF;
                        // uint8_t crc = crc8((uint8_t*)outgoing_frame, MAX_FRAME_SIZE - 1);
                        // outgoing_frame->CRC = crc;
                        char *outgoing_charbuf = convert_frame_to_char(outgoing_frame);
                        // append_crc(outgoing_charbuf, MAX_FRAME_SIZE);
                        ll_append_node(outgoing_frames_head_ptr, outgoing_charbuf);
                        free(outgoing_frame);
                    }
                    else
                    {
                    #ifdef _DEBUG_
                        printf("当前数据未处理完！");
                    #endif
                    }
                }
                // 如果帧在滑动窗口接收范围内
                else if(seqNo > receiver->NFE && seqNo < receiver->NFE + SWS)
                {
                #ifdef _DEBUG_
                    printf("等待前置数据\n");
                #endif
                    receiver->RecvQ[seqNo % SWS].is_received = 1;
                    receiver->RecvQ[seqNo % SWS].frame = inframe;
                    continue;
                }
                // 如果sender没有收到ack确认重新发帧
                else if(seqNo < receiver->NFE)
                {
                #ifdef _DEBUG_
                    printf("resend ackNum: %d\n", receiver->NFE);
                #endif
                    Frame *outgoing_frame = (Frame *)malloc(sizeof(Frame));
                    outgoing_frame->dst_id = inframe->src_id;
                    outgoing_frame->ackNum = receiver->NFE;
                    outgoing_frame->src_id = receiver->recv_id;
                    uint16_t crc = CRC16_XMODEM((uint8_t*)outgoing_frame, MAX_FRAME_SIZE - 2);
                    outgoing_frame->Hcheck = crc >> 8;
                    outgoing_frame->Lcheck = crc & 0x00FF;
                    // uint8_t crc = crc8((uint8_t*)outgoing_frame, MAX_FRAME_SIZE - 1);
                    // outgoing_frame->CRC = crc;
                    char *outgoing_charbuf = convert_frame_to_char(outgoing_frame);
                    // append_crc(outgoing_charbuf, MAX_FRAME_SIZE);
                    ll_append_node(outgoing_frames_head_ptr, outgoing_charbuf);
                    free(outgoing_frame);
                    continue;
                }
                else
                {
                #ifdef _DEBUG_
                    printf("%d, 发送过快\n", seqNo);
                #endif
                    continue;
                }
            }
            free(inframe);
            free(ll_inmsg_node);
        }
    }
}

void *run_receiver(void *input_receiver)
{
    struct timespec time_spec;
    struct timeval curr_timeval;
    const int WAIT_SEC_TIME = 0;
    const long WAIT_USEC_TIME = 100000;
    Receiver *receiver = (Receiver *)input_receiver;
    LLnode *outgoing_frames_head;

    //This incomplete receiver thread, at a high level, loops as follows:
    //1. Determine the next time the thread should wake up if there is nothing in the incoming queue(s)
    //2. Grab the mutex protecting the input_msg queue
    //3. Dequeues messages from the input_msg queue and prints them
    //4. Releases the lock
    //5. Sends out any outgoing messages

    pthread_cond_init(&receiver->buffer_cv, NULL);
    pthread_mutex_init(&receiver->buffer_mutex, NULL);

    while (1)
    {
        //NOTE: Add outgoing messages to the outgoing_frames_head pointer
        outgoing_frames_head = NULL;
        gettimeofday(&curr_timeval,
                     NULL);

        //Either timeout or get woken up because you've received a datagram
        //NOTE: You don't really need to do anything here, but it might be useful for debugging purposes to have the receivers periodically wakeup and print info
        time_spec.tv_sec = curr_timeval.tv_sec;
        time_spec.tv_nsec = curr_timeval.tv_usec * 1000;
        time_spec.tv_sec += WAIT_SEC_TIME;
        time_spec.tv_nsec += WAIT_USEC_TIME * 1000;
        if (time_spec.tv_nsec >= 1000000000)
        {
            time_spec.tv_sec++;
            time_spec.tv_nsec -= 1000000000;
        }

        //*****************************************************************************************
        //NOTE: Anything that involves dequeing from the input frames should go
        //      between the mutex lock and unlock, because other threads CAN/WILL access these structures
        //*****************************************************************************************
        pthread_mutex_lock(&receiver->buffer_mutex);

        //Check whether anything arrived
        int incoming_msgs_length = ll_get_length(receiver->input_framelist_head);
        if (incoming_msgs_length == 0)
        {
            //Nothing has arrived, do a timed wait on the condition variable (which releases the mutex). Again, you don't really need to do the timed wait.
            //A signal on the condition variable will wake up the thread and reacquire the lock
            pthread_cond_timedwait(&receiver->buffer_cv,
                                   &receiver->buffer_mutex,
                                   &time_spec);
        }

        handle_incoming_msgs(receiver,
                             &outgoing_frames_head);

        pthread_mutex_unlock(&receiver->buffer_mutex);

        //CHANGE THIS AT YOUR OWN RISK!
        //Send out all the frames user has appended to the outgoing_frames list
        int ll_outgoing_frame_length = ll_get_length(outgoing_frames_head);
        while (ll_outgoing_frame_length > 0)
        {
            LLnode *ll_outframe_node = ll_pop_node(&outgoing_frames_head);
            char *char_buf = (char *)ll_outframe_node->value;

            //The following function frees the memory for the char_buf object
            send_msg_to_senders(char_buf);

            //Free up the ll_outframe_node
            free(ll_outframe_node);

            ll_outgoing_frame_length = ll_get_length(outgoing_frames_head);
        }
    }
    pthread_exit(NULL);
}
