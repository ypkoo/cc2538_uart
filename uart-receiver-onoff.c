#include "contiki.h"
#include "cpu.h"
#include "sys/etimer.h"
#include "sys/rtimer.h"
#include "dev/leds.h"
#include "dev/uart.h"
#include "dev/button-sensor.h"
#include "dev/watchdog.h"
#include "dev/serial-line.h"
#include "dev/sys-ctrl.h"
#include "net/rime/broadcast.h"
#include "../cpu/cc2538/dev/gpio.h"
#include "net/netstack.h"

#include <stdio.h>
#include <stdint.h>
/*---------------------------------------------------------------------------*/
#define LOOP_INTERVAL       CLOCK_SECOND
#define LEDS_OFF_HYSTERISIS (RTIMER_SECOND >> 1)
#define LEDS_PERIODIC       LEDS_YELLOW
#define LEDS_BUTTON         LEDS_RED
#define LEDS_SERIAL_IN      LEDS_ORANGE
#define LEDS_REBOOT         LEDS_ALL
#define LEDS_RF_RX          (LEDS_YELLOW | LEDS_ORANGE)
#define BROADCAST_CHANNEL   129
/*---------------------------------------------------------------------------*/
static struct etimer et;
static struct rtimer rt;
static uint8_t flag = 0;
/*---------------------------------------------------------------------------*/
PROCESS(cc2538_demo_process, "cc2538 demo process");
AUTOSTART_PROCESSES(&cc2538_demo_process);
/*---------------------------------------------------------------------------*/
static void
broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *from)
{
  char *msg = packetbuf_dataptr();

  if (msg[0] == 'S' )
  {
    //check flag first
    NETSTACK_RADIO.off();
    GPIO_SET_PIN(GPIO_B_BASE, 0x08);
    flag = 1;
  }
}
/*---------------------------------------------------------------------------*/
static const struct broadcast_callbacks bc_rx = { broadcast_recv };
static struct broadcast_conn bc;
/*---------------------------------------------------------------------------*/
void
rt_callback(struct rtimer *t, void *ptr)
{
  leds_off(LEDS_PERIODIC);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(cc2538_demo_process, ev, data)
{

  PROCESS_EXITHANDLER(broadcast_close(&bc))

  PROCESS_BEGIN();
  //gpio_init();

  GPIO_SET_OUTPUT(GPIO_B_BASE, 0x08);
  GPIO_CLR_PIN(GPIO_B_BASE, 0x08);
  broadcast_open(&bc, BROADCAST_CHANNEL, &bc_rx);

  while(1) {
    etimer_set(&et, CLOCK_SECOND);

    PROCESS_YIELD();

    if(flag == 1)
    {
      etimer_set(&et, CLOCK_SECOND*3);
      PROCESS_YIELD();
      printf("S");
      flag = 0;
    }

    if(ev == serial_line_event_message)
    {
      NETSTACK_RADIO.on();
      packetbuf_copyfrom(data, strlen(data));
      broadcast_send(&bc);
      GPIO_CLR_PIN(GPIO_B_BASE, 0x08);
    }
  }

  PROCESS_END();
}
