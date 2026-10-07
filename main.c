/******************************************************************************
* File Name:   main.c
*
* Description: This code example measures and displays the pulse width
*              of an external input signal using the capture function
*              for the TRAVEO™ T2G family.
*
* Related Document: See README.md
*
*
*******************************************************************************
* (c) 2026, Infineon Technologies AG, or an affiliate of Infineon
* Technologies AG. All rights reserved.
* This software, associated documentation and materials ("Software") is
* owned by Infineon Technologies AG or one of its affiliates ("Infineon")
* and is protected by and subject to worldwide patent protection, worldwide
* copyright laws, and international treaty provisions. Therefore, you may use
* this Software only as provided in the license agreement accompanying the
* software package from which you obtained this Software. If no license
* agreement applies, then any use, reproduction, modification, translation, or
* compilation of this Software is prohibited without the express written
* permission of Infineon.
*
* Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
* IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
* THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
* SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
* Infineon reserves the right to make changes to the Software without notice.
* You are responsible for properly designing, programming, and testing the
* functionality and safety of your intended application of the Software, as
* well as complying with any legal requirements related to its use. Infineon
* does not guarantee that the Software will be free from intrusion, data theft
* or loss, or other breaches ("Security Breaches"), and Infineon shall have
* no liability arising out of any Security Breaches. Unless otherwise
* explicitly approved by Infineon, the Software may not be used in any
* application where a failure of the Product or any consequences of the use
* thereof can reasonably be expected to result in personal injury.
*******************************************************************************/

#include "cy_pdl.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "mtb_hal.h"

/* For the Retarget-IO (Debug UART) usage */
static cy_stc_scb_uart_context_t    UART_context;          /** UART context */
static mtb_hal_uart_t               UART_hal_obj;          /** Debug UART HAL object */

/*******************************************************************************
* Macros
********************************************************************************/
/* Flag control */
#define ONE_SECOND            ((TCPWM_COUNTER_config.period + 1) / 10)

/*******************************************************************************
* Global Variables
********************************************************************************/
/* Interrupt configuration */
const cy_stc_sysint_t IRQ_CFG =
{
    .intrSrc = ((NvicMux3_IRQn << CY_SYSINT_INTRSRC_MUXIRQ_SHIFT) | TCPWM_COUNTER_IRQ),
    .intrPriority = 2UL
};

/*******************************************************************************
* Function Prototypes
********************************************************************************/
void handle_TCPWM_IRQ(void)
{
    /* Get interrupt source */
    uint32_t intrMask = Cy_TCPWM_GetInterruptStatusMasked(TCPWM_COUNTER_HW, TCPWM_COUNTER_NUM);

    /* Clear interrupt source */
    Cy_TCPWM_ClearInterrupt(TCPWM_COUNTER_HW, TCPWM_COUNTER_NUM, intrMask);

    /* Rising edge capture */
    if (0UL != (CY_TCPWM_INT_ON_CC0 & intrMask))
    {
        uint32_t capturedCounter = Cy_TCPWM_Counter_GetCapture0Val(TCPWM_COUNTER_HW, TCPWM_COUNTER_NUM);
        uint32_t duration = TCPWM_COUNTER_config.period - capturedCounter;
        if (duration < ONE_SECOND)
        {
            printf("********************************************** Pushdown time = %u ms  \r\n", (unsigned int)duration);
        }
        else
        {
            printf("********************************************** Pushdown time = %u.%03u s  \r\n"
            , (unsigned int)(duration / 1000), (unsigned int)(duration % 1000));
        }
    }
    /* Underflow */
    else if (0UL != (CY_TCPWM_INT_ON_TC & intrMask))
    {
        printf("############################################## Counter has been stopped.\r\n");
    }
}


/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
* This is the main function for CPU. It...
*    1.TCPWM interrupt handler function.
*    2.Checks interrupt status and prints the information out to terminal.
*
* Parameters:
*  void
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    cy_rslt_t result;

    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Board init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Enable global interrupts */
    __enable_irq();

    /* Debug UART init */
    result = (cy_rslt_t)Cy_SCB_UART_Init(UART_HW, &UART_config, &UART_context);

    /* UART init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    Cy_SCB_UART_Enable(UART_HW);

    /* Setup the HAL UART */
    result = mtb_hal_uart_setup(&UART_hal_obj, &UART_hal_config, &UART_context, NULL);

    /* HAL UART init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    result = cy_retarget_io_init(&UART_hal_obj);

    /* HAL retarget_io init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }


    /* retarget-io init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");
    printf("****************** "
           "TCPWM Counter Capture Functionality "
           "****************** \r\n");

    /*TCPWM Counter Mode initial*/
    if (CY_TCPWM_SUCCESS != Cy_TCPWM_Counter_Init(TCPWM_COUNTER_HW, TCPWM_COUNTER_NUM, &TCPWM_COUNTER_config))
    {
        CY_ASSERT(0);
    }

    /* Interrupt settings */
    Cy_SysInt_Init(&IRQ_CFG, &handle_TCPWM_IRQ);
    NVIC_SetPriority((IRQn_Type) NvicMux3_IRQn, 2UL);
    NVIC_EnableIRQ((IRQn_Type) NvicMux3_IRQn);

    /* Enable the initialized counter */
    Cy_TCPWM_Counter_Enable(TCPWM_COUNTER_HW, TCPWM_COUNTER_NUM);

    printf(" The time it takes to press and release user button is displayed below. \r\n");
    printf(" 10 seconds after pressed, the measurement will stop. \r\n\n");

    for (;;)
    {
    }
}

/* [] END OF FILE */