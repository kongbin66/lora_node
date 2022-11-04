#include "yzb_node_cfg.h"
#define CRC_MODEL 0x131
#define SHT20_Measurement_Temp 0xe3
#define SHT20_Measurement_Humi 0xe5
#define slave_addr 0x40
i2c_config_t config;

void my_i2c_init(void)
{
    i2c_deinit(I2C0);
    // enable the clk
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_I2C0, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOA, true);

    // set iomux
    gpio_set_iomux(GPIOA, GPIO_PIN_14, 3);
    gpio_set_iomux(GPIOA, GPIO_PIN_15, 3);

    // init

    i2c_config_init(&config);
    i2c_init(I2C0, &config);
    i2c_cmd(I2C0, true);
}

u8 CRC_Check(u8 *ptr, u8 len, u8 checksum)
{
    u8 i;
    u8 crc = 0x00; //计算的初始crc值

    while (len--)
    {
        crc ^= *ptr++; //每次先与需要计算的数据异或,计算完指向下一数据

        for (i = 8; i > 0; --i) //下面这段计算过程与计算一个字节crc一样
        {
            if (crc & 0x80)
            {
                crc = (crc << 1) ^ CRC_MODEL;
            }
            else
                crc = (crc << 1);
        }
    }
    if (checksum == crc)
    {
        return 0;
    }
    else
        return 1;
}

void i2c_clear_wait()
{
    i2c_clear_flag_status(I2C0, I2C_FLAG_TRANS_EMPTY);
    while (i2c_get_flag_status(I2C0, I2C_FLAG_TRANS_EMPTY) != SET)
        ;
}

float read_sht20_temp(u8 Cmd)
{

    float data_th;
    u8 buf[3];
    u8 checksum;
    // start
    i2c_master_send_start(I2C0, slave_addr, I2C_WRITE);
    i2c_clear_wait();

    // write data
    i2c_send_data(I2C0, Cmd);
    i2c_clear_wait();

    // restart
    i2c_master_send_start(I2C0, slave_addr, I2C_READ);
    i2c_clear_wait();

    // read data

    i2c_set_receive_mode(I2C0, I2C_ACK);
    while (i2c_get_flag_status(I2C0, I2C_FLAG_RECV_FULL) != SET)
        ;
    i2c_clear_flag_status(I2C0, I2C_FLAG_RECV_FULL);
    buf[0] = i2c_receive_data(I2C0);

    i2c_set_receive_mode(I2C0, I2C_ACK);
    while (i2c_get_flag_status(I2C0, I2C_FLAG_RECV_FULL) != SET)
        ;
    i2c_clear_flag_status(I2C0, I2C_FLAG_RECV_FULL);
    buf[1] = i2c_receive_data(I2C0);

    i2c_set_receive_mode(I2C0, I2C_NAK);
    while (i2c_get_flag_status(I2C0, I2C_FLAG_RECV_FULL) != SET)
        ;
    i2c_clear_flag_status(I2C0, I2C_FLAG_RECV_FULL);
    checksum = i2c_receive_data(I2C0);
    // stop
    i2c_master_send_stop(I2C0);

    data_th = (buf[0] << 8) + buf[1];

    if (CRC_Check(&buf[0], 2, checksum) == 0) //校验
    {
        if (Cmd == SHT20_Measurement_Temp)
        {
            data_th = (175.72 * data_th / 65536 - 46.85) * 10; //温度计算公式
        }
        else
            data_th = (125.0 * data_th / 65536 - 6.0) * 10; //湿度计算公式

        return data_th;
    }
    else
        return 0xFFFF; //校验不通过返回0xFFFF
}
