/******************** (C) COPYRIGHT 2011 Ұ��Ƕ��ʽ���������� ********************
 * �ļ���  ��main.c
 * ����    ����ǿ��������֧��ũ����24�����������������Ĺ��� ����ʱ����1970��ΪԪ�꣬
 *           ��32bit��ʱ��Ĵ����������е�2100�����ҡ�         
 * ʵ��ƽ̨��Ұ��STM32������
 * ��汾  ��ST3.0.0
 *
 * ����    ��fire  QQ: 313303034 
 * ����    ��firestm32.blog.chinaunix.net
**********************************************************************************/
#include "stm32f10x.h"
#include "stdio.h" 
#include "calendar.h"
#include "date.h"

__IO uint32_t TimeDisplay = 0;

void RCC_Configuration(void);
void NVIC_Configuration(void);
void GPIO_Configuration(void);
void USART_Configuration(void);
int fputc(int ch, FILE *f);
void RTC_Configuration(void);
//u32 Time_Regulate(void);
void Time_Regulate(struct rtc_time *tm);
void Time_Adjust(void);
void Time_Display(uint32_t TimeVar);
void Time_Show(void);
u8 USART_Scanf(u32 value);


#define  RTCClockSource_LSE	


u8 const *WEEK_STR[] = {"��", "һ", "��", "��", "��", "��", "��"};
u8 const *zodiac_sign[] = {"��", "��", "ţ", "��", "��", "��", "��", "��", "��", "��", "��", "��"};

struct rtc_time systmtime;

int main()
{
	  /* System Clocks Configuration */
	  RCC_Configuration();
	
	  /* NVIC configuration */
	  NVIC_Configuration();
	
	  GPIO_Configuration();
	
	  USART_Configuration();
	
	  /*������ʱ��鱸�ݼĴ���BKP_DR1��������ݲ���0xA5A5,������������ʱ�䲢ѯ���û�����ʱ��*/
	if (BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5)
	{
	    printf("\r\n\n RTC not yet configured....");
		
	    /* RTC Configuration */
	    RTC_Configuration();
		printf("\r\n RTC configured....");
		
		/* Adjust time by users typed on the hyperterminal */
	    Time_Adjust();
		
		BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);
	}
	else
	{
	    /*��������������ʱ��*/
		/*����Ƿ��������*/
		if (RCC_GetFlagStatus(RCC_FLAG_PORRST) != RESET)
		{
		    printf("\r\n\n Power On Reset occurred....");
		}
		/*����Ƿ�Reset��λ*/
		else if (RCC_GetFlagStatus(RCC_FLAG_PINRST) != RESET)
	    {
	      printf("\r\n\n External Reset occurred....");
	    }
		
		printf("\r\n No need to configure RTC....");
		
		/*�ȴ��Ĵ���ͬ��*/
		RTC_WaitForSynchro();
		
		/*����RTC���ж�*/
		RTC_ITConfig(RTC_IT_SEC, ENABLE);
		
		/*�ȴ��ϴ�RTC�Ĵ���д�������*/
		RTC_WaitForLastTask();
	}

	#ifdef RTCClockOutput_Enable
	  /* Enable PWR and BKP clocks */
	  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
	
	  /* Allow access to BKP Domain */
	  PWR_BackupAccessCmd(ENABLE);
	
	  /* Disable the Tamper Pin */
	  BKP_TamperPinCmd(DISABLE); /* To output RTCCLK/64 on Tamper pin, the tamper
	                                 functionality must be disabled */
	
	  /* Enable RTC Clock Output on Tamper Pin */
	  BKP_RTCOutputConfig(BKP_RTCOutputSource_CalibClock);
	#endif
	
	  /* Clear reset flags */
	  RCC_ClearFlag();
	
	  /* Display time in infinite loop */
	  Time_Show();
}

void RCC_Configuration()
{
		SystemInit();
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
}

void NVIC_Configuration()
{
	  NVIC_InitTypeDef NVIC_InitStructure;
	
	  /* Configure one bit for preemption priority */
	  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	
	  /* Enable the RTC Interrupt */
	  NVIC_InitStructure.NVIC_IRQChannel = RTC_IRQn;
	  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	  NVIC_Init(&NVIC_InitStructure);
}

void GPIO_Configuration()
{
	  GPIO_InitTypeDef GPIO_InitStructure;
	
	   /* Configure USART1 Tx (PA.09) as alternate function push-pull */
	  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	  GPIO_Init(GPIOA, &GPIO_InitStructure);
	    
	  /* Configure USART1 Rx (PA.10) as input floating */
	  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	  GPIO_Init(GPIOA, &GPIO_InitStructure);

}

