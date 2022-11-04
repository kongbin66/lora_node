#include "yzb_node_cfg.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "delay.h"
#include "timer.h"
#include "radio.h"
#include "tremo_system.h"
#define NODE_ADD 0X0003  //节点ID
#define GATEWAY_ADD 0X001 //网关ID
#define TIMOUT 1000
uint16_t timerout = 1000; //延时时间1秒
bool flag_recevie_fail = false;

#if defined(REGION_CN470)

#define RF_FREQUENCY 470000000 // Hz

#elif defined(REGION_EU433)

#define RF_FREQUENCY 433000000 // Hz

#endif

#define TX_OUTPUT_POWER 14 // dBm

#define LORA_BANDWIDTH 0        // [0: 125 kHz,
                                //  1: 250 kHz,
                                //  2: 500 kHz,
                                //  3: Reserved]
#define LORA_SPREADING_FACTOR 7 // [SF7..SF12]
#define LORA_CODINGRATE 1       // [1: 4/5,
                                //  2: 4/6,
                                //  3: 4/7,
                                //  4: 4/8]
#define LORA_PREAMBLE_LENGTH 8  // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT 0   // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false

typedef enum
{
    LOWPOWER,
    RX,
    RX_TIMEOUT,
    RX_ERROR,
    TX,
    TX_TIMEOUT
} States_t;

#define RX_TIMEOUT_VALUE 1800 //接收延迟时间

const uint8_t PingMsg[] = "PING";
const uint8_t PongMsg[] = "PONG";

#define BUFFER_SIZE 14 // Define the payload size here
uint16_t BufferSize = BUFFER_SIZE;
uint8_t Buffer[BUFFER_SIZE];

uint8_t sendbuf[14]; //发送缓冲区

volatile States_t State = LOWPOWER;

int8_t RssiValue = 0;
int8_t SnrValue = 0;

uint32_t ChipId[2] = {0};
/*!
 * Radio events function pointer
 */
static RadioEvents_t RadioEvents;

/*!
 * \brief Function to be executed on Radio Tx Done event
 */
void OnTxDone(void)
{
    Radio.Sleep();
    State = TX;
}
/*!
 * \brief Function to be executed on Radio Rx Done event
 */
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
    Radio.Sleep();
    BufferSize = size;
    memcpy(Buffer, payload, BufferSize);
    RssiValue = rssi;
    SnrValue = snr;
    State = RX;
}
/*!
 * \brief Function executed on Radio Tx Timeout event
 */
void OnTxTimeout(void)
{
    Radio.Sleep();
    State = TX_TIMEOUT;
}
/*!
 * \brief Function executed on Radio Rx Timeout event
 */
void OnRxTimeout(void)
{
    printf("OnRxTimeout\r\n");
    if (flag_recevie_fail == false)
    {
        Radio.Rx(RX_TIMEOUT_VALUE);
    }

    Radio.Sleep();
    State = RX_TIMEOUT;
}
/*!
 * \brief Function executed on Radio Rx Error event
 */
void OnRxError(void)
{
    Radio.Sleep();
    State = RX_ERROR;
}

//节点参数
typedef struct NODE_parame
{
    uint32_t node_id;  //节点ID
    uint32_t node_frq; //节点频率
    uint8_t node_ADL;
    uint8_t node_ADH; //节点地址
} yz_typ;
yz_typ node, gateway;

struct NODE_GET_PARAM
{
    float temper;   //温度
    float humi;     //湿度
    uint8_t charge; //电池电量

} node_get;

void node_init()
{
    printf("node init!\r\n");
    // Radio initialization
    RadioEvents.TxDone = OnTxDone;
    RadioEvents.RxDone = OnRxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    RadioEvents.RxTimeout = OnRxTimeout;
    RadioEvents.RxError = OnRxError;
    Radio.Init(&RadioEvents);

    Radio.SetChannel(RF_FREQUENCY);

    Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                      LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                      LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                      true, 0, 0, LORA_IQ_INVERSION_ON, 3000);

    Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                      LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                      LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                      0, true, 0, 0, LORA_IQ_INVERSION_ON, true);
}
void node_get_data()
{
    uint16_t tem = 100;
    uint16_t humity = 100;
    //采集温湿度
    node_get.temper = read_sht20_temp(0xe3);
    // printf("valuee3=%.2f\r\n", *temp / 10);
    node_get.humi = read_sht20_temp(0xe5);
    // printf("valuee5=%.2f\r\n", *humi / 10);
    node_get.charge = 20;

    tem = (int)(node_get.temper * 100);
    humity = (int)(node_get.humi * 100);
    sendbuf[0] = 0xAA;
    sendbuf[1] = GATEWAY_ADD / 0xff; //网关ID
    sendbuf[2] = GATEWAY_ADD;
    sendbuf[3] = 10;              //数据长度
    sendbuf[4] = 1;               //功能码
    sendbuf[5] = NODE_ADD / 0xff; //节点ID
    sendbuf[6] = NODE_ADD;
    sendbuf[7] = tem >> 8;
    sendbuf[8] = tem;
    sendbuf[9] = humity >> 8;
    sendbuf[10] = humity;
    sendbuf[11] = node_get.charge >> 8;
    sendbuf[12] = node_get.charge;
    sendbuf[13] = 0xBB;
}

