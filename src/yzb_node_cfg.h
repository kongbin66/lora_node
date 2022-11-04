#ifndef __NODE_CFG_H
#define __NODE_CFG_H
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "tremo_rcc.h"
#include "tremo_gpio.h"
#include "tremo_i2c.h"
/******************SHT20.H***********************************/
// sht20初始化
void my_i2c_init(void);
// sht20采集温度
float read_sht20_temp(u8 Cmd);
/********************Ssbuf.h****************************************/
#define BUFNUMB 4096 //缓存区大小
#define M_SIZE 100   //标记组长度
void gate_save_init();
//将缓冲区BUF的数据缓存在缓存区。
void IN_save(uint8_t *buf, int size);
bool EN_read(uint8_t *buf, int *len, bool flag);

#endif /* __TREMO_IT_H */
