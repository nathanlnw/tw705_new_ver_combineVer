/*SLE��ز�������*/

#include <rtthread.h>
#include <rthw.h>
#include "include.h"
#include "usart.h"
#include "board.h"
#include <serial.h>
#include <string.h>
#include "SLE4442.h"
#include "hmi.h"
#include "menu_include.h"
#include "jt808_GB19056.h"


unsigned int	R_Flag		= 0;
unsigned char	sle_pass[3] = { 0xFF, 0xFF, 0xFF }; //����//{0x23,0x10,0x91};
unsigned int	delayint;

//#define _Nop() for(i=0;i<13;i++){__NOP;}//2us
//#define DELAY5us() for(i=0;i<35;i++){__NOP;}//5.2us


#define IDENTIFY1   0xA2
#define IDENTIFY2   0x13
#define IDENTIFY3   0x10
#define IDENTIFY4   0x91

/*
��ģ��IIC��ʽ
#define SCL_H		( GPIOB->BSRRL = GPIO_Pin_5 )
#define SCL_L		( GPIOB->BSRRH = GPIO_Pin_5 )
#define SCL_read	( GPIOB->IDR & GPIO_Pin_5 )
*/

#define _CardSetRST_HIGH    GPIOD->BSRRL=GPIO_Pin_7; //GPIO_SetBits(GPIOD,GPIO_Pin_7);
#define _CardSetRST_LOW     GPIOD->BSRRH=GPIO_Pin_7; //GPIO_ResetBits(GPIOD,GPIO_Pin_7);

#define _CardSetCLK_HIGH    GPIOB->BSRRL=GPIO_Pin_12; //GPIO_SetBits(GPIOB,GPIO_Pin_12);
#define _CardSetCLK_LOW     GPIOB->BSRRH=GPIO_Pin_12; //GPIO_ResetBits(GPIOB,GPIO_Pin_12);


#define _CardCMDVCC_HIGH     GPIOC->BSRRL=GPIO_Pin_6; //GPIO_SetBits(GPIOC,GPIO_Pin_6);
#define _CardCMDVCC_LOW      GPIOC->BSRRH=GPIO_Pin_6; //GPIO_ResetBits(GPIOC,GPIO_Pin_6);

#define _CardSetPower_HIGH  GPIOB->BSRRL=GPIO_Pin_0; //GPIO_SetBits(GPIOB,GPIO_Pin_0);
#define _CardSetPower_LOW   GPIOB->BSRRH=GPIO_Pin_0; //GPIO_ResetBits(GPIOB,GPIO_Pin_0);


#define MAM 0 /*�������洢������*/
#define SCM 1 /*������ܴ洢������*/
#define PRM 2 /*���屣���洢������*/


void _Nop( void )
{
    u8 i = 0;
    for( i = 0; i < 13; i++ )
    {
        ; //2us
    }
}

/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void DELAY5us( void )
{
    u8 i = 25;
    for( i = 0; i < 25; i++ )
    {
        ;               //5.2us
    }
}

#if 0

static void delay_us( const uint32_t usec )
{
    __IO uint32_t	count	= 0;
    const uint32_t	utime	= ( 168 * usec / 7 );
    do
    {
        if( ++count > utime )
        {
            return;
        }
    }
    while( 1 );
}

#endif

/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void _CardPutIO_HIGH( void )
{
#if 0
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_2MHz; //GPIO_Speed_50MHz;
    GPIO_Init( GPIOD, &GPIO_InitStructure );

    GPIO_SetBits( GPIOD, GPIO_Pin_6 );
#else
    GPIOD->BSRRL = GPIO_Pin_6;
#endif
}

/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void _CardPutIO_LOW( void )
{
#if 0
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_2MHz;
    GPIO_Init( GPIOD, &GPIO_InitStructure );

    GPIO_ResetBits( GPIOD, GPIO_Pin_6 );
#else
    GPIOD->BSRRH = GPIO_Pin_6;

#endif
}

/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
unsigned char _CardReadIO( void )
{
    unsigned char i;
    //_CardPutIO_HIGH;
    i = GPIO_ReadInputDataBit( GPIOD, GPIO_Pin_6 ); //((P6IN&BIT5)>>5);
    return i;
}
/*******************************************************************
   �����ߺ���
   ����ԭ��: void Start_COM();
   ����: ��������������ʼ����.
 ********************************************************************/
