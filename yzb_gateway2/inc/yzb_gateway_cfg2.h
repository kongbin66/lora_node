#ifndef __YZB_GATEWAY_CFG_H
#define __YZB_GATEWAY_CFG_H
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "radio.h"
/**************************缓存区SSBUF**********************************/
#define BUFNUMB 4096 //缓存区大小
#define M_SIZE 290   //标记组长度
// 2022-10-28 16:27:44，为网关设备开辟数据接收缓冲区，大小和标记组在文件中指定，使用IN_save进行数据的记录，不限单条等长
//使用EN_read进行读取，判定IN！=OUT,连续读取。
typedef struct
{
  uint8_t *STA;          // 4字节
  uint8_t *END;          // 4字节
} PM;                    // 8字节
uint8_t menbuf[BUFNUMB]; //数据缓冲区

PM M[M_SIZE], M2; // 80字节
PM *IN, *OUT;
// extern PM *EN; // M最后位置
extern PM *MS; // M开始位置
extern uint16_t mes_cnt;
void gate_save_init();
//将缓冲区BUF的数据缓存在缓存区。
void IN_save(uint8_t *buf, int size);
bool EN_read(uint8_t *buf, uint16_t *len, bool flag);
/**************************uart0 header**********************************************/
#define USART_REC_LEN 20
int USART_RX_STA;
uint8_t USART_RX_BUF[USART_REC_LEN];
uint8_t USART_TX_BUF[USART_REC_LEN];
void uart0_log_init(void);
void send_byte(uint8_t dat);
void send_group(uint8_t *buff, uint8_t len);
void uart0_recevice();
/***************************PINGPONG.H**********************************************************/
int gateway_app_main();
#endif /* __LORA_CONFIG_H */
