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
�������ܣ���ʱ���ɺ���
��ڲ�����n
***************************************************/
 
 void delaynms(unsigned int ms)
{
   unsigned int i;
	for(i=0;i<ms;i++)
	   delay1ms();
 }


