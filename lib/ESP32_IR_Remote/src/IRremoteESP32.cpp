/*
* Copyright (c) 2019 Cristian Sandu
*
* Based on https://github.com/Darryl-Scott/ESP32-RMT-Library-IR-code-RAW
* Plus: https://github.com/espressif/esp-idf/blob/master/examples/peripherals/rmt_nec_tx_rx/main/infrared_nec_main.c
* https://github.com/pcbreflux/espressif/blob/master/esp32/arduino/sketchbook/ESP32_IR_Remote/ir_demo/ESP32_IR_Remote.cpp
* Also:
* https://github.com/cyborg5/IRLib2
*  
* A small class to send and receive NEC IR codes with the RMT peripheral on an ESP32, could be pushed to IRremote(or IRlib2)
*/

#include "Arduino.h"
#ifdef __cplusplus
extern "C"
{
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/ringbuf.h"

#ifdef __cplusplus
}
#endif

#include "IRremoteESP32.h"

#define roundTo 50                 //rounding microseconds timings
#define MARK_EXCESS 220            //tweeked to get the right timing
#define SPACE_EXCESS 190           //tweeked to get the right timing
#define rmt_item32_TIMEOUT_US 5000 // RMT receiver timeout duration_us(us)

// Clock divisor (base clock is 80MHz)
#define CLK_DIV 100

// Number of clock ticks that represent 10us.  10 us = 1/100th msec.
#define TICK_10_US (80000000 / CLK_DIV / 100000)

#define TOPBIT 0x80000000

#define NEC_HEADER_HIGH_US 9000       // NEC protocol header: positive 9ms
#define NEC_HEADER_LOW_US 4500        // NEC protocol header: negative 4.5m
#define NEC_HEADER_REPEAT_LOW_US 2200 // NEC protocol header, repeat: negative 2.2ms
#define NEC_HEADER_ITEM_COUNT 2

#define NEC_BIT_ONE_HIGH_US 560                           // NEC protocol data bit 1: positive 0.56ms
#define NEC_BIT_ONE_LOW_US (2250 - NEC_BIT_ONE_HIGH_US)   // NEC protocol data bit 1: negative 1.69ms
#define NEC_BIT_ZERO_HIGH_US 560                          // NEC protocol data bit 0: positive 0.56ms
#define NEC_BIT_ZERO_LOW_US (1120 - NEC_BIT_ZERO_HIGH_US) // NEC protocol data bit 0: negative 0.56ms
#define NEC_BIT_END_US 560                                // NEC protocol end: positive 0.56ms

#define NEC_ITEM_DURATION(d) ((d & 0x7fff) * 10 / TICK_10_US) // Parse duration time from memory register duration_us
#define NEC_DATA_ITEM_COUNT 34                                // NEC code item number: header + 32bit data + end
#define RMT_TX_DATA_NUM 100                                   // NEC tx test data number
#define rmt_item32_TIMEOUT_US 9500                            // RMT receiver timeout duration_us(us)

#define PERCENT_TOLERANCE 25
#define NEC_REPEAT_ITEM_COUNT 2

IRremoteESP32::IRremoteESP32()
{
}

void IRremoteESP32::setRecvPin(int recvpin, int port)
{
  if (recvpin >= GPIO_NUM_0 && recvpin < GPIO_NUM_MAX)
  {
    gpionum = recvpin;
  }
  else
  {
    gpionum = (int)GPIO_NUM_22;
  }
  if (port >= RMT_CHANNEL_0 && port < RMT_CHANNEL_MAX)
  {
    rmtport = port;
  }
  else
  {
    rmtport = (int)RMT_CHANNEL_0;
  }
}

void IRremoteESP32::setRecvPin(int recvpin)
{
  setRecvPin(recvpin, (int)RMT_CHANNEL_0);
}

void IRremoteESP32::setSendPin(int sendpin)
{
  setRecvPin(sendpin, (int)RMT_CHANNEL_0);
}

void IRremoteESP32::setSendPin(int sendpin, int port)
{

  if (sendpin >= GPIO_NUM_0 && sendpin < GPIO_NUM_MAX)
  {
    gpionum = sendpin;
  }
  else
  {
    gpionum = (int)GPIO_NUM_22;
  }
  if (port >= RMT_CHANNEL_0 && port < RMT_CHANNEL_MAX)
  {
    rmtport = port;
  }
  else
  {
    rmtport = (int)RMT_CHANNEL_0;
  }
}