bool node_send(uint8_t *buf, int len)
{

    flag_recevie_fail = false;

    Radio.Send(buf, len);
    //等待回复
    State = LOWPOWER;
    while (State != TX)
    {
        Radio.IrqProcess();
    }
    printf("send over!\r\n");
    State = LOWPOWER;
    Radio.Rx(RX_TIMEOUT_VALUE);
    uint16_t Timerout = TIMOUT;
    while ((State != RX) && (Timerout))
    {
        Timerout--;
        DelayMs(1);
        Radio.IrqProcess();
    }
    if (State == RX)
    {
        if ((Buffer[0] == 0XAA) && (Buffer[13] == 0xBB))
        {
            if (Buffer[5] * 0XFF + Buffer[6] == NODE_ADD)
            {
                printf("received ok!!\r\n");
                State = LOWPOWER;
                return 1;
            }
        }
    }
    else
    {
        printf("eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee,state=%d\r\n", State);
        //保存数据，下次发送。
        flag_recevie_fail = true;
        State = LOWPOWER;
    }
    return 0;
}

// int my_app_main()
// {
//     int len, i;
//     bool flag_overflow = 0;
//     uint8_t buftemp[14];
//     my_i2c_init();
//     node_init();
//     init_parame();

//     /*
//     1.节点周期向网关发送数据，发送完成睡眠。网关频率433M 地址 0x0001
//     */
//     while (1)
//     {
//         //采集数据
//         node_get_data();
//         if (flag_overflow = 1)
//         {
//             flag_overflow = 0;
//             while (1)
//             {
//                 i = EN_read(buftemp, &len);
//                 if (i)
//                 {
//                     //补发上次的
//                     node_send(buftemp, len);
//                     DelayMs(10);
//                 }
//                 else
//                 {
//                     break;
//                 }
//             }
//             DelayMs(10);
//             node_send(buftemp, len); //发送本次的
//         }
//         else
//         {
//             IN_save(buftemp, i);
//             flag_overflow = 1;
//         }

//         DelayMs(30000);
//     }
// }

void mm_main()
{
    //唤醒
    int len;
    //    bool flag_overflow=0;
    my_i2c_init();
    node_init();
    gate_save_init();
    while (1)
    {
        if (EN_read(sendbuf, &len, 0)) //有漏发
        {
            for (int i = 0; i < len; i++)
            {
                printf("sendbuf[%d]=%d\r\n", i, sendbuf[i]);
            }
            printf("len=%d,you loufa!!!1111\r\n", len);
            DelayMs(10);
            if (node_send(sendbuf, len)) //发送成功
            {
                printf("loufa send ok222222222\r\n");
                while (EN_read(sendbuf, &len, 1)) //读下一条
                {
                    DelayMs(10);
                    if (node_send(sendbuf, len)) //发送成功
                    {
                        //                        flag_overflow = 0;
                        printf("loufa send ok33333333\r\n");
                    }
                    else //发送失败
                    {
                        printf("loufa send fail!! save data444444444\r\n");
                        node_get_data();
                        IN_save(sendbuf, len);
                        //  flag_overflow = 1;
                    }
                }
            }
            else //发送失败
            {
                printf("loufa send fail!! save data55555\r\n");
                node_get_data();
                for (int i = 0; i < 14; i++)
                {
                    printf("send[%d]=%d\r\n", i, sendbuf[i]);
                }
                IN_save(sendbuf, BUFFER_SIZE);
                // flag_overflow = 1;
            }
        }
        else //正常发送
        {
            printf("zcfsong ok!66666666666");
            node_get_data();
            printf("aaaaaaa\r\n");
            if (node_send(sendbuf, BUFFER_SIZE)) //发送成功
            {
                printf("fasong wanceng ok77777777777!");
                //  flag_overflow = 0;
            }
            else //发送失败
            {
                for (int i = 0; i < 14; i++)
                {
                    printf("send[%d]=%d\r\n", i, sendbuf[i]);
                }
                IN_save(sendbuf, 14);
                //  flag_overflow = 1;
                printf("fasong shibai fail 88888888888!");
            }
        }
        DelayMs(10000);
    }
}

