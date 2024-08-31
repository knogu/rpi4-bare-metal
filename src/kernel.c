/* Makefile's CFLAGS will take care of the include paths */
#include "mini_uart.h"
#include "utils.h"
#include "irq_numbers.h"
#include "irq.h"
#include "generic_timer.h"
#include "fork.h"
#include "sched.h"
#include "mm.h"
#include "sys.h"

#include "trace/trace_main.h"
#include "trace/pl011_uart.h"
#include "trace/ksyms.h"
#include "trace/utils.h"
#include "enc28j60.h"

#include "user_space/init.h"

void *memset(void *dest, unsigned char val, unsigned short len)
{
    uint8_t *ptr = dest;
    while (len-- > 0)
        *ptr++ = val;
    return dest;
}

//void *memcpy(void *dest, const void *src, unsigned short len)
//{
//    uint8_t *d = dest;
//    const uint8_t *s = src;
//    while (len--)
//        *d++ = *s++;
//    return dest;
//}

uint8_t memcmp(void *str1, void *str2, unsigned count)
{
    uint8_t *s1 = str1;
    uint8_t *s2 = str2;

    while (count-- > 0)
    {
        if (*s1++ != *s2++)
            return s1[-1] < s2[-1] ? -1 : 1;
    }

    return 0;
}

int strncmp(const char *s1, const char *s2, unsigned short n)
{
    unsigned char u1, u2;

    while (n-- > 0)
    {
        u1 = (unsigned char) *s1++;
        u2 = (unsigned char) *s2++;
        if (u1 != u2) return u1 - u2;
        if (u1 == '\0') return 0;
    }

    return 0;
}

reg32 state;

/* run by pid 1, will ret back to entry.S and do a kernel_exit 0 */
void kernel_process()
{
    main_output(MU, "pid 1 started in EL");
    main_output_char(MU, get_el() + '0');
    main_output(MU, "\n");

    extern u64 user_start;
    extern u64 user_end;
    u64 user_size = ((u64)(&user_end)) - ((u64)(&user_start));

    int err = prepare_move_to_user((u64)&user_start, user_size, ((u64)user_process) - (u64)&user_start);
    if (err < 0)
        main_output(MU, "Failed to move to user mode!\n");
}

// NETWORKING GLOBALS AND FUNCTIONS

ENC_HandleTypeDef handle;

// MAC address to be assigned to the ENC28J60
unsigned char myMAC[6] = { 0xc0, 0xff, 0xee, 0xc0, 0xff, 0xee };

// IP address to be assigned to the ENC28J60
unsigned char deviceIP[4] = { 192, 168, 0, 66 };

void init_network(void)
{
   handle.Init.DuplexMode = ETH_MODE_HALFDUPLEX;
   handle.Init.MACAddr = myMAC;
   handle.Init.ChecksumMode = ETH_CHECKSUM_BY_HARDWARE;
   handle.Init.InterruptEnableBits = EIE_LINKIE | EIE_PKTIE;

   main_output(MU, "Starting network up.");
//   debugcrlf();
   if (!ENC_Start(&handle)) {
      main_output(MU, "Could not initialise network card.");
   } else {
      main_output(MU, "Setting MAC address to C0:FF:EE:C0:FF:EE.");
//      debugcrlf();

      ENC_SetMacAddr(&handle);

      main_output(MU, "Network card successfully initialised.");
   }
//   debugcrlf();

   main_output(MU, "Waiting for ifup... ");
   while (!(handle.LinkStatus & PHSTAT2_LSTAT)) ENC_IRQHandler(&handle);
   main_output(MU, "done.");
//   debugcrlf();

   // Re-enable global interrupts
   ENC_EnableInterrupts(EIE_INTIE);

   main_output(MU, "Initialising the TCP stack... ");
   init_udp_or_www_server(myMAC, deviceIP);
   main_output(MU, "done.");
//   debugcrlf();
}

void enc28j60PacketSend(unsigned short buflen, void *buffer) {
    if (ENC_RestoreTXBuffer(&handle, buflen) == 0) {
        ENC_WriteBuffer((unsigned char *) buffer, buflen);
        handle.transmitLength = buflen;
        ENC_Transmit(&handle);
    }
}

void serve(void)
{
    while (1) {
        while (!ENC_GetReceivedFrame(&handle));

        uint8_t *buf = (uint8_t *)handle.RxFrameInfos.buffer;
        uint16_t len = handle.RxFrameInfos.length;
        uint16_t dat_p = packetloop_arp_icmp_tcp(buf, len);

        if (dat_p != 0) {
            main_output(MU, "Incoming web request... ");

            if (strncmp("GET ", (char *)&(buf[dat_p]), 4) != 0) {
                main_output(MU, "not GET");
                dat_p = fill_tcp_data(buf, 0, "HTTP/1.0 401 Unauthorized\r\nContent-Type: text/html\r\n\r\n<h1>ERROR</h1>");
            } else if (strncmp("echo ", (char *)&(buf[dat_p]), 5) != 0) {
                dat_p = fill_tcp_data(buf, 0, (char *)&buf[dat_p + 5]);
            } else {
                if (strncmp("/ ", (char *)&(buf[dat_p+4]), 2) == 0) {
                    // just one web page in the "root directory" of the web server
                    main_output(MU, "GET root");
                    dat_p = fill_tcp_data(buf, 0, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>Hello world!</h1>");
                } else {
                    // just one web page not in the "root directory" of the web server
                    main_output(MU, "GET not root");
                    dat_p = fill_tcp_data(buf, 0, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>Goodbye cruel world.</h1>");
                }
            }

            www_server_reply(buf, dat_p); // send web page data
//            debugcrlf();
        }
    }
}

void test() {
    main_output(MU, ")\n");
}
void kernel_main(u64 id)
{
    /* core 0 initializes mini-uart and handles uart interrupts */
    if (id == 0) {
        mini_uart_init();
       pl011_uart_init();
        // enable_interrupt_gic(VC_AUX_IRQ, id);
//        ksyms_init();
        sys_call_table_relocate();
//        trace_init();
//        trace_output_kernel_pts(PL);
        state = 0;
    }

    /* single core for now */
    while (id != 0) {}

    /* output startup message and EL */
    main_output(MU, "Bare Metal... (core ");
    main_output_char(MU, id + '0');
    main_output(MU, ")\n");
    delay(30000);
    main_output(MU, "EL: ");
    main_output_char(MU, get_el() + '0');
    main_output(MU, "\n");
    /* also output the syscount */
    u64 sys_count = get_sys_count();
    main_output_u64(MU, sys_count);
    main_output(MU, "\n");


    // spi_init();
    main_output(MU, "spi init done");
    init_network();
    serve();

    /* initialize exception vectors and timers and the timer interrupt */
    irq_init_vectors();
    generic_timer_init();
    enable_interrupt_gic(NS_PHYS_TIMER_IRQ, id);
    irq_enable();

    /* let the next core run */
    state++;

    while (1) {
        if (id != 0 || state != 1)
            continue;
        sched_init();
        main_output_process(MU, current);
        /* create pid 1, kernel threads don't need a user stack page */
        int res = copy_process(KTHREAD, (u64)&kernel_process, 0);
        if (res <= 0) {
            main_output(MU, "fork error\n");
        }
        while (1) {
            main_output(MU, "init schedule..\n");
            schedule();
        }
    }
}

