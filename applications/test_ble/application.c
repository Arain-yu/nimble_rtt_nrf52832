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

#ifdef __GNUC__

	/* We do nothing */

#else

int main(void)
{
    rt_kprintf("hello world \r\n");
}

#endif

/*@}*/
