#include <STC12C5A60S2.H>
#include <INTRINS.H>
#include <MYDELAY.H>
#define FOSC 12000000           //时钟频率
#define S2RI 0x01
#define S2TI 0x02
#define S2RB8 0x04
#define S2TB8 0x08
#define COM_TX1_Lenth 4         //接收缓冲大小
typedef unsigned char BYTE;
typedef unsigned int WORD;
sfr IP2H=0xb6;
sbit aux = P0^0;       //P0  0是as62的aux
BYTE signal_level();
void baudchange(long baud);
void Inituart(void);
void init_as62_t30();
void UartInit(void);    //波特率初始化
void delay(BYTE m);
void serial1senddata(BYTE dat);//串口1发字节
void serial2senddata(BYTE dat);//串口二发字节
void serial2sendstdring(char *s);//串口二发串
void send_rssitest_to_as62t30(BYTE dat);   //发送地址信道信号强度数据到as62
void control_yuntai_pelco_d(BYTE address,BYTE direction,BYTE horizontalSpeed,BYTE verticalSpeed,BYTE delay);
void auto_alignment();
BYTE check_stop_alignment();
void auto_alignment_eazysearch();
void Uart1_Init(WORD baud);
void Uart2_Init(WORD baud);
BYTE auto_init_position();
void optimized_auto_alignment();
BYTE move_to_dir_and_delay(BYTE dir,WORD delay);
void screw_auto_alignment();
BYTE respond_control();
unsigned char active[6]= {0x08,0x02,0x10,0x04,0x00,0x11}; //命令查询表上、右、下、左、停、自动
unsigned char rxbuf[COM_TX1_Lenth];      //串口1接收缓冲
BYTE automod=0;  //自动模式标志
//BYTE as62_t30_addr=P1&0x0f;       //拨码开关改变as62_t30地址
//BYTE as62_t30_channel=(P0&0x3e)>>1;
int rxpoint=0;  //接收上限指针
int rxbusy=0;   //接收忙标志
BYTE glc=0;
BYTE a[7];
BYTE time0flag=0;
BYTE sum=0;        //反馈信号延时发送计数
BYTE signal=0;
void main()
{
    aux=1;
    Inituart();		 //串口初始化
    init_as62_t30();	 //433初始
    ES=0;
    //auto_init_position();
    ES=1;
	//P2=0;
    while(1)
    {
        P2|=1;         //使能485
        delaynms(3000);
        serial1senddata(signal_level());

        if(sum++>5)
        {
            sum=0;
        }

        if(automod==1)
        {
            //auto_alignment();
            screw_auto_alignment();
            //auto_alignment_eazysearch();
        }
		respond_control();


    }
}

/***********初始化部分*************/
void Uart1_Init(WORD baud)		//1200bps@12.000MHz  定时器1作为波特率发生器，12T
{
    PCON &= 0x7F;		//波特率不倍速
    SCON = 0x50;		//8位数据,可变波特率
    AUXR &= 0xBF;		//定时器1时钟为Fosc/12,即12T
    AUXR &= 0xFE;		//串口1选择定时器1为波特率发生器
    TMOD &= 0x0F;		//清除定时器1模式位
    TMOD |= 0x20;		//设定定时器1为8位自动重装方式
    TL1=256-FOSC/baud/32/12; //设定定时初值
    TH1 = 256-FOSC/baud/32/12;		//设定定时器重装值
    ET1 = 0;		//禁止定时器1中断
    TR1 = 1;		//启动定时器1
}
void Uart2_Init(WORD baud)		// 串口2初始化初始化 使用独立波特率发生器作为波特率发生器
{
    AUXR &= 0xF7;		//波特率不倍速
    S2CON = 0x50;		//8位数据,可变波特率
    AUXR &= 0xFB;		//独立波特率发生器时钟为Fosc/12,即12T
    BRT = 256-FOSC/baud/32/12;			//设定独立波特率发生器重装值
    AUXR |= 0x10;		//启动独立波特率发生器
}
void Inituart(void)    //初始化操作
{
    Uart1_Init(1200);    //串口1初始化
    Uart2_Init(1200);    //串口2初始化
    AUXR1 |=0x10;        //端口映射到P4
    IPH |= 0x18;//串口一优先级1
    IP |= 0x18;
    IPH2 |= 0x01;//串口二优先级1
    IP2 |= 0x01;
    EA=1;
    ES=1;
}

void init_as62_t30()  //初始化as62-t30
{
    //P0 &= 0x3f;
    P0 &=0x3f;
}

/************串口处理部分*******************/

void UART1_Rountine(void) interrupt 4 using 1
{
    BYTE tmp=0x00;
    BYTE i=0;

    if(RI)
    {
        RI=0;
        tmp=SBUF;

        if(rxbusy==0)
        {
            rxpoint=0;

            for(i=0; i<4; i++) { rxbuf[i]=0; }

            if(tmp==0xa0)	{ rxbusy=1; }
            else { return ; }
        }

        rxbuf[rxpoint++]=tmp;

        if(rxpoint>=COM_TX1_Lenth)
        {
            rxbusy=0;
            rxpoint=0;
        }
    }
}