/*******************************************************************************
* Function Name  : USART_Configuration
* Description    : Configures the USART1.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USART_Configuration()
{
	    USART_InitTypeDef USART_InitStructure;
	
		USART_InitStructure.USART_BaudRate = 115200;
		USART_InitStructure.USART_WordLength = USART_WordLength_8b;
		USART_InitStructure.USART_StopBits = USART_StopBits_1;
		USART_InitStructure.USART_Parity = USART_Parity_No ;
		USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	
		USART_Init(USART1, &USART_InitStructure); 
	    USART_Cmd(USART1, ENABLE);
}

 /*******************************************************************************
* Function Name  : fputc
* Description    : Retargets the C library printf function to the USART or ITM Viewer.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/ 
int fputc(int ch, FILE *f)
{
	  /* ��Printf���ݷ������� */
	  USART_SendData(USART1, (unsigned char) ch);

	  while (!(USART1->SR & USART_FLAG_TXE));
	 
	  return (ch);
}

/***********************************************************************
*��������RTC_Configuration
*����������RTC
*���룺��
*�������
*���أ���
************************************************************************/
void RTC_Configuration()
{
     /*����PWR��BKPʱ��*/
	 RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
	 
	 /*�������BKP��*/
	 PWR_BackupAccessCmd(ENABLE);
	 
	 /*��λ������*/
	 BKP_DeInit();
	 
	 #ifdef RTCClockSource_LSI
	 
	 /*����LSI*/
	 RCC_LSICmd(ENABLE);
	 
	 /*�ȴ�LSI׼����*/
	 while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY)==RESET)
	 {
	 }
	 
	 /*ѡ��LSI��ΪRTCʱ��Դ*/
	 RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
	 
	 #elif defined  RTCClockSource_LSE
	 
	 /*����LSE*/
	 RCC_LSEConfig(RCC_LSE_ON);
	 
	 /*�ȴ�LSE׼����*/
	 while(RCC_GetFlagStatus(RCC_FLAG_LSERDY)==RESET)
	 {
	 }
	 
	 /*ѡ��LSE��ΪRTCʱ��Դ*/
	 RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
	 #endif
	 
	 /* Enable RTC Clock */
     RCC_RTCCLKCmd(ENABLE);
	 
	 #ifdef RTCClockOutput_Enable
	 /*��ֹTamper����*/
	 BKP_TamperPinCmd(DISABLE);/*Ϊ�˽�RTCCLK/64��Tamper���������Tamper���ܱ��뱻��ֹ*/
	 
	 /*����RTCʱ����Tamper���������*/
	 BKP_RTCCalibrationClockOutputCmd(ENABLE);
	 #endif
	 
	 /*�ȴ��Ĵ���ͬ��*/
	 RTC_WaitForSynchro();
	 
	 /*�ȴ��ϴ�RTC�Ĵ���д�������*/
	 RTC_WaitForLastTask();
	 
	 /*����RTC���ж�*/
	 RTC_ITConfig(RTC_IT_SEC, ENABLE);
	 
	 /*�ȴ��ϴ�RTC�Ĵ���д�������*/
	 RTC_WaitForLastTask();
	 
	 #ifdef RTCClockSource_LSI
	 /*���÷�Ƶϵ��*/
	 RTC_SetPrescaler(31999); /*RTC����=RTCCLK/RTC_PR=(32.000kHz/(31999+1))*/
	 
	 #elif defined  RTCClockSource_LSE
	 RTC_SetPrescaler(32767); /*RTC����=RTCCLK/RTC_PR=(32.768kHz/(31767+1))*/
	 #endif
	 
	 /*�ȴ��ϴ�RTC�Ĵ���д�������*/
	 RTC_WaitForLastTask();
	
}

