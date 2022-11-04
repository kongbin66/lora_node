#include "yzb_gateway_cfg2.h"
#include "tremo_delay.h"
#include "tremo_uart.h"
#include "tremo_rcc.h"
#include "tremo_gpio.h"
#include <stdio.h>

//串口0初始化
void uart0_log_init(void)
{
  uart_config_t uart_config;

  rcc_enable_peripheral_clk(RCC_PERIPHERAL_UART0, true);
  rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOB, true);

  // UART0_TX--PB1  UART0_RX--PB0
  gpio_set_iomux(GPIOB, GPIO_PIN_0, 1); //复用功能1
  gpio_set_iomux(GPIOB, GPIO_PIN_1, 1); //复用功能1

  uart_config_init(&uart_config); //初始化结构体

  uart_config.baudrate = UART_BAUDRATE_115200;
  uart_init(UART0, &uart_config);

  uart_config_interrupt(UART0, UART_INTERRUPT_RX_DONE, true); //打开接收中断
  uart_cmd(UART0, ENABLE);

  //中断配置
  NVIC_EnableIRQ(UART0_IRQn);
  NVIC_SetPriority(UART0_IRQn, 1);
}

//发送字节
void send_byte(u8 dat)
{
  uart_send_data(UART0, dat);
  while (uart_get_flag_status(UART0, UART_FLAG_TX_FIFO_EMPTY) == 0)
    ;
}

//发送字符串
void send_group(u8 *buff, u8 len)
{
  while (len--)
  {
    send_byte(*buff++);
  }
}
//扫描接收
void uart0_recevice()
{
  uint16_t len = USART_RX_STA & 0x3fff;

  if ((USART_RX_STA & 0x8000))
  {
    if ((USART_RX_BUF[0] == 0xAA) && (USART_RX_BUF[1] == 0x01) && (USART_RX_BUF[2] == 0x01) && (USART_RX_BUF[3] == 0xBB)) //读取单条
    {
      if (EN_read(USART_TX_BUF, &len, true))
      {
        for (int i = 0; i < len; i++)
        {
          send_byte(USART_TX_BUF[i]);
        }
      }
      else
      {
        USART_TX_BUF[0] = 0Xaa;
        USART_TX_BUF[1] = 0X01;
        USART_TX_BUF[2] = 0Xee;
        USART_TX_BUF[3] = 0Xbb;
        for (int i = 0; i < 4; i++)
        {
          send_byte(USART_TX_BUF[i]);
        }
      }
    }
    else if ((USART_RX_BUF[0] == 0xAA) && (USART_RX_BUF[1] == 0x02) && (USART_RX_BUF[2] == 0x01) && (USART_RX_BUF[3] == 0xBB)) //读取多条
    {
      if (mes_cnt)
      {
        len = ((IN - 1)->END - M2.STA);
        printf("IN_END=%p,M2.STA=%p,len=%d,count=%d\r\n", (IN - 1)->END, M2.STA, len, len / 14);
        for (int i = 0; i < len; i++)
        {
          send_byte(menbuf[i]);
        }
        gate_save_init();
      }
      else
      {
        printf("not data!!\r\n");
      }
    }

    USART_RX_STA = 0;
  }
}

//发送中断服务函数
void UART0_IRQHandler(void)
{
  char Res;
  // printf("A");
  if (uart_get_interrupt_status(UART0, UART_INTERRUPT_RX_DONE) != RESET)
  {
    uart_clear_interrupt(UART0, UART_INTERRUPT_RX_DONE);

    Res = uart_receive_data(UART0);
    USART_RX_BUF[USART_RX_STA++] = Res;
    if (Res == 0xBB || Res == 0x0A)
    {
      USART_RX_STA |= 0X8000;
    }
  }
}

// int main(void)
// {
//   uint8_t buf1[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
//   uart_log_init();
//   while (1)
//   {
//     // send_byte(0xaa);
//     // send_group(buf1, 9);
//     delay_ms(1000);
//     if (cnt > 0)
//     {
//       for (int i = 0; i < cnt; i++)
//       {
//         send_byte(rxbuff[i]);
//       }
//       cnt = 0;
//     }
//   }
// }