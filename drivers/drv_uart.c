/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <rtthread.h>
#include <rtdevice.h>

#include "board.h"
#include "drv_uart.h"

#include "nrf.h"
#include "nrf_gpio.h"
#include "nrfx_uart.h"

static struct rt_serial_device _serial0_0;
#if USE_UART0_1
static struct rt_serial_device _serial0_1;
#endif

typedef struct
{
    struct rt_serial_device *serial;
    nrfx_uart_t uart;
    uint32_t rx_pin;
    uint32_t tx_pin;
} UART_CFG_T;

UART_CFG_T uart0 =
{
    .uart = {.p_reg = NRF_UART0, .drv_inst_idx = 0},
#ifdef RT_USING_CONSOLE
    .rx_pin = 8,
    .tx_pin = 6
#else
    .rx_pin = 3,
    .tx_pin = 4
#endif
};

#if USE_UART0_1
UART_CFG_T uart1 =
{
    .uart = NRF_DRV_UART_INSTANCE(0),
    .rx_pin = 3,
    .tx_pin = 4
};
#endif

UART_CFG_T *working_cfg = RT_NULL;

void UART0_IRQHandler(void)
{
    if (nrf_uart_int_enable_check(NRF_UART0, NRF_UART_INT_MASK_ERROR)
            && nrf_uart_event_check(NRF_UART0, NRF_UART_EVENT_ERROR))
    {
        nrf_uart_event_clear(NRF_UART0, NRF_UART_EVENT_ERROR);
    }

    if (nrf_uart_int_enable_check(NRF_UART0, NRF_UART_INT_MASK_RXDRDY)
            && nrf_uart_event_check(NRF_UART0, NRF_UART_EVENT_RXDRDY))
    {
        rt_hw_serial_isr(working_cfg->serial, RT_SERIAL_EVENT_RX_IND);
    }

    if (nrf_uart_int_enable_check(NRF_UART0, NRF_UART_INT_MASK_TXDRDY)
            && nrf_uart_event_check(NRF_UART0, NRF_UART_EVENT_TXDRDY))
    {
        rt_hw_serial_isr(working_cfg->serial, RT_SERIAL_EVENT_TX_DONE);
    }

    if (nrf_uart_event_check(NRF_UART0, NRF_UART_EVENT_RXTO))
    {
        rt_hw_serial_isr(working_cfg->serial, RT_SERIAL_EVENT_RX_TIMEOUT);
    }
}

static rt_err_t _uart_cfg(struct rt_serial_device *serial, struct serial_configure *cfg)
{
    nrfx_uart_config_t config = NRFX_UART_DEFAULT_CONFIG;
    UART_CFG_T *instance = &uart0;

    RT_ASSERT(serial != RT_NULL);
    RT_ASSERT(cfg != RT_NULL);

    if (serial->parent.user_data != RT_NULL)
    {
        instance = (UART_CFG_T *)serial->parent.user_data;
    }

    nrf_uart_disable(instance->uart.p_reg);

    switch (cfg->baud_rate)
    {
    case 115200:
        config.baudrate = NRF_UART_BAUDRATE_115200;
        break;

    case 9600:
        config.baudrate = NRF_UART_BAUDRATE_9600;
        break;

    default:
        config.baudrate = NRF_UART_BAUDRATE_115200;
        break;
    }

    if (cfg->parity == PARITY_NONE)
    {
        config.parity = NRF_UART_PARITY_EXCLUDED;
    }
    else
    {
        config.parity = NRF_UART_PARITY_INCLUDED;
    }

    config.hwfc = NRF_UART_HWFC_DISABLED;
    config.interrupt_priority = 6;
    config.pselcts = 0;
    config.pselrts = 0;
    config.pselrxd = instance->rx_pin;
    config.pseltxd = instance->tx_pin;

    nrf_gpio_pin_set(config.pseltxd);
    nrf_gpio_cfg_output(config.pseltxd);
    nrf_gpio_pin_clear(config.pseltxd);
    nrf_gpio_cfg_input(config.pselrxd, NRF_GPIO_PIN_NOPULL);
    nrf_uart_baudrate_set(instance->uart.p_reg, config.baudrate);
    nrf_uart_configure(instance->uart.p_reg, config.parity, config.hwfc);
    nrf_uart_txrx_pins_set(instance->uart.p_reg, config.pseltxd, config.pselrxd);

    if (config.hwfc == NRF_UART_HWFC_ENABLED)
    {
        nrf_uart_hwfc_pins_set(instance->uart.p_reg, config.pselrts, config.pselcts);
    }

    nrf_uart_event_clear(instance->uart.p_reg, NRF_UART_EVENT_TXDRDY);
    nrf_uart_event_clear(instance->uart.p_reg, NRF_UART_EVENT_RXDRDY);
    nrf_uart_event_clear(instance->uart.p_reg, NRF_UART_EVENT_RXTO);
    nrf_uart_event_clear(instance->uart.p_reg, NRF_UART_EVENT_ERROR);

    nrf_uart_int_enable(instance->uart.p_reg, NRF_UART_INT_MASK_RXDRDY | NRF_UART_INT_MASK_RXTO | NRF_UART_INT_MASK_ERROR);
    NVIC_SetPriority(nrfx_get_irq_number((void *)instance->uart.p_reg), config.interrupt_priority);
    NVIC_EnableIRQ(nrfx_get_irq_number((void *)instance->uart.p_reg));
    nrf_uart_enable(instance->uart.p_reg);
    working_cfg = instance;
    return RT_EOK;
}