void IRremoteESP32::initReceive()
{
  rmt_config_t config;
  config.rmt_mode = RMT_MODE_RX;
  config.channel = (rmt_channel_t)rmtport;
  config.gpio_num = (gpio_num_t)gpionum;
  gpio_pullup_en((gpio_num_t)gpionum);
  config.mem_block_num = 7; //how many memory blocks 64 x N (0-7)
  config.rx_config.filter_en = 1;
  config.rx_config.filter_ticks_thresh = 100; // 80000000/100 -> 800000 / 100 = 8000  = 125us
  config.rx_config.idle_threshold = TICK_10_US * 100 * 20;
  config.clk_div = CLK_DIV;
  ESP_ERROR_CHECK(rmt_config(&config));
  ESP_ERROR_CHECK(rmt_driver_install(config.channel, 1000, 0));
  rmt_rx_start(config.channel, 1);
}

void IRremoteESP32::initSend()
{
  rmt_config_t config;
  config.rmt_mode = RMT_MODE_TX;
  config.channel = (rmt_channel_t)rmtport;
  config.gpio_num = (gpio_num_t)gpionum;
  config.mem_block_num = 7; //how many memory blocks 64 x N (0-7)
  config.clk_div = CLK_DIV;
  config.tx_config.loop_en = false;
  config.tx_config.carrier_duty_percent = 50;
  config.tx_config.carrier_freq_hz = 38000;
  config.tx_config.carrier_level = RMT_CARRIER_LEVEL_HIGH;
  config.tx_config.carrier_en = true;
  config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
  config.tx_config.idle_output_en = true;
  rmt_config(&config);
  rmt_driver_install(config.channel, 0, 0); //19     /*!< RMT interrupt number, select from soc.h */
}

void IRremoteESP32::sendRAW(int *data, int IRlength)
{
  rmt_config_t config;
  config.channel = (rmt_channel_t)rmtport;
  //build item
  size_t size = (sizeof(rmt_item32_t) * IRlength);
  rmt_item32_t *item = (rmt_item32_t *)malloc(size); //allocate memory
  memset((void *)item, 0, size);                     //wipe current data in memory
  int i = 0;
  int x = 0;
  Serial.print("Sending.....:");
  while (x < IRlength)
  {
    Serial.print(data[x]);
    Serial.print(",");
    Serial.print(data[x + 1]);
    Serial.print(",");
    //    To build a series of waveforms.
    buildItem(item + i, data[x], -1 * data[x + 1]);
    x = x + 2;
    i++;
  }
  Serial.println();
  //To send data according to the waveform items.
  rmt_write_items(config.channel, item, IRlength, true);
  //Wait until sending is done.
  rmt_wait_tx_done(config.channel, 1);
  //before we free the data, make sure sending is already done.
  free(item);
}

void IRremoteESP32::sendNEC(const uint32_t &data)
{
  if (data == NEC_REPEAT_DATA)
  {
    // Send only the repeat header
    size_t size = sizeof(rmt_item32_t) * 2;
    rmt_item32_t *items = (rmt_item32_t *)malloc(size);
    memset((void *)items, 0, size);

    buildItem(items, NEC_HEADER_HIGH_US, NEC_HEADER_REPEAT_LOW_US);
    buildItem(items + 1, NEC_BIT_END_US, 0);

    sendRMT(items);

    return;
  }

  // Alloc send buffer
  size_t size = sizeof(rmt_item32_t) * NEC_DATA_ITEM_COUNT;
  rmt_item32_t *items = (rmt_item32_t *)malloc(size);
  memset((void *)items, 0, size);

  // Fill items
  size_t i = 1;
  buildHeaderItem(items);

  // TODO: should I smash the input data instead here?
  uint32_t dataToSend = data;
  for (; i < NEC_DATA_ITEM_COUNT - 1; i++, dataToSend <<= 1)
  {
    if (dataToSend & TOPBIT)
    {
      buildOneItem(items + i);
    }
    else
    {
      buildZeroItem(items + i);
    }
  }

  buildEndItem(items + i);

  sendRMT(items);
}

void IRremoteESP32::sendRMT(rmt_item32_t *items)
{
  // RMT send
  rmt_write_items((rmt_channel_t)rmtport, items, NEC_DATA_ITEM_COUNT, true);
  // Wait for send to finish
  rmt_wait_tx_done((rmt_channel_t)rmtport, 1);
  // Free memory when send is done
  free(items);
}

