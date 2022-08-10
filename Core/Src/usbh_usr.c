/*!
    \file    usbh_usr.c
    \brief   user application layer for USBFS host-mode MSC class operation

    \version 2020-07-28, V3.0.0, firmware for GD32F20x
    \version 2021-07-30, V3.1.0, firmware for GD32F20x
    \version 2022-06-30, V3.2.0, firmware for GD32F20x
*/

/*
    Copyright (c) 2022, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software without
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/

#include <stdlib.h>
#include <string.h>
#include "usbh_usr.h"
#include "drv_usb_hw.h"
#include "usbh_msc_core.h"
#include "usbh_msc_scsi.h"
#include "usbh_msc_bbb.h"
#include "ff.h"
#include "main.h"
#include "LCD_Init.h"
#include "systick.h"

extern usb_core_driver usbh_core;
extern usbh_host usb_host;

#define BLOCK_SIZE 0x800ULL

static FATFS fatfs;
static FIL file;
static FSIZE_t offset;
static uint8_t buffer[FF_MAX_SS+sizeof(UINT)];
static uint8_t usbh_usr_application_state = USBH_USR_FS_INIT;

/*  points to the DEVICE_PROP structure of current device */
usbh_user_cb usr_cb =
{
    usbh_user_init,
    usbh_user_deinit,
    usbh_user_device_connected,
    usbh_user_device_reset,
    usbh_user_device_disconnected,
    usbh_user_over_current_detected,
    usbh_user_device_speed_detected,
    usbh_user_device_desc_available,
    usbh_user_device_address_assigned,
    usbh_user_configuration_descavailable,
    usbh_user_manufacturer_string,
    usbh_user_product_string,
    usbh_user_serialnum_string,
    usbh_user_enumeration_finish,
    usbh_user_userinput,
    usbh_usr_msc_application,
    usbh_user_device_not_supported,
    usbh_user_unrecovered_error
};

static void run_application(void);
static void LCD_GPIO_Config(void);
static void GD_LCD_EXMC_Config(void);

/*!
    \brief      user operation for host-mode initialization
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usbh_user_init(void)
{
    static uint8_t startup = 0U;

    if (0U == startup) {
        startup = 1U;

        rcu_periph_clock_enable(RCU_GPIOD);
        rcu_periph_clock_enable(RCU_GPIOE);
        rcu_periph_clock_enable(RCU_AF);

        gpio_pin_remap_config(GPIO_SWJ_SWDPENABLE_REMAP, ENABLE);

        LCD_GPIO_Config();
        GD_LCD_EXMC_Config();
        
        LCD_Init();
        LCD_Clear(BLACK);
        Lcd_Put_Text(10, 10, 16, "Checking USB Drive...", WHITE);

        gpio_init(LCD_LED_GPIO_Port, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, LCD_LED_Pin);
        gpio_init(BUZZER_GPIO_Port, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, BUZZER_Pin);

        rcu_periph_clock_enable(RCU_TIMER3);
        timer_deinit(TIMER3);

        gpio_pin_remap_config(GPIO_TIMER3_REMAP, ENABLE);

        timer_oc_parameter_struct timer_ocintpara;
        timer_parameter_struct timer_initpara;

        /* TIMER3 configuration */
        timer_initpara.prescaler = 0U;
        timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
        timer_initpara.counterdirection = TIMER_COUNTER_UP;
        timer_initpara.period = 59999U;
        timer_initpara.clockdivision = TIMER_CKDIV_DIV4;
        timer_initpara.repetitioncounter = 0;
        timer_init(TIMER3, &timer_initpara);

        /* CH0 configuration in PWM mode 0 */
        timer_ocintpara.outputstate = TIMER_CCX_ENABLE;
        timer_ocintpara.outputnstate = TIMER_CCXN_DISABLE;
        timer_ocintpara.ocpolarity = TIMER_OC_POLARITY_HIGH;
        timer_ocintpara.ocnpolarity = TIMER_OCN_POLARITY_HIGH;
        timer_ocintpara.ocidlestate = TIMER_OC_IDLE_STATE_LOW;
        timer_ocintpara.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;

        timer_channel_output_config(TIMER3, TIMER_CH_0, &timer_ocintpara);
        timer_channel_output_pulse_value_config(TIMER3, TIMER_CH_0, timer_initpara.period/2);
        timer_channel_output_mode_config(TIMER3, TIMER_CH_0, TIMER_OC_MODE_PWM0);
        timer_channel_output_shadow_config(TIMER3, TIMER_CH_0, TIMER_OC_SHADOW_DISABLE);
        timer_auto_reload_shadow_enable(TIMER3);

        timer_channel_output_config(TIMER3, TIMER_CH_1, &timer_ocintpara);
        timer_channel_output_pulse_value_config(TIMER3, TIMER_CH_1, timer_initpara.period/2);
        timer_channel_output_mode_config(TIMER3, TIMER_CH_1, TIMER_OC_MODE_PWM0);
        timer_channel_output_shadow_config(TIMER3, TIMER_CH_1, TIMER_OC_SHADOW_DISABLE);
        timer_auto_reload_shadow_enable(TIMER3);
        timer_enable(TIMER3);
    }
}