/*******************************************************************************
* Function Name  : Time_Regulate
* Description    : Returns the time entered by user, using Hyperterminal.
* Input          : None
* Output         : None
* Return         : Current time RTC counter value
*******************************************************************************/
//u32 Time_Regulate()
//{
//	  u32 Tmp_HH = 0xFF, Tmp_MM = 0xFF, Tmp_SS = 0xFF;
//	
//	  printf("\r\n==============Time Settings=====================================");
//	  printf("\r\n  Please Set Hours");
//	
//	  while (Tmp_HH == 0xFF)
//	  {
//	    Tmp_HH = USART_Scanf(23);
//	  }
//	  printf(":  %d", Tmp_HH);
//	  printf("\r\n  Please Set Minutes");
//	  while (Tmp_MM == 0xFF)
//	  {
//	    Tmp_MM = USART_Scanf(59);
//	  }
//	  printf(":  %d", Tmp_MM);
//	  printf("\r\n  Please Set Seconds");
//	  while (Tmp_SS == 0xFF)
//	  {
//	    Tmp_SS = USART_Scanf(59);
//	  }
//	  printf(":  %d", Tmp_SS);
//	
//	  /* �����û�����ֵ���Ա����RTC�����Ĵ��� */
//	  return((Tmp_HH*3600 + Tmp_MM*60 + Tmp_SS));
//}

void Time_Regulate(struct rtc_time *tm)
{
	  u32 Tmp_YY = 0xFF, Tmp_MM = 0xFF, Tmp_DD = 0xFF, Tmp_HH = 0xFF, Tmp_MI = 0xFF, Tmp_SS = 0xFF;
	
	  printf("\r\n=========================Time Settings==================");
	
	  printf("\r\n  ���������(Please Set Years):  20");

	  while (Tmp_YY == 0xFF)
	  {
	    Tmp_YY = USART_Scanf(99);
	  }

	  printf("\n\r  ��ݱ�����Ϊ:  20%0.2d\n\r", Tmp_YY);

	  tm->tm_year = Tmp_YY+2000;
	
	  Tmp_MM = 0xFF;

	  printf("\r\n  �������·�(Please Set Months):  ");

	  while (Tmp_MM == 0xFF)
	  {
	    Tmp_MM = USART_Scanf(12);
	  }

	  printf("\n\r  �·ݱ�����Ϊ:  %d\n\r", Tmp_MM);

	  tm->tm_mon= Tmp_MM;
	
	  Tmp_DD = 0xFF;

	  printf("\r\n  ����������(Please Set Dates):  ");

	  while (Tmp_DD == 0xFF)
	  {
	    Tmp_DD = USART_Scanf(31);
	  }

	  printf("\n\r  ���ڱ�����Ϊ:  %d\n\r", Tmp_DD);

	  tm->tm_mday= Tmp_DD;
	
	  Tmp_HH  = 0xFF;

	  printf("\r\n  ������ʱ��(Please Set Hours):  ");

	  while (Tmp_HH == 0xFF)
	  {
	    Tmp_HH = USART_Scanf(23);
	  }

	  printf("\n\r  ʱ�ӱ�����Ϊ:  %d\n\r", Tmp_HH );

	  tm->tm_hour= Tmp_HH;
	    
	  Tmp_MI = 0xFF;

	  printf("\r\n  ���������(Please Set Minutes):  ");

	  while (Tmp_MI == 0xFF)
	  {
	    Tmp_MI = USART_Scanf(59);
	  }

	  printf("\n\r  ���ӱ�����Ϊ:  %d\n\r", Tmp_MI);

	  tm->tm_min= Tmp_MI;
	  
	  Tmp_SS = 0xFF;

	  printf("\r\n  ����������(Please Set Seconds):  ");

	  while (Tmp_SS == 0xFF)
	  {
	    Tmp_SS = USART_Scanf(59);
	  }

	  printf("\n\r  ���ӱ�����Ϊ:  %d\n\r", Tmp_SS);

	  tm->tm_sec= Tmp_SS;
}

/*******************************************************************************
* Function Name  : Time_Adjust
* Description    : Adjusts time.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
//void Time_Adjust()
//{
//	  /* Wait until last write operation on RTC registers has finished */
//	  RTC_WaitForLastTask();
//	  
//	  /* �޸ĵ�ǰRTC�����Ĵ������� */
//	  RTC_SetCounter(Time_Regulate());
//	  
//	  /* Wait until last write operation on RTC registers has finished */
//	  RTC_WaitForLastTask();
//}

void Time_Adjust()
{
	  /* Wait until last write operation on RTC registers has finished */
	  RTC_WaitForLastTask();
	
	  /* Get time entred by the user on the hyperterminal */
	  Time_Regulate(&systmtime);
	  
	  /* Get wday */
	  GregorianDay(&systmtime);

	  /* �޸ĵ�ǰRTC�����Ĵ������� */
	  RTC_SetCounter(mktimev(&systmtime));

	  /* Wait until last write operation on RTC registers has finished */
	  RTC_WaitForLastTask();
}

