//******************************************************************************
// STM32F4 Discovery SDCard + FatFs Test - CLIVE - SOURCER32@GMAIL.COM
//******************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stm32f4xx.h"
#include "stm32f4_discovery_sdio_sd.h"

#include "ff.h"
#include "diskio.h"

#include "usart.h"
#include "gpio.h"

#define DBG

//******************************************************************************

void NVIC_Configuration(void);
void RCC_Configuration(void);
void GPIO_Configuration(void);
void USART6_Configuration(void);

//******************************************************************************
/* Exported functions ------------------------------------------------------- */

#ifdef __GNUC__
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

FRESULT res;
FILINFO fno;
FIL fil;
DIR dir;
FATFS fs32;
char* path;

#if _USE_LFN
    static char lfn[_MAX_LFN + 1];
    fno.lfname = lfn;
    fno.lfsize = sizeof lfn;
#endif

//******************************************************************************

char *dec32(unsigned long i)
{
  static char str[16];
  char *s = str + sizeof(str);

  *--s = 0;

  do
  {
    *--s = '0' + (char)(i % 10);
    i /= 10;
  }
  while(i);

  return(s);
}

//******************************************************************************

USART Serial6;
#define STDSERIAL 	Serial6

int main(void)
{
  /*!< At this stage the microcontroller clock setting is already configured,
       this is done through SystemInit() function which is called from startup
       file (startup_stm32f4xx.s) before to branch to application main.
       To reconfigure the default setting of SystemInit() function, refer to
       system_stm32f4xx.c file
     */
	RCC_ClocksTypeDef RCC_Clocks;

  NVIC_Configuration(); /* Interrupt Config */

	
#ifdef DBG

	usart_begin(&Serial6, USART6, PC7, PC6, 115200);

	puts("FatFs Testing\n");
	
	RCC_GetClocksFreq(&RCC_Clocks);

	printf( "SYSCLK = %dl\n", RCC_Clocks.SYSCLK_Frequency);
	printf( ", HCLK = %dl\n", RCC_Clocks.HCLK_Frequency);

#endif

	memset(&fs32, 0, sizeof(FATFS));

	res = f_mount(0, &fs32);

#ifdef DBG
	if (res != FR_OK)
		printf("res = %d f_mount\n", res);
#endif
	
	memset(&fil, 0, sizeof(FIL));
	
	res = f_open(&fil, "MESSAGE.TXT", FA_READ);

#ifdef DBG
	if (res != FR_OK)
		printf("res = %d f_open MESSAGE.TXT\n", res);
#endif
	
	if (res == FR_OK)
	{
		UINT Total = 0;

		while(1)
		{
			BYTE Buffer[512];
			UINT BytesRead;
			UINT i;

			res = f_read(&fil, Buffer, sizeof(Buffer), &BytesRead);

#ifdef DBG
			if (res != FR_OK)
				printf("res = %d f_read MESSAGE.TXT\n", res);
#endif
			
			if (res != FR_OK)
				break;

			Total += BytesRead;

#ifdef DBG
			for(i=0; i<BytesRead; i++)
				putchar(Buffer[i]);
#endif
			
			if (BytesRead < sizeof(Buffer))
				break;
		}

		res = f_close(&fil); // MESSAGE.TXT

#ifdef DBG
		if (res != FR_OK)
			printf("res = %d f_close MESSAGE.TXT\n", res);

		printf("Total = %d\n", Total);
#endif

    res = f_open(&fil, "LENGTH.TXT", FA_CREATE_ALWAYS | FA_WRITE);

#ifdef DBG
		if (res != FR_OK)
			printf("res = %d f_open LENGTH.TXT\n", res);
#endif
	
    if (res == FR_OK)
    {
      UINT BytesWritten;
      char crlf[] = "\r\n";
      char *s = dec32(Total);

      res = f_write(&fil, s, strlen(s), &BytesWritten);

      res = f_write(&fil, crlf, strlen(crlf), &BytesWritten);

  		res = f_close(&fil); // LENGTH.TXT

#ifdef DBG			
  		if (res != FR_OK)
	  		printf("res = %d f_close LENGTH.TXT\n", res);
#endif			
    }
	}

  res = f_open(&fil, "DIR.TXT", FA_CREATE_ALWAYS | FA_WRITE);

#ifdef DBG
	if (res != FR_OK)
		printf("res = %d f_open DIR.TXT\n", res);
#endif
	
  if (res == FR_OK)
  {
    UINT BytesWritten;

		path = "";

		res = f_opendir(&dir, path);

#ifdef DBG
		if (res != FR_OK)
			printf("res = %d f_opendir\n", res);
#endif
	
		if (res == FR_OK)
		{
			while(1)
			{
        char str[256];
        char *s = str;
				char *fn;

				res = f_readdir(&dir, &fno);

#ifdef DBG
				if (res != FR_OK)
					printf("res = %d f_readdir\n", res);
#endif
			
				if ((res != FR_OK) || (fno.fname[0] == 0))
					break;

#if _USE_LFN
				fn = *fno.lfname ? fno.lfname : fno.fname;
#else
				fn = fno.fname;
#endif

#ifdef DBG
				printf("%c%c%c%c ",
					((fno.fattrib & AM_DIR) ? 'D' : '-'),
					((fno.fattrib & AM_RDO) ? 'R' : '-'),
					((fno.fattrib & AM_SYS) ? 'S' : '-'),
					((fno.fattrib & AM_HID) ? 'H' : '-') );

				printf("%10d ", fno.fsize);

				printf("%s/%s\n", path, fn);
#endif
				
		  	*s++ = ((fno.fattrib & AM_DIR) ? 'D' : '-');
				*s++ = ((fno.fattrib & AM_RDO) ? 'R' : '-');
  			*s++ = ((fno.fattrib & AM_SYS) ? 'S' : '-');
	  		*s++ = ((fno.fattrib & AM_HID) ? 'H' : '-');

        *s++ = ' ';

        strcpy(s, dec32(fno.fsize));
        s += strlen(s);

        *s++ = ' ';

        strcpy(s, path);
        s += strlen(s);

        *s++ = '/';

        strcpy(s, fn);
        s += strlen(s);

        *s++ = 0x0D;
        *s++ = 0x0A;
        *s++ = 0;

        res = f_write(&fil, str, strlen(str), &BytesWritten);
			}
		}

  	res = f_close(&fil); // DIR.TXT

#ifdef DBG		
 		if (res != FR_OK)
  		printf("res = %d f_close DIR.TXT\n", res);
#endif		
  }

  while(1); /* Infinite loop */
}

