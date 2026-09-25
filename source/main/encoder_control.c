#include "encoder_control.h"
#include "SX1509_encoder.h"
#include "control.h"
#include "tonex_params.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>
#include "ui.h"
#include "display.h"

void usb_modify_parameter(uint16_t index, float value);

static const char *TAG = "ENCODER";

typedef struct
{
    uint8_t pin_a;
    uint8_t pin_b;
    uint8_t pin_sw;
    uint8_t last_ab;
    uint8_t last_sw;
    uint8_t raw_sw;
    uint8_t sw_count;

    int8_t step_accum;

    bool active;
    int value;
} Encoder;

static int8_t encoder_decode(uint8_t last, uint8_t current)
{
    if ((last == 0 && current == 1) ||
        (last == 1 && current == 3) ||
        (last == 3 && current == 2) ||
        (last == 2 && current == 0))
        return 1;

    if ((last == 0 && current == 2) ||
        (last == 2 && current == 3) ||
        (last == 3 && current == 1) ||
        (last == 1 && current == 0))
        return -1;

    return 0;
}
static Encoder encoders[5] =
{
    {0, 1, 2, 0, 1, 1, 0, 0, false, 0},
    {3, 4, 5, 0, 1, 1, 0, 0, false, 0},
    {6, 7, 8, 0, 1, 1, 0, 0, false, 0},
    {9, 10, 11, 0, 1, 1, 0, 0, false, 0},
    {12, 13, 14, 0, 1, 1, 0, 0, false, 0}
};


void encoder_control_init(void)
{
    uint16_t pins = 0;

    if (SX1509E_getPinValues(&pins) == ESP_OK)
    {
        for (int i = 0; i < 5; i++)
        {
            uint8_t a = (pins >> encoders[i].pin_a) & 1;
            uint8_t b = (pins >> encoders[i].pin_b) & 1;

            encoders[i].last_ab = (a << 1) | b;
            encoders[i].last_sw = (pins >> encoders[i].pin_sw) & 1;
            encoders[i].raw_sw = encoders[i].last_sw;
            encoders[i].sw_count = 0;
        }
    }

    ESP_LOGI(TAG, "Encoder control initialized");
}