/*******************************************************************************
* Function Name  : Time_Display
* Description    : Displays the current time.
* Input          : - TimeVar: RTC counter value.
* Output         : None
* Return         : None
*******************************************************************************/
//void Time_Display(u32 TimeVar)
//{
//	  u32 THH = 0, TMM = 0, TSS = 0;
//	
//	  /* Compute  hours */
//	  THH = TimeVar / 3600;
//	  /* Compute minutes */
//	  TMM = (TimeVar % 3600) / 60;
//	  /* Compute seconds */
//	  TSS = (TimeVar % 3600) % 60;
//	
//	  printf("Time: %0.2d:%0.2d:%0.2d\r", THH, TMM, TSS);
//}

void Time_Display(uint32_t TimeVar)
{
	   static uint32_t FirstDisplay = 1;
	   u8 str[15]; // �ַ����ݴ�
	   
	   to_tm(TimeVar, &systmtime);
	
	  if((!systmtime.tm_hour && !systmtime.tm_min && !systmtime.tm_sec)  || (FirstDisplay))
	  {
	      
	      GetChinaCalendar((u16)systmtime.tm_year, (u8)systmtime.tm_mon, (u8)systmtime.tm_mday, str);
	
	      printf("\n\r\n\r  ����ũ����%0.2d%0.2d,%0.2d,%0.2d", str[0], str[1], str[2],  str[3]);
	
	      GetChinaCalendarStr((u16)systmtime.tm_year,(u8)systmtime.tm_mon,(u8)systmtime.tm_mday,str);
	      printf("  %s", str);
	
	     if(GetJieQiStr((u16)systmtime.tm_year, (u8)systmtime.tm_mon, (u8)systmtime.tm_mday, str))
	          printf("  %s\n\r", str);
	
	      FirstDisplay = 0;
	  }

	 

	  /* �������ʱ�� */
	  printf("\r   ��ǰʱ��Ϊ: %d��(%s��) %d�� %d�� (����%s)  %0.2d:%0.2d:%0.2d",
	                    systmtime.tm_year, zodiac_sign[(systmtime.tm_year-3)%12], systmtime.tm_mon, systmtime.tm_mday, 
	                    WEEK_STR[systmtime.tm_wday], systmtime.tm_hour, 
	                    systmtime.tm_min, systmtime.tm_sec);

}

/*******************************************************************************
* Function Name  : Time_Show
* Description    : Shows the current time (HH:MM:SS) on the Hyperterminal.
* Input          : None
* Output         : None
* Return         : None
******************************************************************************/
void Time_Show()
{
	  printf("\n\r");
	
	  /* Infinite loop */
	  while (1)
	  {
	    /* ÿ��1s */
	    if (TimeDisplay == 1)
	    {
	      /* Display current time */
	      Time_Display(RTC_GetCounter());
	      TimeDisplay = 0;
	    }
	  }
}

/*******************************************************************************
* Function Name  : USART_Scanf
* Description    : ��΢�������ն˻�ȡ����ֵ
* Input          : ��
* Output         : ��
* Return         : �����ַ���ASCIIֵ
*******************************************************************************/
u8 USART_Scanf(u32 value)
{
	  u32 index = 0;
	  u32 tmp[2] = {0, 0};
	
	  while (index < 2)
	  {
	    /* Loop until RXNE = 1 */
	    while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET)
	    {
		}
		
	    tmp[index++] = (USART_ReceiveData(USART1));
	    if ((tmp[index - 1] < 0x30) || (tmp[index - 1] > 0x39))   /*����0��9��ASCII��Ϊ0x30��0x39*/
	    {
		  if((index == 2) && (tmp[index - 1] == '\r'))
		  {
		      tmp[1] = tmp[0];
              tmp[0] = 0x30;
		  }
		  else
		  {
		      printf("\n\rPlease enter valid number between 0 and 9 -->:  ");
              index--;
		  }
	    }
	  }
	  
	  /* ���������ַ�����ӦASCIIֵ*/
	  index = (tmp[1] - 0x30) + ((tmp[0] - 0x30) * 10);
	  /* Checks */
	  if (index > value)
	  {
	    printf("\n\rPlease enter valid number between 0 and %d", value);
	    return 0xFF;
	  }
	  return index;
}

/******************* (C) COPYRIGHT 2011 Ұ��Ƕ��ʽ���������� *****END OF FILE****/

