/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "opentx.h"
#include "crc.h"
#include "io/hallStick_parser.h"
#include "usb_conf.h"
#include <stdarg.h>
#include <stdio.h>

DMAFifo<HALLSTICK_BUFF_SIZE> hallDMAFifo __DMA (HALL_DMA_Stream_RX);
Fifo<uint8_t, HALLSTICK_BUFF_SIZE> hallStickTxFifo;
static uint8_t hallStickSendState = HALLSTICK_SEND_STATE_IDLE;
unsigned char HallCmd[264] __DMA;
static hallStickParser parser;

STRUCT_HALL HallProtocol = { 0 };
STRUCT_HALL HallProtocolTx = { 0 };
signed short hall_raw_values[FLYSKY_HALL_CHANNEL_COUNT];
STRUCT_STICK_CALIBRATION hall_calibration[FLYSKY_HALL_CHANNEL_COUNT] = { {0, 0, 0} };
unsigned short hall_adc_values[FLYSKY_HALL_CHANNEL_COUNT];

const uint8_t sticks_mapping[4] = { 0 /*STICK1*/,  1/*STICK2*/, 2/*STICK3*/, 3 /*STICK4*/};

unsigned short calc_crc16(void *pBuffer, uint32_t bufferSize)
{
  return crc16(CRC_1021, (const uint8_t*)pBuffer, bufferSize, 0xffff);
}

uint16_t get_hall_adc_value(uint8_t ch)
{
  if (ch >= FLYSKY_HALL_CHANNEL_COUNT)
  {
    return 0;
  }

#if defined(FLYSKY_HALL_STICKS_REVERSE)
  ch = sticks_mapping[ch];

  return MAX_ADC_CHANNEL_VALUE - hall_adc_values[ch];
#else
  if (ch < 2)
  {
    return MAX_ADC_CHANNEL_VALUE - hall_adc_values[ch];
  }

  return hall_adc_values[ch];
#endif
}


void hall_stick_init(uint32_t baudrate)
{
  if (baudrate == 0)
  {
    USART_DeInit(HALL_SERIAL_USART);
    return;
  }

  NVIC_InitTypeDef NVIC_InitStructure;
  NVIC_InitStructure.NVIC_IRQChannel = HALL_SERIAL_RX_DMA_Stream_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  USART_InitTypeDef USART_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;

  GPIO_PinAFConfig(HALL_SERIAL_GPIO, HALL_SERIAL_RX_GPIO_PinSource, HALL_SERIAL_GPIO_AF);
  GPIO_PinAFConfig(HALL_SERIAL_GPIO, HALL_SERIAL_TX_GPIO_PinSource, HALL_SERIAL_GPIO_AF);

  GPIO_InitStructure.GPIO_Pin = HALL_SERIAL_TX_GPIO_PIN | HALL_SERIAL_RX_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(HALL_SERIAL_GPIO, &GPIO_InitStructure);

  USART_InitStructure.USART_BaudRate = baudrate;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
  USART_Init(HALL_SERIAL_USART, &USART_InitStructure);

  DMA_Cmd(HALL_DMA_Stream_RX, DISABLE);
  USART_DMACmd(HALL_SERIAL_USART, USART_DMAReq_Rx, DISABLE);
  DMA_DeInit(HALL_DMA_Stream_RX);

  DMA_InitTypeDef DMA_InitStructure;
  hallDMAFifo.clear();

  USART_ITConfig(HALL_SERIAL_USART, USART_IT_RXNE, DISABLE);
  USART_ITConfig(HALL_SERIAL_USART, USART_IT_TXE, DISABLE);

  DMA_InitStructure.DMA_Channel = HALL_DMA_Channel;
  DMA_InitStructure.DMA_PeripheralBaseAddr = CONVERT_PTR_UINT(&HALL_SERIAL_USART->DR);
  DMA_InitStructure.DMA_Memory0BaseAddr = CONVERT_PTR_UINT(hallDMAFifo.buffer());
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
  DMA_InitStructure.DMA_BufferSize = hallDMAFifo.size();
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  DMA_Init(HALL_DMA_Stream_RX, &DMA_InitStructure);
  USART_DMACmd(HALL_SERIAL_USART, USART_DMAReq_Rx, ENABLE);
  USART_Cmd(HALL_SERIAL_USART, ENABLE);
  DMA_Cmd(HALL_DMA_Stream_RX, ENABLE);

  reset_hall_stick();
}




