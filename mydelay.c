#include <MYDELAY.H>
void delay1ms()
{
   unsigned char i, j;
	_nop_();
	_nop_();
	_nop_();
	i = 11;
	j = 190;
	do
	{
		while (--j);
	} while (--i);	 
 }
 /*****************************************************
函数功能：延时若干毫秒
入口参数：n
***************************************************/
 
 void delaynms(unsigned int ms)
{
   unsigned int i;
	for(i=0;i<ms;i++)
	   delay1ms();
 }


