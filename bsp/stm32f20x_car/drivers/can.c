/**
  ******************************************************************************
  * @file    CAN/CAN_Networking/main.c
  * @author  MCD Application Team
  * @version V1.3.0
  * @date    13-November-2013
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/

#include <stdio.h>

#include <board.h>
#include <rtthread.h>
#include <finsh.h>
#include <rtdevice.h>

#include "include.h"
#include "jt808.h"
#include "can.h"

/** @addtogroup STM32F4xx_StdPeriph_Examples
  * @{
  */

/** @addtogroup CAN_Networking
  * @{
  */
//津	?

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define KEY_PRESSED     0x00
#define KEY_NOT_PRESSED 0x01

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
CAN_InitTypeDef        CAN_InitStructure;
CAN_FilterInitTypeDef  CAN_FilterInitStructure;
CanTxMsg TxMessage;
CanRxMsg RxMessage;
uint8_t ubKeyNumber = 0x0;

CanRxMsg 			can_message_rxbuf[16];
uint8_t				can_message_rxbuf_rd = 0;
volatile uint8_t	can_message_rxbuf_wr = 0;


/* Private function prototypes -----------------------------------------------*/
static void NVIC_Config(void);
static void CAN_Config(void);
static void Delay(void);
void Init_RxMes(CanRxMsg *RxMessage);


//can初始化
void CAN_init(void)
{
    Init_RxMes(&RxMessage);


    /* CAN configuration */
    CAN_Config();
}

//can发送数据
void can_tx(uint8_t tx_data)
{
    TxMessage.Data[0] = tx_data;
    CAN_Transmit(CANx, &TxMessage);
}
FINSH_FUNCTION_EXPORT(can_tx, can_tx);


/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main_stm32_std(void)
{
    /*!< At this stage the microcontroller clock setting is already configured,
         this is done through SystemInit() function which is called from startup
         files (startup_stm32f40_41xxx.s/startup_stm32f427_437xx.s/startup_stm32f429_439xx.s)
         before to branch to application main.
         To reconfigure the default setting of SystemInit() function, refer to
         system_stm32f4xx.c file
       */

    /* NVIC configuration */
    NVIC_Config();

    /* CAN configuration */
    CAN_Config();

    while(1)
    {
        while( KEY_PRESSED)
        {
            if(ubKeyNumber == 0x4)
            {
                ubKeyNumber = 0x00;
            }
            else
            {
                LED_Display(++ubKeyNumber);
                TxMessage.Data[0] = ubKeyNumber;
                CAN_Transmit(CANx, &TxMessage);
            }
        }
    }
}

/******************************************************************************/
/*                 STM32F4xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f40xx.s/startup_stm32f427x.s).                         */
/******************************************************************************/

/**
  * @brief  This function handles CAN1 RX0 request.
  * @param  None
  * @retval None
  */
void CAN1_RX0_IRQHandler(void)
{
    CAN_Receive(CAN1, CAN_FIFO0, &can_message_rxbuf[can_message_rxbuf_wr++]);
    if(can_message_rxbuf_wr >= sizeof(can_message_rxbuf) / sizeof(CanRxMsg))
        can_message_rxbuf_wr = 0;
    /*
    CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);

    //if ((RxMessage.StdId == 0x321)&&(RxMessage.IDE == CAN_ID_STD) && (RxMessage.DLC == 1))
    {
    	rt_kprintf("\nCAN_RX:ID=0x%08X,ExID=0x%08X,IDE=%02X,RTR=%02X,DLC=%02X,FMI=%02X,DATA=",
    				RxMessage.StdId,
    				RxMessage.ExtId,
    				RxMessage.IDE,
    				RxMessage.RTR,
    				RxMessage.DLC,
    				RxMessage.FMI
    				);
    	printf_hex_data(RxMessage.Data, 8);
    }
    */
}

/**
  * @brief  Configures the CAN.
  * @param  None
  * @retval None
  */