void HallSendBuffer(uint8_t * buffer, uint32_t count)
{
  for(uint32_t idx = 0; buffer != HallCmd && idx < count; idx++)
  {
    HallCmd[idx] = buffer[idx];
  }
  DMA_InitTypeDef DMA_InitStructure;
  DMA_DeInit(HALL_DMA_Stream_TX);
  DMA_InitStructure.DMA_Channel = HALL_DMA_Channel;
  DMA_InitStructure.DMA_PeripheralBaseAddr = CONVERT_PTR_UINT(&HALL_SERIAL_USART->DR);
  DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
  DMA_InitStructure.DMA_Memory0BaseAddr = CONVERT_PTR_UINT(HallCmd);
  DMA_InitStructure.DMA_BufferSize = count;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  DMA_Init(HALL_DMA_Stream_TX, &DMA_InitStructure);
  DMA_Cmd(HALL_DMA_Stream_TX, ENABLE);
  USART_DMACmd(HALL_SERIAL_USART, USART_DMAReq_Tx, ENABLE);
  DMA_ITConfig(HALL_DMA_Stream_TX, DMA_IT_TC, ENABLE);

  /* enable interrupt and set it's priority */
  NVIC_EnableIRQ(HALL_SERIAL_TX_DMA_Stream_IRQn);
  NVIC_SetPriority(HALL_SERIAL_TX_DMA_Stream_IRQn, 7);
}

extern "C" void HALL_TX_DMA_Stream_IRQHandler(void)
{
  DEBUG_INTERRUPT(INT_TELEM_DMA);

  if (DMA_GetITStatus(HALL_DMA_Stream_TX, HALL_DMA_TX_FLAG_TC))
  {
    DMA_ClearITPendingBit(HALL_DMA_Stream_TX, HALL_DMA_TX_FLAG_TC);
    HALL_SERIAL_USART->CR1 |= USART_CR1_TCIE;
  }
}

void reset_hall_stick( void )
{
    unsigned short crc16 = 0xffff;

    HallCmd[0] = HALL_PROTOLO_HEAD;
    HallCmd[1] = 0xD1;
    HallCmd[2] = 0x01;
    HallCmd[3] = 0x01;

    crc16 = calc_crc16(HallCmd, 4);

    HallCmd[4] = crc16 & 0xff;
    HallCmd[5] = crc16 >>8 & 0xff;

    HallSendBuffer( HallCmd, 6);
}

void get_hall_config( void )
{
    unsigned short crc16 = 0xffff;

    HallCmd[0] = HALL_PROTOLO_HEAD;
    HallCmd[1] = 0xD1;
    HallCmd[2] = 0x01;
    HallCmd[3] = 0x00;

    crc16 = calc_crc16(HallCmd, 4); // 2B 2C

    HallCmd[4] = crc16 & 0xff;
    HallCmd[5] = crc16 >>8 & 0xff ;

    HallSendBuffer( HallCmd, 6);
}

inline void get_hall_firmware_info()
{
    unsigned short crc16 = 0xffff;

    HallCmd[0] = HALL_PROTOLO_HEAD;
    HallCmd[1] = 0xA2;
    HallCmd[2] = 0x00;

    crc16 = calc_crc16(HallCmd, 3); // BE 02

    HallCmd[3] = crc16 & 0xff;
    HallCmd[4] = crc16 >>8 & 0xff ;

    HallSendBuffer( HallCmd, 5);
}

