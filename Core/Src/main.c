/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 Spider.
  * All rights reserved.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* Private includes ----------------------------------------------------------*/
#include "drv_usb_hw.h"
#include "drv_usb_core.h"
#include "usbh_usr.h"
#include "usbh_msc_core.h"

//#define APP1_START  (uint32_t)0x0800C000

usbh_host usb_host;

static void initHXTAL(void)
{
    // Enable HXTAL
    rcu_osci_on(RCU_HXTAL);
    // Whait for HXTAL run
    while (rcu_flag_get(RCU_FLAG_HXTALSTB) == RESET)
        ;
    rcu_hxtal_clock_monitor_enable();

    rcu_ahb_clock_config(RCU_AHB_CKSYS_DIV1);
    rcu_apb1_clock_config(RCU_APB1_CKAHB_DIV2);
    rcu_apb2_clock_config(RCU_APB2_CKAHB_DIV1);

    // XTAL: 8MHz / 2 = 4MHz * 30 = 120MHz
    rcu_predv0_config(RCU_PREDV0SRC_HXTAL, RCU_PREDV0_DIV2);
    rcu_pll_config(RCU_PLLSRC_HXTAL, RCU_PLL_MUL30);

    // Enable PLL
    rcu_osci_on(RCU_PLL_CK);
    // Wait for PLL run
    while (rcu_flag_get(RCU_FLAG_PLLSTB) == RESET)
        ;

    // Select PLL as clock source
    rcu_system_clock_source_config(RCU_CKSYSSRC_PLL);

    // Wait for PLL selected as source
    while (rcu_system_clock_source_get() != RCU_SCSS_PLL)
        ;

    rcu_predv1_config(RCU_PREDV1_DIV8);
    rcu_pll1_config(RCU_PLL1_MUL8);
    rcu_pll2_config(RCU_PLL1_MUL8);

    rcu_pllt_config(RCU_PLLTSRC_HXTAL);

    //SystemCoreClock = 120000000ULL;
    SystemCoreClockUpdate();

    systick_config();
}

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    initHXTAL();

    usb_rcu_config();    

    usb_timer_init();

    /* configure GPIO pin used for switching VBUS power and charge pump I/O */
    // usb_vbus_config();

    /* register device class */
    usbh_class_register(&usb_host, &usbh_msc);

    /* initialize host library */
    usbh_init(&usb_host, &usr_cb);

    /* enable interrupts */
    usb_intr_config();

    while (1) {
        /* host task handler */
        usbh_core_task(&usb_host);
        if (usb_host.cur_state == HOST_DEFAULT) {
          usb_host.usr_cb->dev_detach();
        }
    }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  NVIC_SystemReset();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
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
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
