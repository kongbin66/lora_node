#include "yzb_gateway_cfg2.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "delay.h"
#include "timer.h"

#include "tremo_system.h"
#include "tremo_uart.h"
#define GATEWAY_ADD 0X0001

typedef struct
{
    uint16_t nodeid; //节点ID
    float temper;    //温度
    float humidity;  //湿度
    uint8_t charge;  //电池电量
    uint8_t signal;  //信号强度
    uint32_t stamp;  //时间戳
} nodedat;
nodedat node;

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

#define BUFFER_SIZE 14 // Define the payload size here
uint16_t BufferSize = BUFFER_SIZE;
uint8_t Buffer[BUFFER_SIZE];

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
    // printf("OnRxTimeout\r\n");
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

void gateway_init()
{
    printf("gateway init!\r\n");
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

int gateway_app_main()
{
    // uint8_t aa;

    gateway_init();
    gate_save_init();
    NVIC_EnableIRQ(UART1_IRQn);
    NVIC_SetPriority(UART1_IRQn, 2);
    uart_config_interrupt(UART1, UART_INTERRUPT_RX_DONE, 1);
    Radio.Rx(RX_TIMEOUT_VALUE);
    /*
    1.节点周期向网关发送数据，发送完成睡眠。网关频率433M 地址 0x0001
    */
    USART_RX_STA = 0;
    while (1)
    {
        switch (State)
        {
        case RX:
            if (BufferSize > 0)
            {
                // printf("[%s()-%d]Rx done,rssi:%d,len:%d,data:%s\r\n", __func__, __LINE__, RssiValue, BufferSize, Buffer);
                if ((Buffer[0] == 0XAA) && (Buffer[13] == 0xBB)) //收到数据，解析包
                {
                    if (Buffer[1] * 0XFF + Buffer[2] == GATEWAY_ADD) //是发给网关的
                    {
                        // int datsiz = Buffer[3];                     //字节
                        //  printf("datsiz=%d\r\n", datsiz);
                        // int fun = Buffer[4];                        //功能码
                        // printf("fun=%d\r\n", fun);
                        node.nodeid = Buffer[5] * 0xff + Buffer[6]; //节点ID
                        printf("nodeid=%d\r\n", node.nodeid);
                        node.temper = Buffer[7] * 0xff + Buffer[8];    //温度值
                                                                       // printf("temper=%.2f\r\n", (float)node.temper / 1000);
                        node.humidity = Buffer[9] * 0xff + Buffer[10]; //湿度值
                                                                       // printf("humi=%d\r\n", (int)node.humidity / 1000);
                        node.charge = Buffer[11] * 0xff + Buffer[12];  //电池电量
                        // printf("charge=%d\r\n", node.charge);
                        //回复
                        DelayMs(10);
                        // printf("Sent: message\r\n");
                        Radio.Send(Buffer, BufferSize);
                        //保存
                        IN_save(Buffer, BufferSize);
                        printf("mes_cnt=%d\r\n", mes_cnt);
                        // uint8_t bbuf[14];
                        //读取数据
                        //  int len;
                        //  bool e;
                        //  e = EN_read(bbuf, &len, 1);
                        //  if (e)
                        //  {
                        //      for (int i = 0; i < len; i++)
                        //      {
                        //          printf("bbuf[%d]=%d\r\n", i, bbuf[i]);
                        //      }
                        //  }
                        //  else
                        //  {
                        //      printf("error!!\r\n");
                        //  }
                    }
                }
            }
            State = LOWPOWER;
            break;
        case TX:
            Radio.Rx(RX_TIMEOUT_VALUE);
            State = LOWPOWER;
            break;
        case RX_TIMEOUT:
        case RX_ERROR:
            Radio.Rx(RX_TIMEOUT_VALUE);
            State = LOWPOWER;
            break;
        case TX_TIMEOUT:
            Radio.Rx(RX_TIMEOUT_VALUE);
            State = LOWPOWER;
            break;
        case LOWPOWER:
        default:
            // Set low power
            break;
        }

        uart0_recevice();
        // Process Radio IRQ
        Radio.IrqProcess();
    }
}
