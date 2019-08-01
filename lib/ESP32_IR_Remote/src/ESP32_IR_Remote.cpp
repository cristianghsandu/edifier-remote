/* Copyright (c) 2018 Darryl Scott. All Rights Reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>. *
 * 
 * Based on the Code from Neil Kolban: https://github.com/nkolban/esp32-snippets/blob/master/hardware/infra_red/receiver/rmt_receiver.c
 * Based on the Code from pcbreflux: https://github.com/pcbreflux/espressif/blob/master/esp32/arduino/sketchbook/ESP32_IR_Remote/ir_demo/ir_demo.ino
 * Based on the Code from Xplorer001: https://github.com/ExploreEmbedded/ESP32_RMT
 */
/* This is a simple code to receive and then save IR received by an IR sensor connected to the ESP32 and then resend it back out via an IR LED  
 * Reason that I created this I could not find any complete examples of IR send and Received in the RAW format, I am hoping this code can then be used
 * to build on the existing IR libraries out there.
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

#include "ESP32_IR_Remote.h"

#define roundTo 50                 //rounding microseconds timings
#define MARK_EXCESS 220            //tweeked to get the right timing
#define SPACE_EXCESS 190           //tweeked to get the right timing
#define rmt_item32_TIMEOUT_US 5000 /*!< RMT receiver timeout value(us) */

// Clock divisor (base clock is 80MHz)
#define CLK_DIV 80

// Number of clock ticks that represent 10us.  10 us = 1/100th msec.
#define TICK_10_US (80000000 / CLK_DIV / 100000)

#define NEC_BITS 32
#define NEC_HDR_MARK 9000
#define NEC_HDR_SPACE 4500
#define NEC_BIT_MARK 560
#define NEC_ONE_SPACE 1690
#define NEC_ZERO_SPACE 560
#define NEC_RPT_SPACE 2250

static RingbufHandle_t ringBuf;

ESP32_IRrecv::ESP32_IRrecv()
{
}

void ESP32_IRrecv::ESP32_IRrecvPIN(int recvpin, int port)
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

void ESP32_IRrecv::ESP32_IRrecvPIN(int recvpin)
{
  ESP32_IRrecvPIN(recvpin, (int)RMT_CHANNEL_0);
}

void ESP32_IRrecv::ESP32_IRsendPIN(int sendpin)
{
  ESP32_IRrecvPIN(sendpin, (int)RMT_CHANNEL_0);
}

void ESP32_IRrecv::ESP32_IRsendPIN(int sendpin, int port)
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

void ESP32_IRrecv::initReceive()
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
  rmt_get_ringbuf_handle(config.channel, &ringBuf);
  rmt_rx_start(config.channel, 1);
}

void ESP32_IRrecv::initSend()
{
  rmt_config_t config;
  config.channel = (rmt_channel_t)rmtport;
  config.gpio_num = (gpio_num_t)gpionum;
  config.mem_block_num = 1; //how many memory blocks 64 x N (0-7)
  config.clk_div = CLK_DIV;
  config.tx_config.loop_en = false;
  config.tx_config.carrier_duty_percent = 30;
  config.tx_config.carrier_freq_hz = 38000;
  config.tx_config.carrier_level = RMT_CARRIER_LEVEL_HIGH;
  config.tx_config.carrier_en = true;
  config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
  config.tx_config.idle_output_en = true;
  config.rmt_mode = (rmt_mode_t)0; //RMT_MODE_TX;
  rmt_config(&config);
  rmt_driver_install(config.channel, 0, 0); //19     /*!< RMT interrupt number, select from soc.h */
}

void ESP32_IRrecv::sendIR(int *data, int IRlength)
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
    buildItem(item[i], data[x], -1 * data[x + 1]);
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

void ESP32_IRrecv::stopIR()
{
  rmt_config_t config;
  config.channel = (rmt_channel_t)rmtport;
  rmt_rx_stop(config.channel);
  Serial.print("Uninstalling..");
  Serial.print("Port : ");
  Serial.println(config.channel);
  rmt_driver_uninstall(config.channel);
}

void ESP32_IRrecv::getDataIR(rmt_item32_t item, int *datato, int index)
{
  int lowValue = (item.duration0) * (10 / TICK_10_US) - SPACE_EXCESS;
  lowValue = roundTo * round((float)lowValue / roundTo);
  Serial.print(lowValue);
  Serial.print(",");
  datato[index] = -lowValue;
  int highValue = (item.duration1) * (10 / TICK_10_US) + MARK_EXCESS;
  highValue = roundTo * round((float)highValue / roundTo);
  Serial.print(highValue);
  Serial.print(",");
  datato[index + 1] = highValue;
}

void ESP32_IRrecv::buildItem(rmt_item32_t &item, int high_us, int low_us)
{
  item.level0 = true;
  item.duration0 = (high_us / 10 * TICK_10_US);
  item.level1 = false;
  item.duration1 = (low_us / 10 * TICK_10_US);
}

void ESP32_IRrecv::decodeRAW(rmt_item32_t *data, int numItems, int *datato)
{
  int x = 0;
  for (int i = 0; i < numItems; i++)
  {
    getDataIR(data[i], datato, x);
    x = x + 2;
  }
  Serial.println();
}