inline void hallStickUpdatefwEnd( void )
{
    unsigned short crc16 = 0xffff;

    HallCmd[0] = HALL_PROTOLO_HEAD;
    HallCmd[1] = 0xA2;
    HallCmd[2] = 0x01;
    HallCmd[3] = 0x07;

    crc16 = calc_crc16(HallCmd, 4);

    HallCmd[4] = crc16 & 0xff;
    HallCmd[5] = crc16 >>8 & 0xff ;

    HallSendBuffer( HallCmd, 6);// 94 DD
}

void parseFlyskyData(STRUCT_HALL *hallBuffer, unsigned char ch)
{
  parser.parse(hallBuffer, ch);
}

#define ERROR_OFFSET      10
inline void convert_hall_to_adcVaule( void )
{
    uint16_t value;

    for ( uint8_t channel = 0; channel < 4; channel++ )
    {
        if (hall_raw_values[channel] < hall_calibration[channel].mid)
        {
            value = hall_calibration[channel].mid - (hall_calibration[channel].min+ERROR_OFFSET);
            value = ( MIDDLE_ADC_CHANNLE_VALUE * (hall_calibration[channel].mid - hall_raw_values[channel] ) ) / ( value );

            if (value >= MIDDLE_ADC_CHANNLE_VALUE ) {
                value = MIDDLE_ADC_CHANNLE_VALUE;
            }

            hall_adc_values[channel] = MIDDLE_ADC_CHANNLE_VALUE - value;
        }
        else
        {
            value = (hall_calibration[channel].max - ERROR_OFFSET) - hall_calibration[channel].mid;

            value = (MIDDLE_ADC_CHANNLE_VALUE * (hall_raw_values[channel] - hall_calibration[channel].mid ) ) / (value);

            if (value >= MIDDLE_ADC_CHANNLE_VALUE )
            {
                value = MIDDLE_ADC_CHANNLE_VALUE;
            }

            hall_adc_values[channel] = MIDDLE_ADC_CHANNLE_VALUE + value + 1;
        }
    }
}

/* HallStick send main program */
void hall_on_usb_data(uint8_t* buffer, uint32_t len)
{
    uint8_t *data = (uint8_t *)&HallProtocolTx;
    for(uint32_t i=0; i < len; i++)
    {
        parser.parse(&HallProtocolTx, buffer[i]);
        if (HallProtocolTx.valid)
        {
            HallProtocolTx.valid = 0;
            data[HallProtocolTx.length + 3] = HallProtocolTx.checkSum & 0xFF;
            data[HallProtocolTx.length + 4] = HallProtocolTx.checkSum >> 8;
            //TRACE("USB: %02X %02X %02X ...%04X; CRC:%04X", data[0], data[1], data[2],
            //      HallProtocolTx.checkSum, calc_crc16(data, HallProtocolTx.length+3));

            switch (HallProtocolTx.hallID.hall_Id.receiverID)
            {
            case TRANSFER_DIR_TXMCU:
                break;

            case TRANSFER_DIR_HALLSTICK:
                onFlySkyUsbDownloadStart(TRANSFER_DIR_HALLSTICK);

                if (HallProtocolTx.hallID.ID == 0xA2)
                {
                    if (HallProtocolTx.length == 0) // 55 A2 00 BE 02
                    {
                        hallStickSendState = HALLSTICK_STATE_SEND_RESET;
                        break;
                    }
                    else if (HallProtocolTx.length == 0x01 && 0x07 == HallProtocol.data[0] )
                    {
                        hallStickSendState = HALLSTICK_SEND_STATE_IDLE;
                    }
                }
                HallSendBuffer(data, HallProtocolTx.length + 3 + 2);
                break;
            case TRANSFER_DIR_RFMODULE:
                onFlySkyUsbDownloadStart(TRANSFER_DIR_RFMODULE);
                if (HallProtocolTx.hallID.ID == 0xAE && HallProtocolTx.length == 0)
                {
                    setFlyskyState(STATE_UPDATE_RF_FIRMWARE);
                    break;
                }

                if (HallProtocolTx.hallID.hall_Id.packetID == 0x0D && HallProtocolTx.data[0] == 1)
                {
                    onFlySkyGetVersionInfoStart(1);
                    break;
                }
                intmoduleSendBufferDMA(data, HallProtocolTx.length + 3 + 2);
                break;
            }
        }
    }
}