void Start_COM( void )
{
    _CardPutIO_HIGH( ); /*������ʼ�����������ź�*/
    _CardSetCLK_LOW;
    _Nop( );
    _Nop( );
    _Nop( );
    _CardSetCLK_HIGH;
    DELAY5us( );        /*��ʼ��������ʱ�����4.7us,��ʱ*/
    _CardPutIO_LOW( );  /*������ʼ�ź�*/
    DELAY5us( );        /*��ʼ��������ʱ�����4 s*/
    _CardSetCLK_LOW;    /*ǯס����׼�����ͻ�������� */
    _Nop( );
    _Nop( );
}

/*******************************************************************
   �������ߺ���
   ����ԭ��: void Stop_COM();
   ����: ����ͽ����ź�
 ********************************************************************/
void Stop_COM( void )
{
    _CardSetCLK_LOW;
    _CardPutIO_LOW( );  /*���ͽ��������������ź�*/
    _Nop( );            /*���ͽ���������ʱ���ź�*/
    _Nop( );
    _Nop( );
    _CardSetCLK_HIGH;   /*������������ʱ�����4 s*/
    DELAY5us( );
    _CardPutIO_HIGH( ); /*�������߽����ź�*/
    _Nop( );
    _Nop( );
}

/*******************************************************************
   �ֽ����ݴ��ͺ���
   ����ԭ��: void SendByte(unsigned char c);
   ����: ������c ���ͳ�ȥ,����������,Ҳ����������
 ********************************************************************/
void SendByte( unsigned char c )
{
    unsigned char BitCnt;
    for( BitCnt = 0; BitCnt < 8; BitCnt++ ) /*Ҫ���͵����ݳ���Ϊ8 λ*/
    {
        if( ( c >> BitCnt ) & 0x01 )
        {
            _CardPutIO_HIGH( );             /*�жϷ���λ*/
        }
        else
        {
            _CardPutIO_LOW( );
        }
        _Nop( );
        _Nop( );
        _CardSetCLK_HIGH;                   /*��ʱ����Ϊ��֪ͨ��������ʼ��������λ*/
        DELAY5us( );                        /*��֤ʱ�Ӹߵ�ƽ���ڴ���4 s*/
        _CardSetCLK_LOW;
    }
}

/*******************************************************************
   �ֽ����ݽ��պ���
   ����ԭ��: unsigned char RcvByte();
   ����: �������մӿ�����������
 ********************************************************************/
unsigned char RcvByte( void )
{
    unsigned char		retc = 0;
    unsigned char		BitCnt;
#if 0
    GPIO_InitTypeDef	GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_IN;
    GPIO_Init( GPIOD, &GPIO_InitStructure );
#endif
    for( BitCnt = 0; BitCnt < 8; BitCnt++ )
    {
        _CardSetCLK_LOW;    /*��ʱ����Ϊ��׼����������λ*/
        DELAY5us( );
        DELAY5us( );        /*ʱ�ӵ͵�ƽ���ڴ���4.7 s*/
        _CardSetCLK_HIGH;   /*��ʱ����Ϊ��ʹ��������������Ч*/
        DELAY5us( );
        retc = retc >> 1;
        if( _CardReadIO( ) )
        {
            retc |= 0x80;   /*������λ,���յ�����λ����retc �� */
        }
        DELAY5us( );
    }
    _CardSetCLK_LOW;
    _Nop( );
    _Nop( );
    return  retc;
}

/*******************************************************************
   ��λ�͸�λ��Ӧ����
   ����ԭ�� void AnRst();
   ���� ��λIC ����������Ӧ�ֽ�
*******************************************************************/
unsigned char AnRst( void )
{
    unsigned char buffer[4], ii = 0;

    _CardSetRST_LOW;
    _CardSetCLK_LOW;
    DELAY5us( );
    DELAY5us( );
    _CardSetRST_HIGH;
    DELAY5us( );
    _CardSetCLK_HIGH;
    DELAY5us( );
    DELAY5us( );
    DELAY5us( );
    _CardSetCLK_LOW;
    DELAY5us( );
    _CardSetRST_LOW;
    _Nop( );

    for( ii = 0; ii < 4; ii++ )
    {
        buffer[ii] = RcvByte( );
    }
    if( ( buffer[0] == IDENTIFY1 ) && ( buffer[1] == IDENTIFY2 ) && ( buffer[2] == IDENTIFY3 ) && ( buffer[3] == IDENTIFY4 ) )
    {
        return 0;   //��λֵ��ȷ,���ظ�λ�ɹ�
    }
    else
    {
        return 1;   //��λֵ����,���ظ�λʧ��
    }
}

