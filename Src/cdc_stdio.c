#include "tusb.h"

#include "FreeRTOS.h"
#include "task.h"

static void cdc_putchar(uint8_t c)
{
    if (tud_cdc_write_available() > 0)
    {
        tud_cdc_write(&c, 1);
    }
}

int __io_putchar(int ch)
{
    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED)
    {
        return ch;
    }

    if (!tud_cdc_connected())
    {
        return ch;
    }

    if (ch == '\n')
    {
        cdc_putchar('\r');
    }

    cdc_putchar((uint8_t)ch);

    if (ch == '\n')
    {
        tud_cdc_write_flush();
    }

    return ch;
}

int __io_getchar(void)
{
    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED)
    {
        return -1;
    }

    if (!tud_cdc_connected())
    {
        return -1;
    }

    uint8_t c;

    if (tud_cdc_available() == 0)
    {
        return -1;
    }

    tud_cdc_read(&c, 1);
    return c;
}