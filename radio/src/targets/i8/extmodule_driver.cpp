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

void extmoduleSendNextFrame();

#ifndef _MSC_VER
#warning "TODO check TRAINER_EXTMODULE_TIMER_IRQn, the timer is also used by trainer"
#endif

void extmoduleStop()
{
  NVIC_DisableIRQ(EXTMODULE_DMA_IRQn);
  NVIC_DisableIRQ(TRAINER_EXTMODULE_TIMER_IRQn);

  EXTMODULE_DMA_STREAM->CR &= ~DMA_SxCR_EN; // Disable DMA
  EXTMODULE_TIMER->DIER &= ~(TIM_DIER_CC2IE | TIM_DIER_UDE);
  EXTMODULE_TIMER->CR1 &= ~TIM_CR1_CEN;

  if (!IS_TRAINER_EXTERNAL_MODULE()) {
    EXTERNAL_MODULE_OFF();
  }
}

void extmoduleNoneStart()
{
  if (!IS_TRAINER_EXTERNAL_MODULE()) {
    EXTERNAL_MODULE_OFF();
  }

  GPIO_PinAFConfig(EXTMODULE_PPM_GPIO, EXTMODULE_PPM_GPIO_PinSource, 0);

  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = EXTMODULE_PPM_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(EXTMODULE_PPM_GPIO, &GPIO_InitStructure);
  GPIO_SetBits(EXTMODULE_PPM_GPIO, EXTMODULE_PPM_GPIO_PIN); // Set high

  EXTMODULE_TIMER->CR1 &= ~TIM_CR1_CEN;
  EXTMODULE_TIMER->PSC = TRAINER_EXTMODULE_TIMER_FREQ / 2000000 - 1; // 0.5uS from 30MHz
  EXTMODULE_TIMER->ARR = 36000; // 18mS
  EXTMODULE_TIMER->CCR2 = 32000; // Update time
  EXTMODULE_TIMER->EGR = 1; // Restart
  EXTMODULE_TIMER->SR &= ~TIM_SR_CC2IF;
  EXTMODULE_TIMER->DIER |= TIM_DIER_CC2IE; // Enable this interrupt
  EXTMODULE_TIMER->CR1 |= TIM_CR1_CEN;

  NVIC_EnableIRQ(TRAINER_EXTMODULE_TIMER_IRQn);
  NVIC_SetPriority(TRAINER_EXTMODULE_TIMER_IRQn, 7);
}

void extmodulePpmStart()
{
  EXTERNAL_MODULE_ON();

  GPIO_PinAFConfig(EXTMODULE_PPM_GPIO, EXTMODULE_PPM_GPIO_PinSource, EXTMODULE_PPM_GPIO_AF);

  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = EXTMODULE_PPM_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(EXTMODULE_PPM_GPIO, &GPIO_InitStructure);

  EXTMODULE_TIMER->CR1 &= ~TIM_CR1_CEN;
  EXTMODULE_TIMER->PSC = TRAINER_EXTMODULE_TIMER_FREQ / 2000000 - 1; // 0.5uS from 30MHz
  EXTMODULE_TIMER->ARR = 45000;
  EXTMODULE_TIMER->CCR1 = GET_PPM_DELAY(EXTERNAL_MODULE)*2;
  EXTMODULE_TIMER->CCER = TIM_CCER_CC1NE | (GET_PPM_POLARITY(EXTERNAL_MODULE) ? TIM_CCER_CC1NP : 0); //     // we are using complementary output so logic has to be reversed here
  EXTMODULE_TIMER->BDTR = TIM_BDTR_MOE;
  EXTMODULE_TIMER->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_0; // Force O/P high
  EXTMODULE_TIMER->EGR = 1;
  EXTMODULE_TIMER->DIER |= TIM_DIER_UDE;
  EXTMODULE_TIMER->CCMR1 = TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC2PE; // PWM mode 1
  EXTMODULE_TIMER->CR1 |= TIM_CR1_CEN;

  extmoduleSendNextFrame();

  NVIC_EnableIRQ(EXTMODULE_DMA_IRQn);
  NVIC_SetPriority(EXTMODULE_DMA_IRQn, 7);
  NVIC_EnableIRQ(TRAINER_EXTMODULE_TIMER_IRQn);
  NVIC_SetPriority(TRAINER_EXTMODULE_TIMER_IRQn, 7);
}