/*!
    \brief      deinitialize user state and associated variables
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usbh_user_deinit(void)
{
    usbh_usr_application_state = USBH_USR_FS_INIT;
}

/*!
    \brief      user operation for device attached
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usbh_user_device_connected(void)
{    
}

/*!
    \brief      user operation when unrecovered error happens
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usbh_user_unrecovered_error (void)
{
    run_application();
}

/*!
    \brief      user operation for device disconnect event
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usbh_user_device_disconnected (void)
{
    Lcd_Put_Text(10, 10+18*3, 16, "USB Flash disconnected!", YELLOW);
    delay_1ms(500);

    run_application();
}

/*!
    \brief      user operation for reset USB Device
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usbh_user_device_reset(void)
{
}

/*!
    \brief      user operation for detecting device speed
    \param[in]  device_speed: device speed
    \param[out] none
    \retval     none
*/
void usbh_user_device_speed_detected(uint32_t device_speed)
{
    if(PORT_SPEED_HIGH == device_speed){
        //"High speed device detected"
    }else if(PORT_SPEED_FULL == device_speed){
        //"Full speed device detected"
    }else if(PORT_SPEED_LOW == device_speed){
        //"Low speed device detected"
    }else{
        //"Device Fault"
    }
}

/*!
    \brief      user operation when device descriptor is available
    \param[in]  device_desc: device descriptor
    \param[out] none
    \retval     none
*/
void usbh_user_device_desc_available(void *device_desc)
{
    usb_desc_dev *pDevStr = device_desc;
    (void)pDevStr;
}

/*!
    \brief      usb device is successfully assigned the address 
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usbh_user_device_address_assigned(void)
{
}

/*!
    \brief      user operation when configuration descriptor is available
    \param[in]  cfg_desc: pointer to configuration descriptor
    \param[in]  itf_desc: pointer to interface descriptor
    \param[in]  ep_desc: pointer to endpoint descriptor
    \param[out] none
    \retval     none
*/
void usbh_user_configuration_descavailable(usb_desc_config *cfg_desc,
                                           usb_desc_itf *itf_desc,
                                           usb_desc_ep *ep_desc)
{
    usb_desc_itf *id = itf_desc;

    if(0x08U  == (*id).bInterfaceClass){
        //"Mass storage device connected"
    }else if (0x03U  == (*id).bInterfaceClass){
        //"HID device connected"
    }
}

/*!
    \brief      user operation when manufacturer string exists
    \param[in]  manufacturer_string: manufacturer string of usb device
    \param[out] none
    \retval     none
*/
void usbh_user_manufacturer_string(void *manufacturer_string)
{
}

/*!
    \brief      user operation when product string exists
    \param[in]  product_string: product string of usb device
    \param[out] none
    \retval     none
*/
void usbh_user_product_string(void *product_string)
{
}

/*!
    \brief      user operation when serialNum string exists
    \param[in]  serial_num_string: serialNum string of usb device
    \param[out] none
    \retval     none
*/
void usbh_user_serialnum_string(void *serial_num_string)
{
}

/*!
    \brief      user response request is displayed to ask for application jump to class
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usbh_user_enumeration_finish(void)
{    
}

/*!
    \brief      user operation when device is not supported
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usbh_user_device_not_supported(void)
{
    run_application();
}

/*!
    \brief      user action for application state entry
    \param[in]  none
    \param[out] none
    \retval     user response for user key
*/
usbh_user_status usbh_user_userinput(void)
{
    return USR_IN_RESP_OK;
}