static void CAN_Config(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;

    /* CAN GPIOs configuration **************************************************/

    /* Enable GPIO clock */
    RCC_AHB1PeriphClockCmd(CAN_GPIO_CLK, ENABLE);

    /* Connect CAN pins to AF9 */
    GPIO_PinAFConfig(CAN_GPIO_PORT, CAN_RX_SOURCE, CAN_AF_PORT);
    GPIO_PinAFConfig(CAN_GPIO_PORT, CAN_TX_SOURCE, CAN_AF_PORT);

    /* Configure CAN RX and TX pins */
    GPIO_InitStructure.GPIO_Pin = CAN_RX_PIN | CAN_TX_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(CAN_GPIO_PORT, &GPIO_InitStructure);

    /* CAN configuration ********************************************************/
    /* Enable CAN clock */
    RCC_APB1PeriphClockCmd(CAN_CLK, ENABLE);

    /* CAN register init */
    CAN_DeInit(CANx);

    /* CAN cell init */
    CAN_InitStructure.CAN_TTCM = DISABLE;
    CAN_InitStructure.CAN_ABOM = DISABLE;
    CAN_InitStructure.CAN_AWUM = DISABLE;
    CAN_InitStructure.CAN_NART = DISABLE;
    CAN_InitStructure.CAN_RFLM = DISABLE;
    CAN_InitStructure.CAN_TXFP = DISABLE;
    CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;
    CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;

    /* CAN Baudrate = 1 MBps (CAN clocked at 42 MHz) */
    /*
    CAN_InitStructure.CAN_BS1 = CAN_BS1_6tq;
    CAN_InitStructure.CAN_BS2 = CAN_BS2_8tq;
    CAN_InitStructure.CAN_Prescaler = 2;
    */
    /* CAN Baudrate = 20kBps (CAN clocked at 30 MHz) */
#ifdef STM32F4XX
    CAN_InitStructure.CAN_BS1 = CAN_BS1_13tq;
#else
    CAN_InitStructure.CAN_BS1 = CAN_BS1_7tq;
#endif
    CAN_InitStructure.CAN_BS2 = CAN_BS2_7tq;
    CAN_InitStructure.CAN_Prescaler = 8; //250K	        42MHZ     42/(1+bs1+bs2)/prescaler
    //CAN_InitStructure.CAN_Prescaler=100;//20K	        42MHZ     42/(1+bs1+bs2)/prescaler
    CAN_Init(CANx, &CAN_InitStructure);

    /* CAN filter init */
    CAN_FilterInitStructure.CAN_FilterNumber = 0;
    CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
    CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
    CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000;
    CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;
    CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000;
    CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;
    CAN_FilterInitStructure.CAN_FilterFIFOAssignment = 0;
    CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
    CAN_FilterInit(&CAN_FilterInitStructure);

    /* Transmit Structure preparation */
    TxMessage.StdId = 0x321;
    TxMessage.ExtId = 0x01;
    TxMessage.RTR = CAN_RTR_DATA;
    TxMessage.IDE = CAN_ID_STD;
    TxMessage.DLC = 1;
    NVIC_Config();
    /* Enable FIFO 0 message pending Interrupt */
    CAN_ITConfig(CANx, CAN_IT_FMP0, ENABLE);
}

/**
  * @brief  Configures the NVIC for CAN.
  * @param  None
  * @retval None
  */
static void NVIC_Config(void)
{
    NVIC_InitTypeDef  NVIC_InitStructure;

    NVIC_InitStructure.NVIC_IRQChannel = CAN1_RX0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0;
    //NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x1;
    //NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/**
  * @brief  Initializes the Rx Message.
  * @param  RxMessage: pointer to the message to initialize
  * @retval None
  */
void Init_RxMes(CanRxMsg *RxMessage)
{
    uint8_t ubCounter = 0;

    RxMessage->StdId = 0x00;
    RxMessage->ExtId = 0x00;
    RxMessage->IDE = CAN_ID_STD;
    RxMessage->DLC = 0;
    RxMessage->FMI = 0;
    for (ubCounter = 0; ubCounter < 8; ubCounter++)
    {
        RxMessage->Data[ubCounter] = 0x00;
    }
}

/**
  * @brief  Turn ON/OFF the dedicated led
  * @param  Ledstatus: Led number from 0 to 3.
  * @retval None
  */
void LED_Display(uint8_t Ledstatus)
{
    /* Turn off all leds */

    rt_kprintf("\n can_rx=%d", Ledstatus);
    switch(Ledstatus)
    {
    case(1):
        //STM_EVAL_LEDOn(LED1);
        break;

    case(2):
        //STM_EVAL_LEDOn(LED2);
        break;

    case(3):
        //STM_EVAL_LEDOn(LED3);
        break;

    case(4):
        //STM_EVAL_LEDOn(LED4);
        break;
    default:
        break;
    }
}



#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1)
    {
    }
}

#endif






/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