/*******************************************************************
   ����4442 �������庯��
   ����ԭ�� void WrmOption();
   ���� ���ʹ���ģʽָ���Ҫ���ô˳���������
*******************************************************************/

void WrmOption( void )
{
    while( 1 )
    {
        _CardSetCLK_LOW;
        _Nop( );
        _Nop( );
        _Nop( );
        _Nop( );
        _Nop( );
        _Nop( );

        _Nop( );
        _Nop( );
        if( _CardReadIO( ) )
        {
            break; /*û�д������������������*/
        }
        _CardSetCLK_HIGH;
        _Nop( );
        _Nop( );
        _Nop( );
        _Nop( );
        _Nop( );
        _Nop( );
    }
}



/*******************************************************************
   ��ֹ��������
   ����ԭ�� void BreakN();
   ���� ��ֹ��ǰ����
*******************************************************************/
void BreakN( void )
{
    _CardSetCLK_LOW;
    DELAY5us( );
    _CardSetRST_HIGH; /*������ֹ������ʱ��*/
    DELAY5us( );
    _CardSetRST_LOW;
}

/*******************************************************************
   ����ͺ���
   ����ԭ�� void SendCOM(unsigned char com1,unsigned char com2,unsigned char com3);
   ���� �����������3 �ֽ�������
   ��������
*******************************************************************/
void SendCOM( unsigned char com1, unsigned char com2, unsigned char com3 )
{
    Start_COM( );
    SendByte( com1 ); /*��������3 �ֽ�ָ��*/
    SendByte( com2 );
    SendByte( com3 );
    Stop_COM( );
}

/*******************************************************************
   SLE4442 �������ݺ���
   ����ԭ��: unsigned char IRcvdat_4442(unsigned char area,unsigned char addr,unsigned char num,unsigned char *buf);
   ����: ��SLE4442 �����ж�����area Ϊ�洢������addr Ϊ��ʼ��ַ
   num Ϊ��ȡ�����ֽ���buf[]Ϊ���ݻ�����ָ��
   ˵�� �����ɹ�����1 ����area ���󷵻�0 ʹ��ǰ���жϿ����û��
 ********************************************************************/
unsigned char IRcvdat_4442( unsigned char area, unsigned char addr, unsigned char num, unsigned char *buf )
{
    unsigned char i;

    switch( area )
    {
    case MAM:
        if( AnRst( ) == 1 )             /*��λSLE4442 �����ո�λ��Ӧ*/
        {
            return 0;
        }
        SendCOM( 0x30, addr, 0x00 );    /*�����洢��*/
        for( i = 0; i < num; i++ )
        {
            *buf = RcvByte( );
            buf++;
        }
        BreakN( );
        break;
    case SCM:
        if( AnRst( ) == 1 )
        {
            return 0;
        }
        SendCOM( 0x31, 0x00, 0x00 );
        for( i = 0; i < num; i++ )
        {
            *buf = RcvByte( );
            buf++;
        }
        BreakN( );
        break;
    case PRM:
        AnRst( );
        SendCOM( 0x34, 0x00, 0x00 );
        for( i = 0; i < num; i++ )
        {
            *buf = RcvByte( );
            buf++;
        }
        BreakN( );
        break;
    default:
        return 0;
    }
    return  1;
}

/*******************************************************************
   SLE4442 ��д���ݺ���
   ����ԭ��: unsigned char ISenddat_4442(unsigned char area,unsigned char addr,unsigned char num,unsigned char *buf);
   ����: ��SLE4442 ������д����area Ϊ�洢������addr Ϊ��ʼ��ַ
   num Ϊ��ȡ�����ֽ���buf[]Ϊ���ݻ�����ָ��
   ˵�� �����ɹ�����1 ����area ���󷵻�0 ʹ��ǰ���жϿ����û��
 ********************************************************************/