/*!
    \brief      user operation for device over current detection event
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usbh_user_over_current_detected (void)
{
}

static void run_application(void)
{
    typedef void (*pFunction)(void); 

    uint32_t JumpAddress = *(__IO uint32_t*)(APP1_START + 4);
    pFunction Jump       = (pFunction)JumpAddress;

    __disable_irq();

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    //Отключение всей ранее настроенной переферии
    //Чтобы основная прошивка принялая после нас процессор "как новый"
    usbh_user_deinit();
    nvic_irq_disable(USBFS_IRQn);
    nvic_irq_disable(USBFS_WKUP_IRQn);
    nvic_irq_disable(TIMER2_IRQn);
    timer_deinit(TIMER3);
    timer_deinit(TIMER2);
    usbh_deinit(&usb_host);
    rcu_periph_clock_disable(RCU_USBFS);
    rcu_periph_clock_disable(RCU_TIMER2);
    rcu_periph_clock_disable(RCU_TIMER3);
    rcu_deinit();

     SCB->VTOR = APP1_START;//SET_VECTOR_TABLE
    __set_MSP(*(__IO uint32_t*)APP1_START);

    //Прерывания в "новом" процессоре по умолчанию включены.
    __enable_irq();
    Jump();
}

/*!
    \brief      demo application for mass storage
    \param[in]  none
    \param[out] none
    \retval     status
*/
int usbh_usr_msc_application(void)
{
    static uint32_t buffer_offset;
    static char str_buff[32];

    switch(usbh_usr_application_state){
    case USBH_USR_FS_INIT:
        /* initializes the file system*/
        if (FR_OK != f_mount(&fatfs, "0:/", 0)) {
            goto start;
        }
        usbh_usr_application_state = USBH_USR_FS_OPEN_WIRMWARE;
        break;

    case USBH_USR_FS_OPEN_WIRMWARE:
        if (f_open(&file, "0:/MagnumDF.bin", FA_READ) != FR_OK || f_size(&file)<127) {
            Lcd_Put_Text(10, 10+18, 16, "No firmware file on USB!", YELLOW);
            delay_1ms(500);
            goto start;
        }
        offset = 0;
        if (FMC_CTL0 & FMC_CTL0_LK) {
            /* write the FMC unlock key */
            FMC_KEY0 = UNLOCK_KEY0;
            FMC_KEY0 = UNLOCK_KEY1;
        }
        Lcd_Fill_Rect(10, 10, 200, 10+18, BLACK);
        Lcd_Put_Text(10, 10, 16, "Flashing:", WHITE);
        usbh_usr_application_state = USBH_USR_FS_READ_BUFFER;
        break;
    
    case USBH_USR_FS_READ_BUFFER: {
        UINT *readed = (UINT *)buffer;
        *readed = buffer_offset = 0;
        if ((f_read(&file, buffer+sizeof(UINT), sizeof(buffer)-sizeof(UINT), readed) == FR_OK) && (*readed>0)) {
            uint32_t block = (APP1_START+offset+*readed)/BLOCK_SIZE;
            if (!offset || ((APP1_START+offset)/BLOCK_SIZE<block)) {
                FMC_CTL0 |= FMC_CTL0_PER;
                FMC_ADDR0 = block*BLOCK_SIZE;
                FMC_CTL0 |= FMC_CTL0_START;

                usbh_usr_application_state = USBH_USR_FS_WAIT_ERASE;
                break;
            }
            usbh_usr_application_state = USBH_USR_FS_WRITE_BUFFER;
            break;
        }
        Lcd_Put_Text(10, 10+18+18+18, 16, "DONE", WHITE);
        delay_1ms(300);
        goto start;
    }
    case USBH_USR_FS_WAIT_ERASE:
        if (!(FMC_STAT0 & FMC_STAT0_BUSY)) {
            FMC_CTL0 &= ~FMC_CTL0_PER;
            usbh_usr_application_state = USBH_USR_FS_WRITE_BUFFER;
        }
        break;
    
    case USBH_USR_FS_WRITE_BUFFER:
        FMC_CTL0 |= FMC_CTL0_PG;
        REG32(APP1_START+offset+buffer_offset) = *((uint32_t *)((uint8_t *)buffer+sizeof(UINT)+buffer_offset));
        /* wait for the FMC ready */
        usbh_usr_application_state = USBH_USR_FS_WAIT_WRITE;
        break;

    case USBH_USR_FS_WAIT_WRITE:
        if (!(FMC_STAT0 & FMC_STAT0_BUSY)) {
            buffer_offset+=sizeof(uint32_t);
            /* reset the PG bit */
            FMC_CTL0 &= ~FMC_CTL0_PG;

            uint32_t *readed = (uint32_t *)buffer;
            if (buffer_offset>=*readed) {
                offset += *readed;

                // strcpy((char *)str_buff, "Progress: ");
                // uint32_t pos = (100UL*offset)/f_size(&file);
                // itoa(pos, (char *)str_buff+strlen((char *)str_buff), 10);
                // strcat((char *)str_buff, "%");
                snprintf(str_buff, sizeof(str_buff), "Progress: %u%%", (unsigned int)((100U*offset)/f_size(&file)));
                Lcd_Fill_Rect(10, 10+18, 150, 10+18+16+1, BLACK);
                Lcd_Put_Text(10, 10+18, 16, (char *)str_buff, WHITE);
                Lcd_Fill_Rect(10, 10+18+18, 150, 10+18+16+1, BLACK);

                uint32_t pos = (30UL*offset)/f_size(&file);
                str_buff[0] = str_buff[30] = '|';
                str_buff[31] = 0;
                for (uint_fast8_t i=1; i<30; ++i) {
                    str_buff[i] = (i<pos)?'=':' ';
                }
                Lcd_Put_Text(10, 10+18+18, 16, str_buff, WHITE);

                usbh_usr_application_state = USBH_USR_FS_READ_BUFFER;
            } else
                usbh_usr_application_state = USBH_USR_FS_WRITE_BUFFER;
        }
        break;

    default:    
        goto start;
    }

    return(0);
start:
    FMC_CTL0 |= FMC_CTL0_LK;
    run_application();
    return(-1);
}