void IRremoteESP32::stopIR()
{
  rmt_config_t config;
  config.channel = (rmt_channel_t)rmtport;
  rmt_rx_stop(config.channel);
  Serial.print("Uninstalling..");
  Serial.print("Port : ");
  Serial.println(config.channel);
  rmt_driver_uninstall(config.channel);
}

void IRremoteESP32::buildItem(rmt_item32_t *item, int high_us, int low_us)
{
  item->level0 = true;
  item->duration0 = (high_us) / 10 * TICK_10_US;
  item->level1 = false;
  item->duration1 = (low_us) / 10 * TICK_10_US;
}

void IRremoteESP32::buildHeaderItem(rmt_item32_t *item)
{
  buildItem(item, NEC_HEADER_HIGH_US, NEC_HEADER_LOW_US);
}

void IRremoteESP32::buildOneItem(rmt_item32_t *item)
{
  buildItem(item, NEC_BIT_ONE_HIGH_US, NEC_BIT_ONE_LOW_US);
}

void IRremoteESP32::buildZeroItem(rmt_item32_t *item)
{
  buildItem(item, NEC_BIT_ZERO_HIGH_US, NEC_BIT_ZERO_LOW_US);
}

void IRremoteESP32::buildEndItem(rmt_item32_t *item)
{
  buildItem(item, NEC_BIT_END_US, 0x7fff);
}

bool IRremoteESP32::NEC_checkRange(int duration_ticks, int expected_us)
{
  const auto duration_us = NEC_ITEM_DURATION(duration_ticks);
  return (duration_us >= (uint16_t)(expected_us * (1.0 - PERCENT_TOLERANCE / 100.0))) && (duration_us <= (uint16_t)(expected_us * (1.0 + PERCENT_TOLERANCE / 100.0)));
}

bool IRremoteESP32::NEC_is0(rmt_item32_t *item)
{
  return NEC_checkRange(item->duration0, NEC_BIT_ZERO_HIGH_US) && NEC_checkRange(item->duration1, NEC_BIT_ZERO_LOW_US);
}

bool IRremoteESP32::NEC_is1(rmt_item32_t *item)
{
  return NEC_checkRange(item->duration0, NEC_BIT_ONE_HIGH_US) && NEC_checkRange(item->duration1, NEC_BIT_ONE_LOW_US);
}

bool IRremoteESP32::NEC_isEnd(rmt_item32_t *item)
{
}

bool IRremoteESP32::NEC_isHeader(rmt_item32_t *item)
{
  return NEC_checkRange(item->duration0, NEC_HEADER_HIGH_US) && NEC_checkRange(item->duration1, NEC_HEADER_LOW_US);
}

bool IRremoteESP32::NEC_isRepeat(rmt_item32_t *item)
{
  return NEC_checkRange(item->duration0, NEC_HEADER_HIGH_US) && NEC_checkRange(item->duration1, NEC_HEADER_REPEAT_LOW_US);
}

int IRremoteESP32::decodeNEC(rmt_item32_t *item, int itemCount, uint32_t *data)
{
  if (itemCount == NEC_REPEAT_ITEM_COUNT)
  {
    *data = NEC_REPEAT_DATA;
    return NEC_REPEAT_ITEM_COUNT;
  }
  else if (itemCount == NEC_DATA_ITEM_COUNT)
  {
    if (NEC_isHeader(item))
    {
      // Skip
    }

    // Skip header and the end marker
    for (size_t i = 1; i < itemCount - 1; i++)
    {
      if (NEC_is1(item + i))
      {
        (*data) = (*data << 1) | 1;
      }
      else if (NEC_is0(item + i))
      {
        (*data) <<= 1;
      }
    }

    return itemCount - 2;
  }
  else
  {
#if DEBUG
    Serial.println("Unkown protocol");
#endif
    return 0;
  }
}

int IRremoteESP32::readNEC(uint32_t *data)
{
  size_t itemSize = 0;
  RingbufHandle_t rb = NULL;
  rmt_get_ringbuf_handle((rmt_channel_t)rmtport, &rb);

  while (rb)
  {
    rmt_item32_t *item = (rmt_item32_t *)xRingbufferReceive(rb, &itemSize, (TickType_t)rmt_item32_TIMEOUT_US); //portMAX_DELAY);
    int numItems = itemSize / sizeof(rmt_item32_t);
    if (numItems == 0)
    {
      return 0;
    }

    auto res = decodeNEC(item, numItems, data);

    // TODO: Why do we need to put it back?
    vRingbufferReturnItem(rb, (void *)item);
    return res;
  }

  return 0;
}