static rt_err_t _uart_ctrl(struct rt_serial_device *serial, int cmd, void *arg)
{
    UART_CFG_T *instance = working_cfg;

    RT_ASSERT(serial != RT_NULL);

    if (serial->parent.user_data != RT_NULL)
    {
        instance = (UART_CFG_T *)serial->parent.user_data;
    }

    switch (cmd)
    {
    /* disable interrupt */
    case RT_DEVICE_CTRL_CLR_INT:
        nrf_uart_task_trigger(instance->uart.p_reg, NRF_UART_TASK_STOPRX);
        nrf_uart_int_disable(instance->uart.p_reg, NRF_UART_INT_MASK_RXDRDY
                             | NRF_UART_INT_MASK_RXTO
                             | NRF_UART_INT_MASK_ERROR);
        NVIC_DisableIRQ(nrfx_get_irq_number((void *)instance->uart.p_reg));
        break;

    /* enable interrupt */
    case RT_DEVICE_CTRL_SET_INT:
        nrf_uart_event_clear(instance->uart.p_reg, NRF_UART_EVENT_RXDRDY);
        nrf_uart_event_clear(instance->uart.p_reg, NRF_UART_EVENT_RXTO);
        nrf_uart_event_clear(instance->uart.p_reg, NRF_UART_EVENT_ERROR);
        /* Enable RX interrupt. */
        nrf_uart_int_enable(instance->uart.p_reg, NRF_UART_INT_MASK_RXDRDY
                            | NRF_UART_INT_MASK_RXTO
                            | NRF_UART_INT_MASK_ERROR);
        NVIC_SetPriority(nrfx_get_irq_number((void *)instance->uart.p_reg), 6);
        NVIC_EnableIRQ(nrfx_get_irq_number((void *)instance->uart.p_reg));
        nrf_uart_task_trigger(instance->uart.p_reg, NRF_UART_TASK_STARTRX);
        break;

    case RT_DEVICE_CTRL_CUSTOM:
        if ((rt_uint32_t)(arg) == UART_CONFIG_BAUD_RATE_9600)
        {
            instance->serial->config.baud_rate = 9600;
            nrf_uart_baudrate_set(instance->uart.p_reg, NRF_UART_BAUDRATE_9600);
        }
        else if ((rt_uint32_t)(arg) == UART_CONFIG_BAUD_RATE_115200)
        {
            instance->serial->config.baud_rate = 115200;
            nrf_uart_baudrate_set(instance->uart.p_reg, NRF_UART_BAUDRATE_115200);
        }

        // _uart_cfg(instance->serial, &(instance->serial->config));
        // nrf_uart_task_trigger(instance->uart.reg.p_uart, NRF_UART_TASK_STARTRX);
        break;

    case RT_DEVICE_CTRL_PIN:
        if (working_cfg != instance)
        {
            _uart_cfg(instance->serial, &(instance->serial->config));
        }
        break;

    case RT_DEVICE_POWERSAVE:
        nrf_uart_disable(instance->uart.p_reg);
        nrf_uart_txrx_pins_disconnect(instance->uart.p_reg);
        nrf_gpio_pin_clear(instance->rx_pin);
        nrf_gpio_cfg_output(instance->rx_pin);
        nrf_gpio_pin_clear(instance->tx_pin);
        nrf_gpio_cfg_output(instance->tx_pin);
        break;

    case RT_DEVICE_WAKEUP:
        _uart_cfg(instance->serial, &(instance->serial->config));
        break;

    default:
        return RT_ERROR;
    }

    return RT_EOK;
}

static int _uart_putc(struct rt_serial_device *serial, char c)
{
    UART_CFG_T *instance = working_cfg;

    RT_ASSERT(serial != RT_NULL);

    if (serial->parent.user_data != RT_NULL)
    {
        instance = (UART_CFG_T *)serial->parent.user_data;
    }

    nrf_uart_event_clear(instance->uart.p_reg, NRF_UART_EVENT_TXDRDY);
    nrf_uart_task_trigger(instance->uart.p_reg, NRF_UART_TASK_STARTTX);
    nrf_uart_txd_set(instance->uart.p_reg, (uint8_t)c);
    while (!nrf_uart_event_check(instance->uart.p_reg, NRF_UART_EVENT_TXDRDY))
    {
    }

    return 1;
}

static int _uart_getc(struct rt_serial_device *serial)
{
    int ch = -1;
    UART_CFG_T *instance = working_cfg;

    RT_ASSERT(serial != RT_NULL);

    if (serial->parent.user_data != RT_NULL)
    {
        instance = (UART_CFG_T *)serial->parent.user_data;
    }

    if (nrf_uart_event_check(instance->uart.p_reg, NRF_UART_EVENT_RXDRDY))
    {
        nrf_uart_event_clear(instance->uart.p_reg, NRF_UART_EVENT_RXDRDY);
        ch = (int)(nrf_uart_rxd_get(instance->uart.p_reg));
    }

    return ch;
}

static struct rt_uart_ops _uart_ops =
{
    _uart_cfg,
    _uart_ctrl,
    _uart_putc,
    _uart_getc
};

int rt_hw_uart_init(void)
{
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;

    nrf_gpio_pin_write(uart0.tx_pin, 1);
    nrf_gpio_cfg_output(uart0.tx_pin);

    config.bufsz = RT_SERIAL_RB_BUFSZ;
    _serial0_0.config = config;
    _serial0_0.ops = &_uart_ops;
    uart0.serial = &_serial0_0;

    rt_hw_serial_register(&_serial0_0, "uart0", RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX, &uart0);

#if USE_UART0_1
    config.bufsz = UART0_RB_SIZE;
    _serial0_1.config = config;
    _serial0_1.ops = &_uart_ops;
    uart1.serial = &_serial0_1;
    rt_hw_serial_register(&_serial0_1, "uart1", RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX, &uart1);
#endif

    return 0;
}

INIT_BOARD_EXPORT(rt_hw_uart_init);
