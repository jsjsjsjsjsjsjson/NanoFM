#include "app_main.h"
#include "stm32f1xx_hal.h"

#include "FreeRTOS.h"
#include "task.h"
#include "tusb.h"

#include <stdio.h>
#include <string.h>

#include "config.h"
#include "synth_core.h"

extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim4;

#define PWM_PERIOD 1023

static uint16_t pwm_buf[BLOCK_SIZE * 2];

static TaskHandle_t audio_task_handle = NULL;

static SYNTH synth;

enum {
    AUDIO_EVT_HALF = 1u << 0,
    AUDIO_EVT_FULL = 1u << 1,
};

static uint16_t pcm_to_pwm(int16_t s)
{
    uint32_t u = (uint32_t)((int32_t)s + 32768);
    uint32_t pwm = (u * (PWM_PERIOD + 1)) >> 16;
    if (pwm > PWM_PERIOD) {
        pwm = PWM_PERIOD;
    }

    return (uint16_t)pwm;
}

static void render_to_pwm(uint16_t *dst, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) {
        dst[i] = pcm_to_pwm(synth.process());
    }

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, (GPIO_PinState)!HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13));
}

static void audio_dma_half_callback(DMA_HandleTypeDef *)
{
    BaseType_t woken = pdFALSE;

    if (audio_task_handle != nullptr) {
        xTaskNotifyFromISR(audio_task_handle, AUDIO_EVT_HALF, eSetBits, &woken);
        portYIELD_FROM_ISR(woken);
    }
}

static void audio_dma_full_callback(DMA_HandleTypeDef *)
{
    BaseType_t woken = pdFALSE;

    if (audio_task_handle != nullptr) {
        xTaskNotifyFromISR(audio_task_handle, AUDIO_EVT_FULL, eSetBits, &woken);
        portYIELD_FROM_ISR(woken);
    }
}

static void midi_handle_packet(uint8_t const packet[4])
{
    uint8_t status = packet[1];
    uint8_t data1  = packet[2];
    uint8_t data2  = packet[3];

    uint8_t type = status & 0xF0;
    uint8_t ch   = status & 0x0F;

    switch (type) {
    case 0x80:
        synth.note_off(ch, data1);
        break;

    case 0x90:
        synth.note_on(ch, data1, data2);
        break;

    case 0xB0:
        synth.control_change(ch, data1, data2);
        break;

    case 0xC0:
        synth.program_change(ch, data1);
        break;
    }
}

void midi_task(void *arg)
{
    (void)arg;
    uint8_t packet[4];

    for (;;) {
        while (tud_midi_available()) {
            if (tud_midi_packet_read(packet)) {
                midi_handle_packet(packet);
            }
        }

        taskYIELD();
    }
}

static void audio_task(void *)
{
    render_to_pwm(&pwm_buf[0], BLOCK_SIZE);
    render_to_pwm(&pwm_buf[BLOCK_SIZE], BLOCK_SIZE);

    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);

    htim2.hdma[TIM_DMA_ID_UPDATE]->XferHalfCpltCallback = audio_dma_half_callback;
    htim2.hdma[TIM_DMA_ID_UPDATE]->XferCpltCallback     = audio_dma_full_callback;

    HAL_DMA_Start_IT(
        htim2.hdma[TIM_DMA_ID_UPDATE],
        (uint32_t)pwm_buf,
        (uint32_t)&TIM4->CCR1,
        BLOCK_SIZE * 2
    );

    __HAL_TIM_ENABLE_DMA(&htim2, TIM_DMA_UPDATE);
    HAL_TIM_Base_Start(&htim2);

    xTaskCreate(midi_task, "midi", 128, NULL, 10, NULL);

    for (;;) {
        uint32_t ev = 0;

        xTaskNotifyWait(
            0,
            0xFFFFFFFF,
            &ev,
            portMAX_DELAY
        );

        if (ev & AUDIO_EVT_HALF) {
            render_to_pwm(&pwm_buf[0], BLOCK_SIZE);
        }

        if (ev & AUDIO_EVT_FULL) {
            render_to_pwm(&pwm_buf[BLOCK_SIZE], BLOCK_SIZE);
        }

        taskYIELD();
    }
}

void app_main(void)
{
    memset(pwm_buf, 0, sizeof(pwm_buf));

    BaseType_t ok = xTaskCreate(
        audio_task,
        "synth",
        256,
        NULL,
        10,
        &audio_task_handle
    );

    printf("audio task create: %ld\n", (long)ok);
}