/**
 * Main application entry point.
 */
// int app_start(void)
// {
//     bool isMaster = true;
//     uint8_t i;
//     uint32_t random;

//     printf("PingPong test Start!\r\n");

//     (void)system_get_chip_id(ChipId);

//     // Radio initialization
//     RadioEvents.TxDone = OnTxDone;
//     RadioEvents.RxDone = OnRxDone;
//     RadioEvents.TxTimeout = OnTxTimeout;
//     RadioEvents.RxTimeout = OnRxTimeout;
//     RadioEvents.RxError = OnRxError;

//     Radio.Init(&RadioEvents);

//     Radio.SetChannel(RF_FREQUENCY);

//     Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
//                       LORA_SPREADING_FACTOR, LORA_CODINGRATE,
//                       LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
//                       true, 0, 0, LORA_IQ_INVERSION_ON, 3000);

//     Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
//                       LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
//                       LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
//                       0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

//     Radio.Rx(RX_TIMEOUT_VALUE);

//     while (1)
//     {
//         switch (State)
//         {
//         case RX:
//             if (isMaster == true)
//             {
//                 if (BufferSize > 0)
//                 {
//                     if (strncmp((const char *)Buffer, (const char *)PongMsg, 4) == 0)
//                     {
//                         printf("Received: PONG\r\n");

//                         // Send the next PING frame
//                         Buffer[0] = 'P';
//                         Buffer[1] = 'I';
//                         Buffer[2] = 'N';
//                         Buffer[3] = 'G';
//                         // We fill the buffer with numbers for the payload
//                         for (i = 4; i < BufferSize; i++)
//                         {
//                             Buffer[i] = i - 4;
//                         }
//                         DelayMs(10);
//                         printf("Sent: PING\r\n");
//                         Radio.Send(Buffer, BufferSize);
//                     }
//                     else if (strncmp((const char *)Buffer, (const char *)PingMsg, 4) == 0)
//                     { // A master already exists then become a slave
//                         isMaster = false;
//                         Radio.Rx(RX_TIMEOUT_VALUE);
//                     }
//                     else // valid reception but neither a PING or a PONG message
//                     {    // Set device as master ans start again
//                         isMaster = true;
//                         Radio.Rx(RX_TIMEOUT_VALUE);
//                     }
//                 }
//             }
//             else
//             {
//                 if (BufferSize > 0)
//                 {
//                     if (strncmp((const char *)Buffer, (const char *)PingMsg, 4) == 0)
//                     {
//                         printf("Received: PING\r\n");

//                         // Send the reply to the PONG string
//                         Buffer[0] = 'P';
//                         Buffer[1] = 'O';
//                         Buffer[2] = 'N';
//                         Buffer[3] = 'G';
//                         // We fill the buffer with numbers for the payload
//                         for (i = 4; i < BufferSize; i++)
//                         {
//                             Buffer[i] = i - 4;
//                         }
//                         DelayMs(10);
//                         Radio.Send(Buffer, BufferSize);
//                         printf("Sent: PONG\r\n");
//                     }
//                     else // valid reception but not a PING as expected
//                     {    // Set device as master and start again
//                         isMaster = true;
//                         Radio.Rx(RX_TIMEOUT_VALUE);
//                     }
//                 }
//             }
//             State = LOWPOWER;
//             break;
//         case TX:
//             Radio.Rx(RX_TIMEOUT_VALUE);
//             State = LOWPOWER;
//             break;
//         case RX_TIMEOUT:
//         case RX_ERROR:
//             if (isMaster == true)
//             {
//                 // Send the next PING frame
//                 Buffer[0] = 'P';
//                 Buffer[1] = 'I';
//                 Buffer[2] = 'N';
//                 Buffer[3] = 'G';
//                 for (i = 4; i < BufferSize; i++)
//                 {
//                     Buffer[i] = i - 4;
//                 }
//                 srand(*ChipId);
//                 random = (rand() + 1) % 90;
//                 DelayMs(random);
//                 Radio.Send(Buffer, BufferSize);
//                 printf("Sent: PING\r\n");
//             }
//             else
//             {
//                 Radio.Rx(RX_TIMEOUT_VALUE);
//             }
//             State = LOWPOWER;
//             break;
//         case TX_TIMEOUT:
//             Radio.Rx(RX_TIMEOUT_VALUE);
//             State = LOWPOWER;
//             break;
//         case LOWPOWER:
//         default:
//             // Set low power
//             break;
//         }

//         // Process Radio IRQ
//         Radio.IrqProcess();
//     }
// }
