// #include <stdint.h>
// #include <libopencm3/stm32/rcc.h>
// #include <libopencm3/stm32/gpio.h>
// #include <libopencm3/stm32/usart.h>

///* This is the original retarget.c but with code filled in for
// * the STM32F4 and the Butterfly board.
// */
//int _write (int fd, char *ptr, int len)
//{
//  /* Write "len" of char from "ptr" to file id "fd"
//   * Return number of char written.
//   * Need implementing with UART here. */
//    int i = 0;
//    while (*ptr && (i < len)) {
//        usart_send_blocking(USART1, *ptr);
//        if (*ptr == '\n') {
//            usart_send_blocking(USART1, '\r');
//        }
//        i++;
//        ptr++;
//    }
//  return i;
//}
//
//int _read (int fd, char *ptr, int len)
//{
//  /* not used in this example */
//  *ptr = usart_recv_blocking(USART1);
//  return 1;
//}
//
//void _ttywrch(int ch) {
//  /* Write one char "ch" to the default console
//   * Need implementing with UART here. */
//	usart_send_blocking(USART1, ch);
//}
//
//
//void retarget_init()
//{
//  // Initialize UART
///* *** Added Code Begin *** */
//    /* MUST enable the GPIO clock in ADDITION to the USART clock */
//    rcc_periph_clock_enable(RCC_GPIOA);
//
//
//	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9 | GPIO10);
//	gpio_set_af(GPIOA, GPIO_AF7, GPIO9 | GPIO10);
//
//
//    /* This then enables the clock to the USART1 peripheral which is
//     * attached inside the chip to the APB2 bus. Different peripherals
//     * attach to different buses, and even some UARTS are attached to
//     * APB1 and some to APB2, again the data sheet is useful here.
//     */
//    rcc_periph_clock_enable(RCC_USART1);
//
//    /* Set up USART/UART parameters using the libopencm3 helper functions */
//    usart_set_baudrate(USART1, 115200);
//    usart_set_databits(USART1, 8);
//    usart_set_stopbits(USART1, USART_STOPBITS_1);
//    usart_set_mode(USART1, USART_MODE_TX_RX);
//    usart_set_parity(USART1, USART_PARITY_NONE);
//    usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
//    usart_enable(USART1);
//
///* *** Added Code End *** */
//
//}