void extmodulePxxStart()
{
  EXTERNAL_MODULE_ON();

  GPIO_PinAFConfig(EXTMODULE_PPM_GPIO, EXTMODULE_PPM_GPIO_PinSource, EXTMODULE_PPM_GPIO_AF);

  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = EXTMODULE_PPM_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(EXTMODULE_PPM_GPIO, &GPIO_InitStructure);

  EXTMODULE_TIMER->CR1 &= ~TIM_CR1_CEN;
  EXTMODULE_TIMER->PSC = TRAINER_EXTMODULE_TIMER_FREQ / 2000000 - 1; // 0.5uS (2Mhz)
  EXTMODULE_TIMER->ARR = 18000;
  EXTMODULE_TIMER->CCER = TIM_CCER_CC1E | TIM_CCER_CC1NE;    //polarity, default low
  EXTMODULE_TIMER->BDTR = TIM_BDTR_MOE; // Enable outputs
  EXTMODULE_TIMER->CCR1 = 18;
  EXTMODULE_TIMER->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_0; // Force O/P high
  EXTMODULE_TIMER->EGR = 1; // Restart
  EXTMODULE_TIMER->DIER |= TIM_DIER_UDE; // Enable DMA on update
  EXTMODULE_TIMER->CCMR1 = TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2;
  EXTMODULE_TIMER->CR1 |= TIM_CR1_CEN;

  extmoduleSendNextFrame();

  NVIC_EnableIRQ(EXTMODULE_DMA_IRQn);
  NVIC_SetPriority(EXTMODULE_DMA_IRQn, 7);
  NVIC_EnableIRQ(TRAINER_EXTMODULE_TIMER_IRQn);
  NVIC_SetPriority(TRAINER_EXTMODULE_TIMER_IRQn, 7);
}

#if defined(DSM2) || defined(MULTIMODULE)
void extmoduleDsm2Start()
{
  EXTERNAL_MODULE_ON();

  GPIO_PinAFConfig(EXTMODULE_PPM_GPIO, EXTMODULE_PPM_GPIO_PinSource, EXTMODULE_PPM_GPIO_AF);

  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = EXTMODULE_PPM_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(EXTMODULE_PPM_GPIO, &GPIO_InitStructure);

  EXTMODULE_TIMER->CR1 &= ~TIM_CR1_CEN;
  EXTMODULE_TIMER->PSC = TRAINER_EXTMODULE_TIMER_FREQ / 2000000 - 1; // 0.5uS from 30MHz
  EXTMODULE_TIMER->ARR = 44000; // 22mS
  EXTMODULE_TIMER->CCER = TIM_CCER_CC1NE | TIM_CCER_CC1NP;
  EXTMODULE_TIMER->BDTR = TIM_BDTR_MOE; // Enable outputs
  EXTMODULE_TIMER->CCR1 = 0;
  EXTMODULE_TIMER->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_0; // Force O/P high
  EXTMODULE_TIMER->EGR = 1; // Restart
  EXTMODULE_TIMER->DIER |= TIM_DIER_UDE; // Enable DMA on update
  EXTMODULE_TIMER->CCMR1 = TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_0;
  EXTMODULE_TIMER->CR1 |= TIM_CR1_CEN;

  extmoduleSendNextFrame();

  NVIC_EnableIRQ(EXTMODULE_DMA_IRQn);
  NVIC_SetPriority(EXTMODULE_DMA_IRQn, 7);
  NVIC_EnableIRQ(TRAINER_EXTMODULE_TIMER_IRQn);
  NVIC_SetPriority(TRAINER_EXTMODULE_TIMER_IRQn, 7);
}
#endif

