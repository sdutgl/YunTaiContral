#include <STC12C5A60S2.H>
#include <INTRINS.H>
#include <MYDELAY.H>
#define FOSC 12000000           //ʱ��Ƶ��
#define S2RI 0x01
#define S2TI 0x02
#define S2RB8 0x04
#define S2TB8 0x08
#define COM_TX1_Lenth 4         //���ջ����С
typedef unsigned char BYTE;
typedef unsigned int WORD;
sfr IP2H=0xb6;
sbit aux = P0^0;       //P0  0��as62��aux
BYTE signal_level();
void baudchange(long baud);
void Inituart(void);
void init_as62_t30();
void UartInit(void);    //�����ʳ�ʼ��
void delay(BYTE m);
void serial1senddata(BYTE dat);//����1���ֽ�
void serial2senddata(BYTE dat);//���ڶ����ֽ�
void serial2sendstdring(char *s);//���ڶ�����
void send_rssitest_to_as62t30(BYTE dat);   //���͵�ַ�ŵ��ź�ǿ�����ݵ�as62
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
unsigned char active[6]= {0x08,0x02,0x10,0x04,0x00,0x11}; //�����ѯ���ϡ��ҡ��¡���ͣ���Զ�
unsigned char rxbuf[COM_TX1_Lenth];      //����1���ջ���
BYTE automod=0;  //�Զ�ģʽ��־
//BYTE as62_t30_addr=P1&0x0f;       //���뿪�ظı�as62_t30��ַ
//BYTE as62_t30_channel=(P0&0x3e)>>1;
int rxpoint=0;  //��������ָ��
int rxbusy=0;   //����æ��־
BYTE glc=0;
BYTE a[7];
BYTE time0flag=0;
BYTE sum=0;        //�����ź���ʱ���ͼ���
BYTE signal=0;
void main()
{
    aux=1;
    Inituart();		 //���ڳ�ʼ��
    init_as62_t30();	 //433��ʼ
    ES=0;
    //auto_init_position();
    ES=1;
	//P2=0;
    while(1)
    {
        P2|=1;         //ʹ��485
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

/***********��ʼ������*************/
void Uart1_Init(WORD baud)		//1200bps@12.000MHz  ��ʱ��1��Ϊ�����ʷ�������12T
{
    PCON &= 0x7F;		//�����ʲ�����
    SCON = 0x50;		//8λ����,�ɱ䲨����
    AUXR &= 0xBF;		//��ʱ��1ʱ��ΪFosc/12,��12T
    AUXR &= 0xFE;		//����1ѡ��ʱ��1Ϊ�����ʷ�����
    TMOD &= 0x0F;		//�����ʱ��1ģʽλ
    TMOD |= 0x20;		//�趨��ʱ��1Ϊ8λ�Զ���װ��ʽ
    TL1=256-FOSC/baud/32/12; //�趨��ʱ��ֵ
    TH1 = 256-FOSC/baud/32/12;		//�趨��ʱ����װֵ
    ET1 = 0;		//��ֹ��ʱ��1�ж�
    TR1 = 1;		//������ʱ��1
}
void Uart2_Init(WORD baud)		// ����2��ʼ����ʼ�� ʹ�ö��������ʷ�������Ϊ�����ʷ�����
{
    AUXR &= 0xF7;		//�����ʲ�����
    S2CON = 0x50;		//8λ����,�ɱ䲨����
    AUXR &= 0xFB;		//���������ʷ�����ʱ��ΪFosc/12,��12T
    BRT = 256-FOSC/baud/32/12;			//�趨���������ʷ�������װֵ
    AUXR |= 0x10;		//�������������ʷ�����
}
void Inituart(void)    //��ʼ������
{
    Uart1_Init(1200);    //����1��ʼ��
    Uart2_Init(1200);    //����2��ʼ��
    AUXR1 |=0x10;        //�˿�ӳ�䵽P4
    IPH |= 0x18;//����һ���ȼ�1
    IP |= 0x18;
    IPH2 |= 0x01;//���ڶ����ȼ�1
    IP2 |= 0x01;
    EA=1;
    ES=1;
}

void init_as62_t30()  //��ʼ��as62-t30
{
    //P0 &= 0x3f;
    P0 &=0x3f;
}

/************���ڴ�������*******************/

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


void serial1senddata(BYTE dat)	   //����1���͵��ֽ�
{
    SBUF=dat;

    while(!TI);

    TI=0;
}
//void serial1sendstring(char *s)		//����1�����ַ���
//{
//    while(*s)
//    {
//        serial1senddata(*s++);
//    }
//}

void serial2senddata(BYTE dat)		 //����2���͵��ֽ�
{
    S2BUF=dat;

    while(!S2CON&S2TI);

    S2CON&=~S2TI;
}
//void serial2sendstring(char *s)		 //����2�����ַ���
//{
//    while(*s)
//    {
//        serial2senddata(*s++);
//    }
//}

/************�����ź�ǿ�Ȳ���***************/
void send_rssitest_to_as62t30(BYTE dat)   //���͵�ַ�ŵ��ź�ǿ�����ݵ�as62
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

/************************��̨���Ʋ���*****************************/
void control_yuntai_pelco_d(BYTE address,BYTE direction,BYTE horizontalSpeed,BYTE verticalSpeed,BYTE delay)
{
    BYTE f=0;
    BYTE i=0;
    f=delay;
    f=0;

    for(i=0; i<8; i++) { a[i]=0; }

    a[0]=0xFF;				//��ʼ��
    a[1]=address;			   //��ַ��
    a[2]=0x00;			   //����λ1   �޶�������������ͷ���ã�
    a[3]=direction;			   //����λ2   ������̨ת������
    a[4]=horizontalSpeed;			   //����λ3   ������̨ˮƽת���ٶ�
    a[5]=verticalSpeed;			   //����λ4   ������̨��ֱת���ٶ�
    // a[6]=0x0A;			   //ֹͣ��

    for(f=1; f<6; f++)	 //����У����
    {
        a[6]+=a[f];
    }

    for(i=0; i<7; i++)	 //ͨ������2��������
    {
        serial2senddata(a[i]);
        delaynms(30);
    }

    //delaynms(delay);		 //��̨�ڴ˿��Ʒ�ʽ��ת��ʱ��
}

//------------------------------------------�����Զ���׼����---------------------------------------------

/*****************************************************
�������ܣ����433�Ƿ��յ��Ϸ���ֹͣ�Զ���׼ģʽ�ź�
*****************************************************/
BYTE check_stop_alignment()
{
    int k=0;

    if(rxbusy==0)                  //���ջ������δ��ռ�ã���������Ƿ�Ϸ��������ݽ��������ز���
    {
        if(rxbuf[0]==0xa0&&(rxbuf[3]==(rxbuf[0]^rxbuf[1]^rxbuf[2])))                //���ݺϷ��Լ���
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

            for(k=0; k<4; k++)   //������ջ���
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
�������ܣ���̨�Զ���׼�򵥶Խ��������㷨
**********************************/
void auto_alignment_eazysearch()
{
    int i=0,j=0,f=0,k=0;
    //        ֹͣ0 ��1  ��2  ��3  ��4
    BYTE d[5]= {0x00,0x08,0x10,0x04,0x02};
    //-------------------������̨ת���������½�-----------------------

    if(move_to_dir_and_delay(d[3]|d[2],30000)==1) { return ; }

    //--------------------��б������---------------------------
    for(i=0; i<6; i++)
    {
        if(move_to_dir_and_delay(d[4],2400)==1) { return ; }

        if(move_to_dir_and_delay(d[1],4500)==1) { return ; }
    }
}
/**********************************
�������ܣ���̨�Զ���׼�ռ�����㷨
**********************************/
void auto_alignment()
{
    int i=0,j=0,f=0,k=0;
    //        ֹͣ0 ��1  ��2  ��3  ��4
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
�������ܣ��Ż�����Զ���׼�㷨
***********************************************/
void optimized_auto_alignment()
{
    int i=0,j=0,f=0,k=0;
	BYTE tmp=1;
    //        ֹͣ0 ��1  ��2  ��3  ��4
    BYTE d[5]= {0x00,0x08,0x10,0x04,0x02};
    //--------------------��б������---------------------------
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
�������ܣ����������㷨
***********************************************/
void screw_auto_alignment()
{
    int i=0,j=0,f=0,k=0;
    //        ֹͣ0 ��1  ��2  ��3  ��4
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
�������ܣ���ʼ����̨λ�ã�ʹ��̨����ˮƽ����ֱ���е�λ��
***********************************************/
BYTE auto_init_position()
{
    int i=0,j=0,f=0,k=0;
    //        ֹͣ0 ��1  ��2  ��3  ��4
    BYTE d[5]= {0x00,0x08,0x10,0x04,0x02};

    if(move_to_dir_and_delay(d[3]|d[2],30000)==1) { return 1; }

    if(move_to_dir_and_delay(d[4]|d[1],7500)==1) { return 1; }

    if(move_to_dir_and_delay(d[1],7500)==1) { return 1; }

    return 0;
}
/***********************************************
�������ܣ���dir����ת��delay��
***********************************************/
BYTE move_to_dir_and_delay(BYTE dir,WORD delay)
{
    int k=0;
    control_yuntai_pelco_d(0x01,dir,0x01,0x01,1);

    for(k=0; k<delay; k++)         //��ʱ30�� �����ֹͣ�Զ�ģʽ�Ŀ�������͹���������̨����ָֹͣ������Զ����ƺ���
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

//------------------------------��������-------------------------------------
BYTE respond_control()
{
	WORD k=0;
    if(rxbusy==0)                  //���ջ������δ��ռ�ã���������Ƿ�Ϸ��������ݽ��������ز���
    {
        if(rxbuf[0]==0xa0&&(rxbuf[3]==(rxbuf[0]^rxbuf[1]^rxbuf[2])))                //���ݺϷ��Լ���
        {
            //serial2senddata(0xbb);
            switch(rxbuf[1])
            {
                case 0:
                    control_yuntai_pelco_d(0x01,0x00,0x00,0x00,1); 	        //ֹͣ
                    break;

                case 1:
                    control_yuntai_pelco_d(0x01,0x08,0x00,0x01,1);			//��
                    break;

                case 2:
                    control_yuntai_pelco_d(0x01,0x10,0x00,0x01,1);			//��
                    break;

                case 3:
                    control_yuntai_pelco_d(0x01,0x04,0x01,0x00,1);			//��
                    break;

                case 4:
                    control_yuntai_pelco_d(0x01,0x02,0x01,0x00,1);			//��
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

            for(k=0; k<4; k++)  { rxbuf[k]=0; }		   //�������1���ջ���
        }
    }
	
}
BYTE signal_level()							  //���ص�ǰ�м����ź�
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