unsigned char ISenddat_4442( unsigned char area, unsigned char addr, unsigned char num, unsigned char *buf )
{
    unsigned char i;

    switch( area )
    {
    case MAM:
        if( AnRst( ) == 1 )
        {
            return 0x00;
        }
        for( i = 0; i < num; i++ )
        {
            SendCOM( 0x38, addr + i, *buf );    /*д���洢��*/
            buf++;
            WrmOption( );                       /*���Ͳ�������*/
        }
        break;
    case SCM:
        AnRst( );
        for( i = 0; i < num; i++ )
        {
            SendCOM( 0x39, addr + i, *buf );
            buf++;
            WrmOption( );
        }
        break;
    case PRM:
        AnRst( );
        for( i = 0; i < num; i++ )
        {
            SendCOM( 0x3c, addr + i, *buf );
            buf++;
            WrmOption( );
        }
        break;
    default:
        return ( 0 );
    }
    return ( 1 );
}

/*******************************************************************
   SLE4442 ��У�����뺯��
   ����ԭ��: unsigned char IChkpsw_4442(void);
   ����: ����SLE4442 ����������˶Ժ˶Ժ��ܽ���д����
   ˵�� �����ɹ�����0x00 ����Ч���𻵷���0x01,�������
   ��0x02 ��ֻʣ1 �λ��᷵��0x03.
 ********************************************************************/
unsigned char IChkpsw_4442( void )
{
    unsigned char ec;
    if( IRcvdat_4442( SCM, 0x00, 1, &ec ) == 0 )
    {
        return 0x01;
    }
    switch( ec & 0x7 )
    {
    case 1:
    case 2:
    case 4:
        return 0x3;
    case 3:
    case 5:
        ec	= 0x1;
        break;
    case 6:
        ec	= 0x2;
        break;
    case 7:
        ec	= 0x3;
        break;
    default:
        return 0x1;
    }
    if( AnRst( ) == 1 )
    {
        return 0x01;
    }
    SendCOM( 0x39, 0x00, ec ); //��дEC �ֽ�
    WrmOption( );
    SendCOM( 0x33, 0x01, sle_pass[0] );
    WrmOption( );
    SendCOM( 0x33, 0x02, sle_pass[1] );
    WrmOption( );
    SendCOM( 0x33, 0x03, sle_pass[2] );
    WrmOption( );
    SendCOM( 0x39, 0x00, 0xff ); //�޸�ECֵ
    WrmOption( );
    ec = 0;
    if( IRcvdat_4442( SCM, 0x00, 1, &ec ) == 0 )
    {
        return 0x01;
    }

    if( ( ec & 0x07 ) != 0x07 )
    {
        return ( 0x02 );
    }
    return ( 0x00 );
}



/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void Init_4442( void )
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOA, ENABLE );
    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOB, ENABLE );
    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOC, ENABLE );
    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOD, ENABLE );
    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOE, ENABLE );

    GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_NOPULL;

    GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_0 | GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;
    GPIO_Init( GPIOB, &GPIO_InitStructure );

    GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;
    GPIO_Init( GPIOD, &GPIO_InitStructure );

#if 0
    GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;
    GPIO_Init( GPIOD, &GPIO_InitStructure );
    GPIO_SetBits( GPIOD, GPIO_Pin_6 );
#else
    GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType	= GPIO_OType_OD;
    GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;
    GPIO_Init( GPIOD, &GPIO_InitStructure );
    GPIOD->BSRRL = GPIO_Pin_6;
#endif

    GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;
    GPIO_Init( GPIOC, &GPIO_InitStructure );
    GPIO_SetBits( GPIOC, GPIO_Pin_6 );

    GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_7; // IC  ������ָʾ
    GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_IN;
    GPIO_Init( GPIOC, &GPIO_InitStructure );
}



/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
unsigned char Rx_4442( unsigned char addr, unsigned char num, unsigned char *buf )
{
    if( addr < 32 )
    {
        return 0x03;
    }
    if( ( GPIO_ReadInputDataBit( GPIOC, GPIO_Pin_7 ) ) == 0 )
    {
        return 0x01;
    }
    if( IRcvdat_4442( MAM, addr, num, buf ) == 0 )
    {
        return 0x02;
    }
    return 0x00;
}

/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
unsigned char Tx_4442( unsigned char addr, unsigned char num, unsigned char *buf )
{
    if( addr < 32 )
    {
        return 0x03;
    }
    if( ( GPIO_ReadInputDataBit( GPIOC, GPIO_Pin_7 ) ) == 0 )
    {
        return 0x01;
    }
    if( IChkpsw_4442( ) == 0 )
    {
        if( ISenddat_4442( MAM, addr, num, buf ) == 0 )
        {
            return 0x02;
        }
    }
    else
    {
        return 0x02;
    }
    return 0x00;
}