void extmoduleSendNextFrame()
{
  if (moduleState[EXTERNAL_MODULE].protocol == PROTOCOL_CHANNELS_PPM) {
    EXTMODULE_TIMER->CCR1 = GET_PPM_DELAY(EXTERNAL_MODULE)*2;
    EXTMODULE_TIMER->CCER = TIM_CCER_CC1NE | (GET_PPM_POLARITY(EXTERNAL_MODULE) ? TIM_CCER_CC1NP : 0); //     // we are using complementary output so logic has to be reversed here
    EXTMODULE_TIMER->CCR2 = *(extmodulePulsesData.ppm.ptr - 1) - 4000; // 2mS in advance
    EXTMODULE_DMA_STREAM->CR &= ~DMA_SxCR_EN; // Disable DMA
    EXTMODULE_DMA_STREAM->CR |= EXTMODULE_DMA_CHANNEL | DMA_SxCR_DIR_0 | DMA_SxCR_MINC | DMA_SxCR_PSIZE_0 | DMA_SxCR_MSIZE_0 | DMA_SxCR_PL_0 | DMA_SxCR_PL_1;
    EXTMODULE_DMA_STREAM->PAR = CONVERT_PTR_UINT(&EXTMODULE_TIMER->ARR);
    EXTMODULE_DMA_STREAM->M0AR = CONVERT_PTR_UINT(extmodulePulsesData.ppm.pulses);
    EXTMODULE_DMA_STREAM->NDTR = extmodulePulsesData.ppm.ptr - extmodulePulsesData.ppm.pulses;
    EXTMODULE_DMA_STREAM->CR |= DMA_SxCR_EN | DMA_SxCR_TCIE; // Enable DMA
  }
  else if (moduleState[EXTERNAL_MODULE].protocol == PROTOCOL_CHANNELS_PXX1_PULSES) {
    EXTMODULE_TIMER->CCR2 = *(extmodulePulsesData.pxx.ptr - 1) - 4000; // 2mS in advance
    EXTMODULE_DMA_STREAM->CR &= ~DMA_SxCR_EN; // Disable DMA
    EXTMODULE_DMA_STREAM->CR |= EXTMODULE_DMA_CHANNEL | DMA_SxCR_DIR_0 | DMA_SxCR_MINC | DMA_SxCR_PSIZE_0 | DMA_SxCR_MSIZE_0 | DMA_SxCR_PL_0 | DMA_SxCR_PL_1;
    EXTMODULE_DMA_STREAM->PAR = CONVERT_PTR_UINT(&EXTMODULE_TIMER->ARR);
    EXTMODULE_DMA_STREAM->M0AR = CONVERT_PTR_UINT(extmodulePulsesData.pxx.pulses);
    EXTMODULE_DMA_STREAM->NDTR = extmodulePulsesData.pxx.ptr - extmodulePulsesData.pxx.pulses;
    EXTMODULE_DMA_STREAM->CR |= DMA_SxCR_EN | DMA_SxCR_TCIE; // Enable DMA
  }
  else if (IS_DSM2_PROTOCOL(moduleState[EXTERNAL_MODULE].protocol) || IS_MULTIMODULE_PROTOCOL(moduleState[EXTERNAL_MODULE].protocol) || IS_SBUS_PROTOCOL(moduleState[EXTERNAL_MODULE].protocol)) {
    if (IS_SBUS_PROTOCOL(moduleState[EXTERNAL_MODULE].protocol))
      EXTMODULE_TIMER->CCER = TIM_CCER_CC1NE | (GET_SBUS_POLARITY(EXTERNAL_MODULE) ? TIM_CCER_CC1NP : 0); // reverse polarity for Sbus if needed
    EXTMODULE_TIMER->CCR2 = *(extmodulePulsesData.dsm2.ptr - 1) - 4000; // 2mS in advance
    EXTMODULE_DMA_STREAM->CR &= ~DMA_SxCR_EN; // Disable DMA
    EXTMODULE_DMA_STREAM->CR |= EXTMODULE_DMA_CHANNEL | DMA_SxCR_DIR_0 | DMA_SxCR_MINC | DMA_SxCR_PSIZE_0 | DMA_SxCR_MSIZE_0 | DMA_SxCR_PL_0 | DMA_SxCR_PL_1;
    EXTMODULE_DMA_STREAM->PAR = CONVERT_PTR_UINT(&EXTMODULE_TIMER->ARR);
    EXTMODULE_DMA_STREAM->M0AR = CONVERT_PTR_UINT(extmodulePulsesData.dsm2.pulses);
    EXTMODULE_DMA_STREAM->NDTR = extmodulePulsesData.dsm2.ptr - extmodulePulsesData.dsm2.pulses;
    EXTMODULE_DMA_STREAM->CR |= DMA_SxCR_EN | DMA_SxCR_TCIE; // Enable DMA
  }
  else {
    EXTMODULE_TIMER->DIER |= TIM_DIER_CC2IE;
  }
}

#if defined(EXTMODULE_DMA_IRQHandler)
extern "C" void EXTMODULE_DMA_IRQHandler()
{
  if (!DMA_GetITStatus(EXTMODULE_DMA_STREAM, EXTMODULE_DMA_FLAG_TC))
    return;

  DMA_ClearITPendingBit(EXTMODULE_DMA_STREAM, EXTMODULE_DMA_FLAG_TC);

  EXTMODULE_TIMER->SR &= ~TIM_SR_CC2IF; // Clear flag
  EXTMODULE_TIMER->DIER |= TIM_DIER_CC2IE; // Enable this interrupt
}
#endif

// The Timer interrupt code is inside the Trainer driver (same timer)