void encoder_control_task(void *arg)
{
    (void)arg;

    uint16_t pin_values = 0;

    while (1)
    {
        if (SX1509E_getPinValues(&pin_values) == ESP_OK)
        {
            for (int i = 0; i < 5; i++)
            {
                uint8_t a = (pin_values >> encoders[i].pin_a) & 1;
                uint8_t b = (pin_values >> encoders[i].pin_b) & 1;
                uint8_t ab = (a << 1) | b;

                /* Rotary encoder */
                if (ab != encoders[i].last_ab)
                {
                    int8_t direction =
                        encoder_decode(encoders[i].last_ab, ab);

                    encoders[i].last_ab = ab;

                    if (direction != 0 && encoders[i].active)
                    {
                        encoders[i].step_accum += direction;

                        /* CW */
                        if (encoders[i].step_accum >= 4)
                        {
                            encoders[i].step_accum = 0;

                            if (i == 0)
                            {
                                control_request_preset_up();
                            }
                           else if (i == 1)
                            {
                                encoders[i].value--;

                                if (encoders[i].value < 0)
                                    encoders[i].value = 0;

                                usb_modify_parameter(
                                    TONEX_PARAM_MODEL_GAIN,
                                    (float)encoders[i].value / 10.0f
                                );
                            }
                             else if (i == 2)
                            {
                                encoders[i].value++;

                                if (encoders[i].value > 100)
                                    encoders[i].value = 100;

                                tModellerParameter *param_ptr = NULL;
                                uint16_t delay_mix_param =
                                    TONEX_PARAM_DELAY_DIGITAL_MIX;

                                if (tonex_params_get_locked_access(&param_ptr) == ESP_OK)
                                {
                                    if ((int)param_ptr[TONEX_PARAM_DELAY_MODEL].Value
                                        == TONEX_DELAY_TAPE)
                                    {
                                        delay_mix_param =
                                            TONEX_PARAM_DELAY_TAPE_MIX;
                                    }

                                    tonex_params_release_locked_access();

                                    usb_modify_parameter(
                                        delay_mix_param,
                                        (float)encoders[i].value
                                    );
                                }
                            }
                            else if (i == 3)
                            {
                                encoders[i].value++;

                                if (encoders[i].value > 100)
                                    encoders[i].value = 100;

                                tModellerParameter *param_ptr = NULL;
                                uint16_t reverb_mix_param =
                                    TONEX_PARAM_REVERB_ROOM_MIX;

                                if (tonex_params_get_locked_access(&param_ptr) == ESP_OK)
                                {
                                    switch ((int)param_ptr[TONEX_PARAM_REVERB_MODEL].Value)
                                    {
                                        case TONEX_REVERB_SPRING_1:
                                            reverb_mix_param =
                                                TONEX_PARAM_REVERB_SPRING1_MIX;
                                            break;

                                        case TONEX_REVERB_SPRING_2:
                                            reverb_mix_param =
                                                TONEX_PARAM_REVERB_SPRING2_MIX;
                                            break;

                                        case TONEX_REVERB_SPRING_3:
                                            reverb_mix_param =
                                                TONEX_PARAM_REVERB_SPRING3_MIX;
                                            break;

                                        case TONEX_REVERB_SPRING_4:
                                            reverb_mix_param =
                                                TONEX_PARAM_REVERB_SPRING4_MIX;
                                            break;

                                        case TONEX_REVERB_ROOM:
                                            reverb_mix_param =
                                                TONEX_PARAM_REVERB_ROOM_MIX;
                                            break;

                                        case TONEX_REVERB_PLATE:
                                        default:
                                            reverb_mix_param =
                                                TONEX_PARAM_REVERB_PLATE_MIX;
                                            break;
                                    }

                                    tonex_params_release_locked_access();

                                    usb_modify_parameter(
                                        reverb_mix_param,
                                        (float)encoders[i].value
                                    );
                                }
                            }
                            else if (i == 4)
                            {
                                encoders[i].value += 3;

                                if (encoders[i].value > 3)
                                    encoders[i].value = 3;

                                usb_modify_parameter(
                                    TONEX_GLOBAL_MASTER_VOLUME,
                                    (float)encoders[i].value
                                );
                            }

                            ESP_LOGI(
                                TAG,
                                "Encoder %d: CW, value=%d",
                                i + 1,
                                encoders[i].value
                            );
                        }

                        /* CCW */
                        else if (encoders[i].step_accum <= -4)
                        {
                            encoders[i].step_accum = 0;

                            if (i == 0)
                            {
                                control_request_preset_down();
                            }
                            else if (i == 1)
                            {
                                encoders[i].value--;

                                usb_modify_parameter(
                                    TONEX_PARAM_MODEL_GAIN,
                                    (float)encoders[i].value / 10.0f
                                );
                            }
                            else if (i == 2)
                            {
                                encoders[i].value--;

                                if (encoders[i].value < 0)
                                    encoders[i].value = 0;

                                tModellerParameter *param_ptr = NULL;
                                uint16_t delay_mix_param =
                                    TONEX_PARAM_DELAY_DIGITAL_MIX;

                                if (tonex_params_get_locked_access(&param_ptr) == ESP_OK)
                                {
                                    if ((int)param_ptr[TONEX_PARAM_DELAY_MODEL].Value
                                        == TONEX_DELAY_TAPE)
                                    {
                                        delay_mix_param =
                                            TONEX_PARAM_DELAY_TAPE_MIX;
                                    }

                                    tonex_params_release_locked_access();

                                    usb_modify_parameter(
                                        delay_mix_param,
                                        (float)encoders[i].value
                                    );
                                }
                            }
                            else if (i == 3)
                            {
                                encoders[i].value--;

                                if (encoders[i].value < 0)
                                    encoders[i].value = 0;

                                tModellerParameter *param_ptr = NULL;
                                uint16_t reverb_mix_param =
                                    TONEX_PARAM_REVERB_ROOM_MIX;

                                if (tonex_params_get_locked_access(&param_ptr) == ESP_OK)
                                {
                                    switch ((int)param_ptr[TONEX_PARAM_REVERB_MODEL].Value)
                                    {
                                        case TONEX_REVERB_SPRING_1:
                                            reverb_mix_param =
                                                TONEX_PARAM_REVERB_SPRING1_MIX;
                                            break;

                                        case TONEX_REVERB_SPRING_2:
                                            reverb_mix_param =
                                                TONEX_PARAM_REVERB_SPRING2_MIX;
                                            break;

                                        case TONEX_REVERB_SPRING_3:
                                            reverb_mix_param =
                                                TONEX_PARAM_REVERB_SPRING3_MIX;
                                            break;

                                        case TONEX_REVERB_SPRING_4:
                                            reverb_mix_param =
                                                TONEX_PARAM_REVERB_SPRING4_MIX;
                                            break;

                                        case TONEX_REVERB_ROOM:
                                            reverb_mix_param =
                                                TONEX_PARAM_REVERB_ROOM_MIX;
                                            break;

                                        case TONEX_REVERB_PLATE:
                                        default:
                                            reverb_mix_param =
                                                TONEX_PARAM_REVERB_PLATE_MIX;
                                            break;
                                    }

                                    tonex_params_release_locked_access();

                                    usb_modify_parameter(
                                        reverb_mix_param,
                                        (float)encoders[i].value
                                    );
                                }
                            }
                            else if (i == 4)
                            {
                                encoders[i].value -= 3;

                                if (encoders[i].value < -40)
                                    encoders[i].value = -40;

                                usb_modify_parameter(
                                    TONEX_GLOBAL_MASTER_VOLUME,
                                    (float)encoders[i].value
                                );
                            }

                            ESP_LOGI(
                                TAG,
                                "Encoder %d: CCW, value=%d",
                                i + 1,
                                encoders[i].value
                            );
                        }
                    }
                }

                /* Switch */
                uint8_t sw =
                    (pin_values >> encoders[i].pin_sw) & 1;

                if (sw != encoders[i].raw_sw)
                {
                    encoders[i].raw_sw = sw;
                    encoders[i].sw_count = 0;
                }
                else if (encoders[i].sw_count < 4)
                {
                    encoders[i].sw_count++;

                    if (encoders[i].sw_count >= 4 &&
                        sw != encoders[i].last_sw)
                    {
                        encoders[i].last_sw = sw;

                        /* Button pressed */
                        if (sw == 0)
                        {
                            encoders[i].active = !encoders[i].active;

                            /* Encoder 2 = AMP GAIN */
                            if (encoders[i].active && i == 1)
                            {
                                tModellerParameter *param_ptr = NULL;

                                if (tonex_params_get_locked_access(&param_ptr) == ESP_OK)
                                {
                                    encoders[i].value =
                                        (int)(param_ptr[TONEX_PARAM_MODEL_GAIN].Value * 10.0f);

                                    tonex_params_release_locked_access();

                                    ESP_LOGI(
                                        TAG,
                                        "Encoder 2 start Gain: %.1f",
                                        (float)encoders[i].value / 10.0f
                                    );
                                }
                            }

                            /* Encoder 3 = DELAY MIX */
                            if (encoders[i].active && i == 2)
                            {
                                tModellerParameter *param_ptr = NULL;

                                if (tonex_params_get_locked_access(&param_ptr) == ESP_OK)
                                {
                                    uint16_t delay_mix_param =
                                        TONEX_PARAM_DELAY_DIGITAL_MIX;

                                    if ((int)param_ptr[TONEX_PARAM_DELAY_MODEL].Value
                                        == TONEX_DELAY_TAPE)
                                    {
                                        delay_mix_param =
                                            TONEX_PARAM_DELAY_TAPE_MIX;
                                    }

                                    encoders[i].value =
                                        (int)param_ptr[delay_mix_param].Value;

                                    tonex_params_release_locked_access();

                                    ESP_LOGI(
                                        TAG,
                                        "Encoder 3 start Delay: %d%%",
                                        encoders[i].value
                                    );
                                }
                            }

                            /* Encoder 4 = REVERB MIX */
                            if (encoders[i].active && i == 3)
                            {
                                tModellerParameter *param_ptr = NULL;

                                if (tonex_params_get_locked_access(&param_ptr) == ESP_OK)
                                {
                                    uint16_t reverb_mix_param =
                                        TONEX_PARAM_REVERB_ROOM_MIX;

                                    switch ((int)param_ptr[TONEX_PARAM_REVERB_MODEL].Value)
                                    {
                                        case TONEX_REVERB_SPRING_1:
                                            reverb_mix_param =
                                                TONEX_PARAM_REVERB_SPRING1_MIX;
                                            break;

                                        case TONEX_REVERB_SPRING_2:
                                            reverb_mix_param =
                                                TONEX_PARAM_REVERB_SPRING2_MIX;
                                            break;

                                        case TONEX_REVERB_SPRING_3:
                                            reverb_mix_param =
                                                TONEX_PARAM_REVERB_SPRING3_MIX;
                                            break;

                                        case TONEX_REVERB_SPRING_4:
                                            reverb_mix_param =
                                                TONEX_PARAM_REVERB_SPRING4_MIX;
                                            break;

                                        case TONEX_REVERB_ROOM:
                                            reverb_mix_param =
                                                TONEX_PARAM_REVERB_ROOM_MIX;
                                            break;

                                        case TONEX_REVERB_PLATE:
                                        default:
                                            reverb_mix_param =
                                                TONEX_PARAM_REVERB_PLATE_MIX;
                                            break;
                                    }

                                    encoders[i].value =
                                        (int)param_ptr[reverb_mix_param].Value;

                                    tonex_params_release_locked_access();

                                    ESP_LOGI(
                                        TAG,
                                        "Encoder 4 start Reverb: %d%%",
                                        encoders[i].value
                                    );
                                }
                            }

                            /* Encoder 5 = MASTER VOLUME */
                            if (encoders[i].active && i == 4)
                            {
                                tModellerParameter *param_ptr = NULL;

                                if (tonex_params_get_locked_access(&param_ptr) == ESP_OK)
                                {
                                    encoders[i].value =
                                        (int)param_ptr[TONEX_GLOBAL_MASTER_VOLUME].Value;

                                    tonex_params_release_locked_access();

                                    ESP_LOGI(
                                        TAG,
                                        "Encoder 5 start Master Volume: %d dB",
                                        encoders[i].value
                                    );
                                }
                            }

                            /* Encoder 5 popup */
                            if (i == 4)
                            {
                                UI_SetMasterPopup(encoders[i].active ? 1 : 0);
                            }

                            ESP_LOGI(
                                TAG,
                                "Encoder %d: %s",
                                i + 1,
                                encoders[i].active ? "ACTIVE" : "SAVED"
                            );
                        }
                    }
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}