unsigned char	IC_Check_Count	= 0;
unsigned char	IC_NotCheck_Count	= 0;
unsigned int	DelayCheckIc	= 0;

#define b_CardEdge 0x0001


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/

TYPE_IC_CARD_PARA ic_card_para = {0, 0, 0, 0, 0};



void CheckICCard( void )
{
    //	unsigned char	write_flag	= 0;
    u8				i, crc, result = 0;
    unsigned char	buf[20];

    unsigned char	buf1[12];
    unsigned char	buf2[100];
    unsigned char	buf3[4];
    unsigned char	buf4[22];
    unsigned char	buf5[40];
    unsigned char	buf6[42];

    if( GPIO_ReadInputDataBit( GPIOC, GPIO_Pin_7 ) )
    {
        IC_NotCheck_Count = 0;
        if(ic_card_para.IC_Card_Checked == 1) return;
        IC_Check_Count++;
        if( IC_Check_Count >= 10 )
        {
            IC_NotCheck_Count = 0;
            IC_Check_Count = 0;
            R_Flag			|= b_CardEdge;
            _CardCMDVCC_LOW;
            for( DelayCheckIc = 0; DelayCheckIc < 500; DelayCheckIc++ )
            {
                DELAY5us( );
            }
            _CardSetPower_HIGH;
            _CardSetRST_LOW;
            for( DelayCheckIc = 0; DelayCheckIc < 15; DelayCheckIc++ )
            {
                _CardSetCLK_LOW;
                DELAY5us( );
                DELAY5us( );
                DELAY5us( );
                _CardSetCLK_HIGH;
                DELAY5us( );
                DELAY5us( );
                DELAY5us( );
                _CardSetCLK_LOW;
            }
            R_Flag		&= ~b_CardEdge;
            Rx_4442( 241, 13, buf );
            ic_card_para.card_change_mytime = mytime_now;
            ic_card_para.IC_Card_valitidy[0]	= 0x20;
            ic_card_para.IC_Card_valitidy[1]	= 0x20;
            ic_card_para.IC_Card_valitidy[2]	= 0x01;
            ic_card_para.IC_Card_valitidy[3]	= 0x01;
            //ic_card_para.IC_Card_valitidy	= 0xffffffff;
            if( strncmp( (char *)buf, "administrator", 13 ) == 0 )
            {
                debug_write( "����Ա��" );
                ic_card_para.administrator_card = 1;
                ic_card_para.IC_Card_Checked = 1;
                ic_card_para.card_state	= 0;
                beep(5, 5, 2);
            }
            else
            {
                ic_card_para.administrator_card = 0;
                //#ifdef JT808_TEST_JTB
                result = Rx_4442( 32, 96, buf2 );							//����ʻԱ����
                if(result == 0)
                {
                    for(i = 0, crc = 0; i < 95; i++)
                    {
                        crc ^= buf2[i];
                    }
                    if(buf2[95] == crc)
                    {
                        ///��ʻ֤��Ч��
                        ic_card_para.IC_Card_valitidy[0]	= 0x20;
                        ic_card_para.IC_Card_valitidy[1]	= buf2[18];
                        ic_card_para.IC_Card_valitidy[2]	= buf2[19];
                        ic_card_para.IC_Card_valitidy[3]	= buf2[20];

                        ///��ҵ�ʸ�֤
                        memcpy(buf5, buf2 + 21, 18);

                        ///��ʻԱ����
                        result = Rx_4442( 128, 10, buf1 );							//����ʻԱ����
                        memset(buf3, 0, sizeof(buf3));
                        memset(buf4, 0, sizeof(buf4));
                        memset(buf6, 0, sizeof(buf6));
                    }
                    else
                    {
                        //result = 1;
                        result = 0;
                        debug_write( "�ǹ�������׼IC��!" );
                        result = Rx_4442( 70, 10, buf1 );							//����ʻԱ����
                        result += Rx_4442( 52, 18, buf2 );							//����ʻ֤����
                        result += Rx_4442( 49, 3, buf3 );							//����ʻԱ����
                        result += Rx_4442( 80, 20, buf4 );							//���֤����
                        result += Rx_4442( 100, 40, buf5 );							//��ҵ�ʸ�֤
                        result += Rx_4442( 141, 41, buf6 );							//��֤����
                    }
                }

                //#else
                /*
                result = Rx_4442( 70, 10, buf1 );							//����ʻԱ����
                result += Rx_4442( 52, 18, buf2 );							//����ʻ֤����
                result += Rx_4442( 49, 3, buf3 );							//����ʻԱ����
                result += Rx_4442( 80, 20, buf4 );							//���֤����
                result += Rx_4442( 100, 40,buf5 );							//��ҵ�ʸ�֤
                result += Rx_4442( 141, 41, buf6 );							//��֤����
                */
                //#endif
                rt_kprintf( "\n IC result=%d", result );
                /*
                rt_kprintf( "\n��ʻԱ����:%s", buf1 );
                rt_kprintf( "\n��ʻ֤����:%s", buf2 );
                rt_kprintf( "\n��ʻԱ����:%s", buf3 );
                rt_kprintf( "\n���֤����:%s", buf4 );
                rt_kprintf( "\n��ҵ�ʸ�֤:%s", buf5 );
                rt_kprintf( "\n��֤����:%10s", buf6 );
                */
                if(result == 0)
                {
                    debug_write( "IC����������" );
                    ic_card_para.IC_Card_Checked = 1;
                    ic_card_para.card_state	= 0;
                    memset(jt808_param.id_0xF008, 0, sizeof(jt808_param.id_0xF008));
                    memset(jt808_param.id_0xF009, 0, sizeof(jt808_param.id_0xF009));
                    memset(jt808_param.id_0xF00B, 0, sizeof(jt808_param.id_0xF00B));
                    memset(jt808_param.id_0xF00C, 0, sizeof(jt808_param.id_0xF00C));
                    strncpy(jt808_param.id_0xF008, buf1, 10);
                    strncpy(jt808_param.id_0xF009, buf2, 18);
                    strncpy(jt808_param.id_0xF00B, buf5, 20);
                    strncpy(jt808_param.id_0xF00C, buf6, 32);
                    sprintf(buf6, "��֤����:%s", jt808_param.id_0xF00C);
                    debug_write( buf6 );
                    sprintf(buf6, "��ʻ֤����:%s", jt808_param.id_0xF009);
                    debug_write( buf6 );
                    sprintf(buf6, "��ʻ�ʸ�֤:%s", jt808_param.id_0xF00B);
                    debug_write( buf6 );
                    sprintf(buf6, "����:%s", jt808_param.id_0xF008);
                    debug_write( buf6 );
                    param_save(1);


                    //---GB IC  ������
                    GB_IC_Check_IN();
                    // ---  GB IC �� end

                    beep(5, 5, 2);
                }
                else
                {
                    debug_write( "IC������" );
                    ic_card_para.card_state	= 4;
                    ic_card_para.IC_Card_Checked = 1;
                    beep(2, 2, 3);
                }
                if( device_unlock )
                {
                    IC_card_showinfo = 1;
                    pMenuItem = &Menu_2_5_DriverInfor;
                    pMenuItem->show( );
                }
                _CardSetRST_HIGH;
                _CardSetPower_LOW;
                _CardCMDVCC_HIGH;
                IC_CARD_jt808_0x0702(mast_socket->linkno, 0);
            }
        }
    }
    else
    {
        IC_Check_Count = 0;
        IC_NotCheck_Count++;
        if(IC_NotCheck_Count == 10)
        {
            //_CardSetRST_HIGH;
            //_CardSetPower_LOW;
            //_CardCMDVCC_HIGH;
            ic_card_para.card_state	= 0;
            ic_card_para.administrator_card = 0;
            if(ic_card_para.IC_Card_Checked == 1)
            {
                ic_card_para.IC_Card_Checked = 0;
                ic_card_para.card_change_mytime = mytime_now;
                IC_CARD_jt808_0x0702(mast_socket->linkno, 0);

                //  GB IC �� �γ�
                GB_IC_Check_OUT();


                beep(10, 10, 1);
            }
        }
    }
}