/* Run it in 1ms timer routine */
void hall_stick_loop(void)
{
    static uint8_t count = 0;
    static tmr10ms_t lastConfigTime = get_tmr10ms();
    bool log = 0;
    if (count > 10)
    {
        count = 0;
        switch (hallStickSendState)
        {
        case HALLSTICK_STATE_SEND_RESET:
            reset_hall_stick();
            hallStickSendState = HALLSTICK_STATE_GET_FIRMWARE;
            break;
        case HALLSTICK_STATE_GET_FIRMWARE:
            get_hall_firmware_info();
            hallStickSendState = HALLSTICK_STATE_UPDATE_FW;
            break;
        case HALLSTICK_STATE_UPDATE_FW:
            break;
        }
    }
    count++;
    uint8_t byte;
    while(hallDMAFifo.pop(byte))
    {
        HallProtocol.index++;
        parser.parse(&HallProtocol, byte);
        if (HallProtocol.valid)
        {
            HallProtocol.valid = 0;
            HallProtocol.stickState = HallProtocol.data[HallProtocol.length - 1];
            switch ( HallProtocol.hallID.hall_Id.receiverID )
            {
            case TRANSFER_DIR_TXMCU:
                if(HallProtocol.hallID.hall_Id.packetID == HALL_RESP_TYPE_CALIB) {
                  memcpy(&hall_calibration, HallProtocol.data, sizeof(hall_calibration));
                }
                else if(HallProtocol.hallID.hall_Id.packetID == HALL_RESP_TYPE_VALUES) {
                  memcpy(hall_raw_values, HallProtocol.data, sizeof(hall_raw_values));
                  convert_hall_to_adcVaule();
                }
                break;
            case TRANSFER_DIR_HOSTPC:
                if (HallProtocol.length == 0x01 && (HallProtocol.data[0] == 0x05 || HallProtocol.data[0] == 0x06))
                {
                    hallStickSendState = HALLSTICK_SEND_STATE_IDLE;
                }
            case TRANSFER_DIR_HALLSTICK:
                uint8_t *data = (uint8_t*)&HallProtocol;
                //HallProtocol.head = HALL_PROTOLO_HEAD;
                //TRACE("HALL: %02X %02X %02X ...%04X", data[0], data[1], data[2], HallProtocol.checkSum);
                data[HallProtocol.length + 3] = HallProtocol.checkSum & 0xFF;
                data[HallProtocol.length + 4] = HallProtocol.checkSum >> 8;
                usbDownloadTransmit(data, HallProtocol.length + 5 );
                break;
            }
        }
    }
    //check periodically  if calibration is correct
    if (get_tmr10ms() - lastConfigTime > 200 && hallStickSendState == HALLSTICK_SEND_STATE_IDLE)
    {
        //invalid calibration
        if(hall_calibration[0].max - hall_calibration[0].min < 1024) {
          get_hall_config();
          lastConfigTime = get_tmr10ms();
        }

        if (log)
        {
            TRACE_NOCRLF("Hall(%0d):", FLYSKY_HALL_BAUDRATE);
            for (int idx = 0; idx < HallProtocol.length + 5; idx++)
            {
                TRACE_NOCRLF(" %02X", *((uint8_t*)&HallProtocol + idx));
            }
            TRACE(";");
        }
    }
}