void serial1senddata(BYTE dat)	   //串口1发送单字节
{
    SBUF=dat;

    while(!TI);

    TI=0;
}
//void serial1sendstring(char *s)		//串口1发送字符串
//{
//    while(*s)
//    {
//        serial1senddata(*s++);
//    }
//}

void serial2senddata(BYTE dat)		 //串口2发送单字节
{
    S2BUF=dat;

    while(!S2CON&S2TI);

    S2CON&=~S2TI;
}
//void serial2sendstring(char *s)		 //串口2发送字符串
//{
//    while(*s)
//    {
//        serial2senddata(*s++);
//    }
//}

/************反馈信号强度部分***************/
void send_rssitest_to_as62t30(BYTE dat)   //发送地址信道信号强度数据到as62
{
    BYTE i;
    BYTE send[4];

    while(!aux);//busy

    send[0]=0xa0;
    send[1]=dat;
    send[2]=0xaf;
    send[3]=send[0]^send[1]^send[2];

    for(i=0; i<4; i++)
    {
        serial1senddata(send[i]);
        delaynms(10);
    }
}

/************************云台控制部分*****************************/
void control_yuntai_pelco_d(BYTE address,BYTE direction,BYTE horizontalSpeed,BYTE verticalSpeed,BYTE delay)
{
    BYTE f=0;
    BYTE i=0;
    f=delay;
    f=0;

    for(i=0; i<8; i++) { a[i]=0; }

    a[0]=0xFF;				//起始码
    a[1]=address;			   //地址码
    a[2]=0x00;			   //数据位1   无动作（无涉摄像头安置）
    a[3]=direction;			   //数据位2   控制云台转动方向
    a[4]=horizontalSpeed;			   //数据位3   控制云台水平转动速度
    a[5]=verticalSpeed;			   //数据位4   控制云台竖直转动速度
    // a[6]=0x0A;			   //停止码

    for(f=1; f<6; f++)	 //生成校验码
    {
        a[6]+=a[f];
    }

    for(i=0; i<7; i++)	 //通过串口2发送数据
    {
        serial2senddata(a[i]);
        delaynms(30);
    }

    //delaynms(delay);		 //云台在此控制方式下转动时间
}

//------------------------------------------天线自动对准部分---------------------------------------------

/*****************************************************
函数功能：检查433是否收到合法的停止自动对准模式信号
*****************************************************/
BYTE check_stop_alignment()
{
    int k=0;

    if(rxbusy==0)                  //接收缓冲如果未被占用，检查数据是否合法，并根据结果进行相关操作
    {
        if(rxbuf[0]==0xa0&&(rxbuf[3]==(rxbuf[0]^rxbuf[1]^rxbuf[2])))                //数据合法性检验
        {
            //serial2senddata(0xbb);
            switch(rxbuf[1])
            {
                case 5:
                    automod=1;
                    break;

                default:
                    automod=0;
                    break;
            }

            for(k=0; k<4; k++)   //清除接收缓存
            {
                rxbuf[k]=0;
            }

            if(automod!=1)
            {
                return 1;
            }
        }
    }

    return 0;
}

/**********************************
函数功能：云台自动对准简单对角线搜索算法
**********************************/
void auto_alignment_eazysearch()
{
    int i=0,j=0,f=0,k=0;
    //        停止0 上1  下2  左3  右4
    BYTE d[5]= {0x00,0x08,0x10,0x04,0x02};
    //-------------------控制云台转动到最左下角-----------------------

    if(move_to_dir_and_delay(d[3]|d[2],30000)==1) { return ; }

    //--------------------简单斜线搜索---------------------------
    for(i=0; i<6; i++)
    {
        if(move_to_dir_and_delay(d[4],2400)==1) { return ; }

        if(move_to_dir_and_delay(d[1],4500)==1) { return ; }
    }
}
/**********************************
函数功能：云台自动对准空间遍历算法
**********************************/
void auto_alignment()
{
    int i=0,j=0,f=0,k=0;
    //        停止0 上1  下2  左3  右4
    BYTE d[5]= {0x00,0x08,0x10,0x04,0x02};

    if(move_to_dir_and_delay(d[3]|d[2],30000)==1) { return ; }

    for(i=0; i<6; i++)
    {
        for(j=0; j<7; j++)
        {
            if(f==0)
            {
                if(move_to_dir_and_delay(d[4],2400)==1) { return ; }
            }
            else
            {
                if(move_to_dir_and_delay(d[3],2400)==1) { return ; }
            }
        }

        f=(f==0)?1:0;

        if(move_to_dir_and_delay(d[1],4500)==1) { return ; }
    }

    for(j=0; j<7; j++)
    {
        if(f==0)
        {
            if(move_to_dir_and_delay(d[4],2400)==1) { return ; }
        }
        else
        {
            if(move_to_dir_and_delay(d[3],2400)==1) { return ; }
        }
    }
}