/*********************************************************************************
  *��������:rt_err_t IC_CARD_jt808_0x0702_ex(uint8_t linkno, uint8_t is_ack, uint8_t is_794 )
  *��������:����IC���û���Ϣ������������Ϊ�������ͺͱ���Ӧ������������is_ackΪ0������Ϊ1
  *��	��:	linkno	:������·���
  			is_ack	:Ϊ0��ʾ�����������ݣ���ҪӦ��Ϊ1��ʾӦ����λ�����󣬲���ҪӦ��
  			is_794	:Ϊ1ʱ�����ϵ�794Э�鷢�ͣ�0ʱ�����µ�808��Э�鷢��
  *��	��:	none
  *�� �� ֵ:rt_err_t
  *��	��:������
  *��������:2014-01-24
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
rt_err_t IC_CARD_jt808_0x0702_ex(uint8_t linkno, uint8_t is_ack, uint8_t is_794 )
{
    u8						ptempbuf[128];
    u16						datalen = 0;
    JT808_TX_NODEDATA		 *pnodedata;
    u8 						i;

    if(is_794)
    {
        memset(ptempbuf, 0, sizeof(ptempbuf));
        //��ʻԱ�������ȣ�1byte
        ptempbuf[datalen++]	= strlen(jt808_param.id_0xF008);
        //��ʻԱ����
        memcpy(ptempbuf + datalen, jt808_param.id_0xF008, strlen(jt808_param.id_0xF008));
        datalen	+= strlen(jt808_param.id_0xF008);
        //���֤���룬���ȹ̶�Ϊ20�ֽڣ�
        memcpy(ptempbuf + datalen, jt808_param.id_0xF009, strlen(jt808_param.id_0xF009));
        datalen += 20;
        //��ҵ�ʸ�֤�����,���ȹ̶�Ϊ40�ֽ�
        memcpy(ptempbuf + datalen, jt808_param.id_0xF00B, strlen(jt808_param.id_0xF00B));
        datalen	+= 40;
        //��֤�������Ƴ��ȣ�1byte
        ptempbuf[datalen++]	= strlen(jt808_param.id_0xF00C);
        //��֤��������
        memcpy(ptempbuf + datalen, jt808_param.id_0xF00C, strlen(jt808_param.id_0xF00C));
        datalen	+= strlen(jt808_param.id_0xF00C);
    }
    else
    {
        //1byte��1��ʾ��Ƭ�Ѿ����룬2��ʾû�в���
        if(ic_card_para.IC_Card_Checked)
        {
            ptempbuf[datalen++]	= 1;
            //ʱ��BCD,6byte
            mytime_to_bcd(ptempbuf + datalen, ic_card_para.card_change_mytime);
            datalen += 6;
            //IC����ȡ�����1byte
            ptempbuf[datalen++]	= ic_card_para.card_state;
            //��ʻԱ�������ȣ�1byte
            ptempbuf[datalen++]	= strlen(jt808_param.id_0xF008);
            //��ʻԱ����
            memcpy(ptempbuf + datalen, jt808_param.id_0xF008, strlen(jt808_param.id_0xF008));
            datalen	+= strlen(jt808_param.id_0xF008);
            //��ҵ�ʸ�֤�����
            memcpy(ptempbuf + datalen, jt808_param.id_0xF00B, 20);
            datalen	+= 20;
            //��֤�������Ƴ��ȣ�1byte
            ptempbuf[datalen++]	= strlen(jt808_param.id_0xF00C);
            //��֤��������
            memcpy(ptempbuf + datalen, jt808_param.id_0xF00C, strlen(jt808_param.id_0xF00C));
            datalen	+= strlen(jt808_param.id_0xF00C);
            //֤����Ч��,4BYTE,
            for(i = 0; i < 4; i++)
            {
                ptempbuf[datalen++]	= ic_card_para.IC_Card_valitidy[i];
            }
        }
        else
        {
            ptempbuf[datalen++]	= 2;
            //ʱ��BCD,6byte
            mytime_to_bcd(ptempbuf + datalen, ic_card_para.card_change_mytime);
            datalen += 6;
        }
    }

    if(is_ack)
    {
        jt808_add_tx(linkno, SINGLE_FIRST, 0x0702, -1, RT_NULL, RT_NULL, datalen, ptempbuf, RT_NULL );
    }
    else
    {
        jt808_add_tx(linkno, SINGLE_CMD, 0x0702, -1, RT_NULL, RT_NULL, datalen, ptempbuf, RT_NULL );
    }
    return RT_EOK;
}




/*********************************************************************************
  *��������:rt_err_t IC_CARD_jt808_0x0702(uint8_t linkno, uint8_t is_ack )
  *��������:����IC���û���Ϣ��������2013��808Э�飬��Ϊ�������ͺͱ���Ӧ������������is_ackΪ0������Ϊ1
  *��	��:	linkno	:������·���
  			is_ack	:Ϊ0��ʾ�����������ݣ���ҪӦ��Ϊ1��ʾӦ����λ�����󣬲���ҪӦ��
  *��	��:	none
  *�� �� ֵ:rt_err_t
  *��	��:������
  *��������:2014-01-24
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
rt_err_t IC_CARD_jt808_0x0702(uint8_t linkno, uint8_t is_ack )
{
    return IC_CARD_jt808_0x0702_ex(linkno, is_ack, 0);
}



/*********************************************************************************
  *��������:rt_err_t IC_CARD_jt808_0x0702(uint8_t linkno, uint8_t is_ack )
  *��������:����IC���û���Ϣ������������Ϊ�������ͺͱ���Ӧ������������is_ackΪ0������Ϊ1
  *��	��:	linkno	:������·���
  			is_ack	:Ϊ0��ʾ�����������ݣ���ҪӦ��Ϊ1��ʾӦ����λ�����󣬲���ҪӦ��
  *��	��:	none
  *�� �� ֵ:rt_err_t
  *��	��:������
  *��������:2014-01-24
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
rt_err_t IC_CARD_jt808_0x0702_2013(uint8_t linkno, uint8_t is_ack )
{
    u8						ptempbuf[128];
    u16						datalen = 0;
    JT808_TX_NODEDATA		 *pnodedata;

    //1byte��1��ʾ��Ƭ�Ѿ����룬2��ʾû�в���
    if(ic_card_para.IC_Card_Checked)
        ptempbuf[datalen++]	= 1;
    else
        ptempbuf[datalen++]	= 2;

    //ʱ��BCD,6byte
    mytime_to_bcd(ptempbuf + datalen, ic_card_para.card_change_mytime);
    datalen += 6;
    //IC����ȡ�����1byte
    ptempbuf[datalen++]	= ic_card_para.card_state;
    //��ʻԱ�������ȣ�1byte
    ptempbuf[datalen++]	= strlen(jt808_param.id_0xF008);
    //��ʻԱ����
    memcpy(ptempbuf + datalen, jt808_param.id_0xF008, strlen(jt808_param.id_0xF008));
    datalen	+= strlen(jt808_param.id_0xF008);
    //��ҵ�ʸ�֤�����
#ifdef JT808_TEST_JTB
    memcpy(ptempbuf + datalen, jt808_param.id_0xF00B, 40);
    datalen	+= 40;
#else
    memcpy(ptempbuf + datalen, jt808_param.id_0xF00B, 20);
    datalen	+= 20;
#endif
    //��֤�������Ƴ��ȣ�1byte
    ptempbuf[datalen++]	= strlen(jt808_param.id_0xF00C);
    //��֤��������
    memcpy(ptempbuf + datalen, jt808_param.id_0xF00C, strlen(jt808_param.id_0xF00C));
    datalen	+= strlen(jt808_param.id_0xF00C);
    //֤����Ч��,4BYTE,
    ptempbuf[datalen++]	= 0x20;
    /*
    ptempbuf[datalen++]	= ic_card_para.IC_Card_valitidy[0];
    ptempbuf[datalen++]	= ic_card_para.IC_Card_valitidy[1];
    ptempbuf[datalen++]	= ic_card_para.IC_Card_valitidy[2];
    ptempbuf[datalen++]	= ic_card_para.IC_Card_valitidy[3];
    */
    ptempbuf[datalen++]	= 0x20;
    ptempbuf[datalen++]	= 0x01;
    ptempbuf[datalen++]	= 0x01;


    if(is_ack)
    {
        jt808_add_tx(linkno, SINGLE_FIRST, 0x0702, -1, RT_NULL, RT_NULL, datalen, ptempbuf, RT_NULL );
    }
    else
    {
        jt808_add_tx(linkno, SINGLE_CMD, 0x0702, -1, RT_NULL, RT_NULL, datalen, ptempbuf, RT_NULL );
    }
    return RT_EOK;
}

/************************************** The End Of File **************************************/
