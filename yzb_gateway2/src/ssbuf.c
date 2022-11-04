
#include "yzb_gateway_cfg2.h"
PM *EN = &M[M_SIZE - 1];                    // M[0]~M[9],没有M[10],下标减一;
PM M2 = {&menbuf[0], &menbuf[BUFNUMB - 1]}; //缓冲区首地址,//缓冲区尾地址
uint16_t mes_cnt = 0;                       //信息条数
void gate_save_init()
{
  mes_cnt = 0;
  IN = &M[0];
  OUT = &M[0];
  IN->STA = &menbuf[0];
  OUT->STA = &menbuf[0];
}
//将缓冲区BUF的数据缓存在缓存区。
void IN_save(uint8_t *buf, int size)
{
  uint8_t *p = IN->STA;
  if ((EN - IN) > 0) // M有空间
  {
    printf("MMMMMM have a space!EN=%p,IN=%p,E-I=%d\r\n", EN, IN, EN - IN);
    if ((M2.END - p) >= size) // BUF数组有空间
    {
      printf("BUF have a space!PL=%p,p=%p,PL-p=%d\r\n", M2.END, p, (M2.END) - p);
      for (int i = 0; i < size; i++)
      {
        *p = *buf;
        // printf("&p=%p,*p=%d,buf=%d\r\n", p, *p, *buf);
        p++;
        buf++;
      }
      IN->END = p;
      IN++;
      IN->STA = p;
    }
    else
    {
      printf("BUF not space!PL=%p,p=%p,PL-p=%d\r\n", M2.END, p, M2.END - p);
      for (int i = 0; i < size; i++)
      {
        *p = *buf;
        p++;
        buf++;
      }
      IN->END = p;
      IN++;
      IN->STA = M2.STA; //从头覆盖缓冲区
    }
  }
  else if (EN - IN == 0)
  {
    printf("MMMMMM not space!EN=%p,IN=%p,E-I=%d\r\n", EN, IN, EN - IN);
    if (M2.END - p >= size) //数据有空间
    {

      printf("BUF have a space!PL=%p,p=%p,PL-p=%d\r\n", M2.END, p, M2.END - p);
      for (int i = 0; i < size; i++)
      {
        *p = *buf;
        p++;
        buf++;
      }
      IN->END = p;
      IN = M;
      IN->STA = p;
    }
    else
    {
      printf("BUF not space!PL=%p,p=%p,PL-p=%d\r\n", M2.END, p, M2.END - p);
      for (int i = 0; i < size; i++)
      {
        *p = *buf;
        p++;
        buf++;
      }
      IN->END = p;
      gate_save_init();
    }
  }
  if (mes_cnt < M_SIZE)
  {
    mes_cnt++;
  }
  else
  {
    mes_cnt = 0;
  }
}
//读取缓冲区数据,结果在形参中。根据in!=out判断是否有数据
bool EN_read(uint8_t *buf, uint16_t *len, bool flag)
{
  uint8_t *p = OUT->STA;
  if (flag) //读下一个
  {
    if (IN != OUT) //有数据
    {
      if (EN - OUT > 0) //有空间
      {
        *len = OUT->END - OUT->STA;
        printf("len=%d\r\n", *len);
        for (int i = 0; i < *len; i++)
        {
          *buf = *p;
          p++;
          buf++;
        }
        OUT++;
      }
      else //没空间
      {
        *len = OUT->END - OUT->STA;
        printf("len=%d\r\n", *len);
        for (int i = 0; i < *len; i++)
        {
          *buf = *p;
          p++;
          buf++;
        }
        OUT = M;
      }
      return true;
    }
    else
    {
      gate_save_init();
      return false;
    }
  }
  else //只读本条，不加地址
  {
    *len = OUT->END - OUT->STA;
    printf("len=%d\r\n", *len);
    if (IN != OUT) //有数据
    {
      for (int i = 0; i < *len; i++)
      {
        *buf = *p;
        p++;
        buf++;
      }
      return true;
    }
    else
    {
      gate_save_init();
      printf("no data!OUT=%p,in=%p\r\n", OUT, IN);
      return false;
    }
  }
}