bool ESP32_IRrecv::isInRange(rmt_item32_t item, int lowDuration, int highDuration, int tolerance)
{
  uint32_t lowValue = item.duration0 * 10 / TICK_10_US;
  uint32_t highValue = item.duration1 * 10 / TICK_10_US;
  /*
  ESP_LOGI(TAG, "lowValue=%d, highValue=%d, lowDuration=%d, highDuration=%d",
    lowValue, highValue, lowDuration, highDuration);
  */
  if (lowValue < (lowDuration - tolerance) || lowValue > (lowDuration + tolerance) ||
      (highValue != 0 &&
       (highValue < (highDuration - tolerance) || highValue > (highDuration + tolerance))))
  {
    return false;
  }
  return true;
}

bool ESP32_IRrecv::NEC_is0(rmt_item32_t item)
{
  return isInRange(item, NEC_BIT_MARK, NEC_BIT_MARK, 100);
}

bool ESP32_IRrecv::NEC_is1(rmt_item32_t item)
{
  return isInRange(item, NEC_BIT_MARK, NEC_ONE_SPACE, 100);
}

int ESP32_IRrecv::decodeNEC(rmt_item32_t *data, int numItems)
{
  if (!isInRange(data[0], NEC_HDR_MARK, NEC_HDR_SPACE, 200))
  {
    //ESP_LOGD(TAG, "Not an NEC");
    return 0;
  }

  int i;
  uint8_t address = 0, notAddress = 0, command = 0, notCommand = 0;
  int accumCounter = 0;
  uint8_t accumValue = 0;
  for (i = 1; i < numItems; i++)
  {
    if (NEC_is0(data[i]))
    {
      //ESP_LOGD(TAG, "%d: 0", i);
      accumValue = accumValue >> 1;
    }
    else if (NEC_is1(data[i]))
    {
      //ESP_LOGD(TAG, "%d: 1", i);
      accumValue = (accumValue >> 1) | 0x80;
    }
    else
    {
      //ESP_LOGD(TAG, "Unknown");
    }
    if (accumCounter == 7)
    {
      accumCounter = 0;
      //ESP_LOGD(TAG, "Byte: 0x%.2x", accumValue);
      if (i == 8)
      {
        address = accumValue;
      }
      else if (i == 16)
      {
        notAddress = accumValue;
      }
      else if (i == 24)
      {
        command = accumValue;
      }
      else if (i == 32)
      {
        notCommand = accumValue;
      }
      accumValue = 0;
    }
    else
    {
      accumCounter++;
    }
  }
  //ESP_LOGD(TAG, "Address: 0x%.2x, NotAddress: 0x%.2x", address, notAddress ^ 0xff);
  if (address != (notAddress ^ 0xff) || command != (notCommand ^ 0xff))
  {
    // Data mis match
    return 0;
  }
  // Serial.print("Address: ");
  // Serial.print(address);
  // Serial.print(" Command: ");
  // Serial.println(command);

  return command;
}

int ESP32_IRrecv::readNEC(int *data, int maxBuf)
{
  RingbufHandle_t rb = NULL;
  rmt_config_t config;
  config.channel = (rmt_channel_t)rmtport;

  memset(data, 0, maxBuf);

  rmt_get_ringbuf_handle(config.channel, &rb);
  int count = maxBuf;
  if (rb)
  {
    Serial.print("NEC IR Code :");
    while (count)
    {
      size_t itemSize = 0;
      rmt_item32_t *item = (rmt_item32_t *)xRingbufferReceive(rb, &itemSize, (TickType_t)rmt_item32_TIMEOUT_US); //portMAX_DELAY);
      int numItems = itemSize / sizeof(rmt_item32_t);
      if (numItems == 0)
      {
        return 0;
      }

      data[count] = decodeNEC(item, numItems);
      Serial.print(data[count]);
      vRingbufferReturnItem(ringBuf, (void *)item);

      count--;
    }
    Serial.println();

    return maxBuf - count;
  }

  vTaskDelete(NULL);
  return 0;
}

int ESP32_IRrecv::readIR(int *data, int maxBuf)
{
  RingbufHandle_t rb = NULL;
  rmt_config_t config;
  config.channel = (rmt_channel_t)rmtport;

  rmt_get_ringbuf_handle(config.channel, &rb);
  while (rb)
  {
    size_t itemSize = 0;
    rmt_item32_t *item = (rmt_item32_t *)xRingbufferReceive(rb, &itemSize, (TickType_t)rmt_item32_TIMEOUT_US); //portMAX_DELAY);
    int numItems = itemSize / sizeof(rmt_item32_t);
    if (numItems == 0)
    {
      return 0;
    }

    Serial.print("Found num of Items :");
    Serial.println(numItems * 2 - 1);
    memset(data, 0, maxBuf);

    Serial.print("Raw IR Code :");
    decodeRAW(item, numItems, data);
    vRingbufferReturnItem(ringBuf, (void *)item);
    return (numItems * 2 - 1);
  }

  vTaskDelete(NULL);
  return 0;
}
