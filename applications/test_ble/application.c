/**
  *********************************************************************************
  *
  * @file    applications.c
  * @brief   Main start
  * @author  yuzrain
  * @note    The main() function is not valid here when using the GCC compiler
  *
  *********************************************************************************
  */

/**
 * @addtogroup NRF52832
 */
/*@{*/
#include <rtthread.h>
#include <rtdevice.h>
#include <rthw.h>


#ifdef __GNUC__

void main_tm_task(void *parameter)
{

}

void _main(void)
{
	uint8_t flag=0;
	rt_pin_mode(17, PIN_MODE_OUTPUT);
	rt_pin_mode(18, PIN_MODE_OUTPUT);
	rt_pin_mode(19, PIN_MODE_OUTPUT);
	rt_pin_mode(20, PIN_MODE_OUTPUT);
	//rt_timer_create("main_tm", main_tm_task, NULL, 100, RT_TIMER_FLAG_PERIODIC|RT_TIMER_FLAG_SOFT_TIMER);
	while (1)
	{
		rt_thread_delay(100);
		if (flag == 0)
		{
			rt_pin_write(17, 1);
			rt_pin_write(18, 1);
			rt_pin_write(19, 1);
			rt_pin_write(20, 1);
			flag = 1;
		}
		else
		{
			rt_pin_write(17, 0);
			rt_pin_write(18, 0);
			rt_pin_write(19, 0);
			rt_pin_write(20, 0);
			flag = 0;
		}
	}
}

#else

int main(void)
{
    rt_kprintf("hello world \r\n");
}

#endif

/*@}*/