//******************************************************************************

void NVIC_Configuration(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;

  /* Configure the NVIC Preemption Priority Bits */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

  NVIC_InitStructure.NVIC_IRQChannel = SDIO_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

/**************************************************************************************/

/**************************************************************************************/

/**************************************************************************************/

//******************************************************************************
// Hosting of stdio functionality through USART6
//******************************************************************************

#include <rt_misc.h>

#pragma import(__use_no_semihosting_swi)

struct __FILE { 
int handle; /* Add whatever you need here */ 
};
FILE __stdout;
FILE __stdin;

int fputc(int ch, FILE *f)
{
	static int last;

	if ((ch == (int)'\n') && (last != (int)'\r'))
	{
		last = (int)'\r';
  	usart_write(&STDSERIAL, last);
		usart_flush(&STDSERIAL);
	}
	else
		last = ch;

	usart_write(&STDSERIAL, last);

  return(ch);
}

int fgetc(FILE *f)
{
  return((int)usart_read(&STDSERIAL));
}

int ferror(FILE *f)
{
  /* Your implementation of ferror */
  return EOF;
}



/**
  * @brief  Gets numeric values from the hyperterminal.
  * @param  None
  * @retval None
  */
uint8_t USART_Scanf(uint32_t value)
{
  uint32_t index = 0;
  uint32_t tmp[2] = {0, 0};

  while (index < 2)
  {
    /* Loop until RXNE = 1 */
    while (USART_GetFlagStatus(USART6, USART_FLAG_RXNE) == RESET)
    {}
    tmp[index++] = (USART_ReceiveData(USART6));
    if ((tmp[index - 1] < 0x30) || (tmp[index - 1] > 0x39))
    {
      printf("\n\r Please enter valid number between 0 and 9 \n\r");
      index--;
    }
  }
  /* Calculate the Corresponding value */
  index = (tmp[1] - 0x30) + ((tmp[0] - 0x30) * 10);
  /* Checks */
  if (index > value)
  {
    printf("\n\r Please enter valid number between 0 and %d \n\r", value);
    return 0xFF;
  }
  return index;
}

void _sys_exit(int return_code)
{
label:  goto label;  /* endless loop */
}

//******************************************************************************

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  while(1); /* Infinite loop */
}
#endif

//******************************************************************************