/***********************************************
函数功能：优化后的自动对准算法
***********************************************/
void optimized_auto_alignment()
{
    int i=0,j=0,f=0,k=0;
	BYTE tmp=1;
    //        停止0 上1  下2  左3  右4
    BYTE d[5]= {0x00,0x08,0x10,0x04,0x02};
    //--------------------简单斜线搜索---------------------------
	while(1)
	{
		if(signal_level()>tmp)
		{
			tmp=signal_level();
			if(move_to_dir_and_delay(d[4],2400)==1) { return ; }
		}
		else
		{
			if(move_to_dir_and_delay(d[3],2400)==1) { return ; }
		}
	}
	tmp=1;
	while(1)
	{
		if(signal_level()>tmp)
		{
			tmp=signal_level();
			if(move_to_dir_and_delay(d[1],2400)==1) { return ; }
		}
		else
		{
			if(move_to_dir_and_delay(d[2],2400)==1) { return ; }
		}
	}
}
/***********************************************
函数功能：螺旋遍历算法
***********************************************/
void screw_auto_alignment()
{
    int i=0,j=0,f=0,k=0;
    //        停止0 上1  下2  左3  右4
    BYTE d[5]= {0x00,0x08,0x10,0x04,0x02};
    BYTE e[4]= {0x02,0x08,0x04,0x10};

    if(auto_init_position()==1) { return; }

    for(i=0; i<10; i++)
    {
        for(j=0; j<=i/2; j++)
        {
            if(f>3) { f=0; }

            if(move_to_dir_and_delay(e[f],2000)==1) { return; }
        }

        f++;
    }
}
/***********************************************
函数功能：初始化云台位置，使云台处于水平和竖直的中点位置
***********************************************/
BYTE auto_init_position()
{
    int i=0,j=0,f=0,k=0;
    //        停止0 上1  下2  左3  右4
    BYTE d[5]= {0x00,0x08,0x10,0x04,0x02};

    if(move_to_dir_and_delay(d[3]|d[2],30000)==1) { return 1; }

    if(move_to_dir_and_delay(d[4]|d[1],7500)==1) { return 1; }

    if(move_to_dir_and_delay(d[1],7500)==1) { return 1; }

    return 0;
}
/***********************************************
函数功能：向dir方向转动delay秒
***********************************************/
BYTE move_to_dir_and_delay(BYTE dir,WORD delay)
{
    int k=0;
    control_yuntai_pelco_d(0x01,dir,0x01,0x01,1);

    for(k=0; k<delay; k++)         //延时30秒 如果有停止自动模式的控制命令发送过来就向云台发送停止指令并跳出自动控制函数
    {
        delay1ms();

        if(check_stop_alignment()==1||signal_level()>0)
        {
            control_yuntai_pelco_d(0x01,0x00,0x00,0x00,1);
            return 1;
        }
    }

    control_yuntai_pelco_d(0x01,0x00,0x00,0x00,1);
    return 0;
}

//------------------------------辅助函数-------------------------------------
BYTE respond_control()
{
	WORD k=0;
    if(rxbusy==0)                  //接收缓冲如果未被占用，检查数据是否合法，并根据结果进行相关操作
    {
        if(rxbuf[0]==0xa0&&(rxbuf[3]==(rxbuf[0]^rxbuf[1]^rxbuf[2])))                //数据合法性检验
        {
            //serial2senddata(0xbb);
            switch(rxbuf[1])
            {
                case 0:
                    control_yuntai_pelco_d(0x01,0x00,0x00,0x00,1); 	        //停止
                    break;

                case 1:
                    control_yuntai_pelco_d(0x01,0x08,0x00,0x01,1);			//上
                    break;

                case 2:
                    control_yuntai_pelco_d(0x01,0x10,0x00,0x01,1);			//下
                    break;

                case 3:
                    control_yuntai_pelco_d(0x01,0x04,0x01,0x00,1);			//左
                    break;

                case 4:
                    control_yuntai_pelco_d(0x01,0x02,0x01,0x00,1);			//右
                    break;

                case 5:
                    automod=1;
                    break;

                case 6:
                    automod=0;
                    break;

                case 7:
                    break;

                case 8:
                    break;

                default:
                    break;
            }

            for(k=0; k<4; k++)  { rxbuf[k]=0; }		   //清除串口1接收缓存
        }
    }
	
}
BYTE signal_level()							  //返回当前有几格信号
{
	BYTE i=5;
	BYTE  signal=0;
	for(i=0;i<5;i++)
	{
		signal=0x01<<(5-i);
		if(signal<=(~P2&0x3f)) break;
	}
	return 5-i;
}