static void LCD_GPIO_Config(void)
{
    // fsmc 16bit data pins
    gpio_init(GPIOD, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_0);
    gpio_init(GPIOD, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_1);
    gpio_init(GPIOD, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_8);
    gpio_init(GPIOD, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);
    gpio_init(GPIOD, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_10);
    gpio_init(GPIOD, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_14);
    gpio_init(GPIOD, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_15);
    gpio_init(GPIOE, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_7);
    gpio_init(GPIOE, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_8);
    gpio_init(GPIOE, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);
    gpio_init(GPIOE, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_10);
    gpio_init(GPIOE, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_11);
    gpio_init(GPIOE, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_12);
    gpio_init(GPIOE, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_13);
    gpio_init(GPIOE, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_14);
    gpio_init(GPIOE, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_15);

    /*Configure the control line corresponding to FSMC
     * PD4-FSMC_NOE :LCD-RD
     * PD5-FSMC_NWE :LCD-WR
     * PD7-FSMC_NE1 :LCD-CS
     * PE2-FSMC_A23 :LCD-RS   LCD-RS data or cmd
     */
    gpio_init(GPIOD, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_4);
    gpio_init(GPIOD, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5);
    gpio_init(GPIOD, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_7);
    gpio_init(GPIOE, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2);
}

static void GD_LCD_EXMC_Config(void)
{
    exmc_norsram_parameter_struct EXMC_NORSRAMInitStructure;
    exmc_norsram_timing_parameter_struct readWriteTiming, writeTiming;

    rcu_periph_clock_enable(RCU_EXMC);

    /* EXMC configuration */
    readWriteTiming.asyn_address_setuptime = 1U;
    readWriteTiming.asyn_address_holdtime = 0U;
    readWriteTiming.asyn_data_setuptime = 15U;
    readWriteTiming.bus_latency = 0U;
    readWriteTiming.syn_clk_division = 0U;
    readWriteTiming.syn_data_latency = 0U;
    readWriteTiming.asyn_access_mode = EXMC_ACCESS_MODE_A;

    writeTiming.asyn_address_setuptime = 1U;
    writeTiming.asyn_address_holdtime = 0U;
    writeTiming.asyn_data_setuptime = 0x03;
    writeTiming.bus_latency = 0U;
    writeTiming.syn_clk_division = 0U;
    writeTiming.syn_data_latency = 0U;
    writeTiming.asyn_access_mode = EXMC_ACCESS_MODE_A;

    EXMC_NORSRAMInitStructure.norsram_region = EXMC_BANK0_NORSRAM_REGION0;
    EXMC_NORSRAMInitStructure.write_mode = EXMC_ASYN_WRITE;
    EXMC_NORSRAMInitStructure.extended_mode = DISABLE;
    EXMC_NORSRAMInitStructure.asyn_wait = DISABLE;
    EXMC_NORSRAMInitStructure.nwait_signal = DISABLE;
    EXMC_NORSRAMInitStructure.memory_write = ENABLE;
    EXMC_NORSRAMInitStructure.nwait_config = EXMC_NWAIT_CONFIG_BEFORE;
    EXMC_NORSRAMInitStructure.wrap_burst_mode = DISABLE;
    EXMC_NORSRAMInitStructure.nwait_polarity = EXMC_NWAIT_POLARITY_LOW;
    EXMC_NORSRAMInitStructure.burst_mode = DISABLE;
    EXMC_NORSRAMInitStructure.databus_width = EXMC_NOR_DATABUS_WIDTH_16B;
    EXMC_NORSRAMInitStructure.memory_type = EXMC_MEMORY_TYPE_NOR;
    EXMC_NORSRAMInitStructure.address_data_mux = DISABLE;
    EXMC_NORSRAMInitStructure.read_write_timing = &readWriteTiming;
    EXMC_NORSRAMInitStructure.write_timing = &writeTiming;

    exmc_norsram_init(&EXMC_NORSRAMInitStructure);
    /* enable EXMC SARM bank0 */
    exmc_norsram_enable(EXMC_BANK0_NORSRAM_REGION0);
}