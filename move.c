#include "move.h"

// Latest signed encoder samples used by the motor speed correction.
volatile int32_t encoder_motor1_delta_debug = 0;
volatile int32_t encoder_motor2_delta_debug = 0;

static int32_t clamp_compare(int32_t value, uint32_t arr) {
    if (value < 0) return 0;
    if ((uint32_t)value > arr) return (int32_t)arr;
    return value;
}

static void calculate_pwm(Mouse* mouse) {
    // Encoder counters are 16-bit. Interpret the sampled counter as a signed
    // delta so down-counting during reverse motion is represented correctly.
    int32_t enc1 = (int16_t)__HAL_TIM_GET_COUNTER(mouse->htim_encoder_Motor1);
    int32_t enc2 = (int16_t)__HAL_TIM_GET_COUNTER(mouse->htim_encoder_Motor2);
    encoder_motor1_delta_debug = enc1;
    encoder_motor2_delta_debug = enc2;
    __HAL_TIM_SET_COUNTER(mouse->htim_encoder_Motor1, 0);
    __HAL_TIM_SET_COUNTER(mouse->htim_encoder_Motor2, 0);

    // Compare wheel speed magnitudes so opposite encoder polarity does not
    // reverse the correction when the mouse changes direction.
    int32_t speed1 = (enc1 < 0) ? -enc1 : enc1;
    int32_t speed2 = (enc2 < 0) ? -enc2 : enc2;
    int32_t error = speed1 - speed2;
    int32_t correction = error * 2;

    uint32_t arr1 = __HAL_TIM_GET_AUTORELOAD(mouse->htim_pwm_Motor1);
    uint32_t arr2 = __HAL_TIM_GET_AUTORELOAD(mouse->htim_pwm_Motor2);
    mouse->pwm1 = clamp_compare(mouse->base_ccr - correction, arr1);
    mouse->pwm2 = clamp_compare(mouse->base_ccr + correction, arr2);
}
void motion_reset(Mouse* mouse) {
    __HAL_TIM_SET_COMPARE(mouse->htim_pwm_Motor1, mouse->pwm_channel_Motor1, 0);
    __HAL_TIM_SET_COMPARE(mouse->htim_pwm_Motor2, mouse->pwm_channel_Motor2, 0);
    mouse->pwm1 = 0;
    mouse->pwm2 = 0;
    __HAL_TIM_SET_COUNTER(mouse->htim_encoder_Motor1, 0);
    __HAL_TIM_SET_COUNTER(mouse->htim_encoder_Motor2, 0);
    encoder_motor1_delta_debug = 0;
    encoder_motor2_delta_debug = 0;
}
void move_forward(Mouse* mouse) {
    calculate_pwm(mouse);

    HAL_GPIO_WritePin(mouse->IN1_Motor1_Port, mouse->IN1_Motor1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(mouse->IN2_Motor1_Port, mouse->IN2_Motor1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(mouse->IN1_Motor2_Port, mouse->IN1_Motor2_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(mouse->IN2_Motor2_Port, mouse->IN2_Motor2_Pin, GPIO_PIN_RESET);

    __HAL_TIM_SET_COMPARE(mouse->htim_pwm_Motor1, mouse->pwm_channel_Motor1, mouse->pwm1);
    __HAL_TIM_SET_COMPARE(mouse->htim_pwm_Motor2, mouse->pwm_channel_Motor2, mouse->pwm2);
}

void move_backward(Mouse* mouse) {
    calculate_pwm(mouse);

    HAL_GPIO_WritePin(mouse->IN1_Motor1_Port, mouse->IN1_Motor1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(mouse->IN2_Motor1_Port, mouse->IN2_Motor1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(mouse->IN1_Motor2_Port, mouse->IN1_Motor2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(mouse->IN2_Motor2_Port, mouse->IN2_Motor2_Pin, GPIO_PIN_SET);

    __HAL_TIM_SET_COMPARE(mouse->htim_pwm_Motor1, mouse->pwm_channel_Motor1, mouse->pwm1);
    __HAL_TIM_SET_COMPARE(mouse->htim_pwm_Motor2, mouse->pwm_channel_Motor2, mouse->pwm2);
}
static uint32_t encoder_abs(TIM_HandleTypeDef* htim) {
    int32_t count = (int16_t)__HAL_TIM_GET_COUNTER(htim);
    return (count < 0) ? (uint32_t)(-count) : (uint32_t)(count);
}
void turn_left_90(Mouse* mouse){
    const uint32_t target_motor1 = 420;
    const uint32_t target_motor2 = 448;
    HAL_GPIO_WritePin(mouse->IN1_Motor1_Port, mouse->IN1_Motor1_Pin, 0);
    HAL_GPIO_WritePin(mouse->IN2_Motor1_Port, mouse->IN2_Motor1_Pin, 1);
    HAL_GPIO_WritePin(mouse->IN1_Motor2_Port, mouse->IN1_Motor2_Pin, 1);
    HAL_GPIO_WritePin(mouse->IN2_Motor2_Port, mouse->IN2_Motor2_Pin, 0);
    //reset encoder -> 0
    __HAL_TIM_SET_COUNTER(mouse->htim_encoder_Motor1, 0);
    __HAL_TIM_SET_COUNTER(mouse->htim_encoder_Motor2, 0);

    __HAL_TIM_SET_COMPARE(mouse->htim_pwm_Motor1, mouse->pwm_channel_Motor1, mouse->base_ccr);
    __HAL_TIM_SET_COMPARE(mouse->htim_pwm_Motor2, mouse->pwm_channel_Motor2, mouse->base_ccr);
    uint8_t m1Flag = 0;
    uint8_t m2Flag = 0;
    uint32_t start_tick = HAL_GetTick();
    while (!m1Flag || !m2Flag) {
        uint32_t count1 = encoder_abs(mouse->htim_encoder_Motor1);
        uint32_t count2 = encoder_abs(mouse->htim_encoder_Motor2);

        if (!m1Flag && count1 >= target_motor1) {
            __HAL_TIM_SET_COMPARE(mouse->htim_pwm_Motor1, mouse->pwm_channel_Motor1, 0);
            m1Flag = 1;
        }

        if (!m2Flag && count2 >= target_motor2) {
            __HAL_TIM_SET_COMPARE(mouse->htim_pwm_Motor2, mouse->pwm_channel_Motor2, 0);
            m2Flag = 1;
        }

        // Fail safe if an encoder does not reach its target.
        if ((HAL_GetTick() - start_tick) >= 3000U) {
            break;
        }
    }
    __HAL_TIM_SET_COMPARE(mouse->htim_pwm_Motor1, mouse->pwm_channel_Motor1, 0);
    __HAL_TIM_SET_COMPARE(mouse->htim_pwm_Motor2, mouse->pwm_channel_Motor2, 0);
}
void turn_right_90(Mouse* mouse) {
    const uint32_t target1 = 420;
    const uint32_t target2 = 448;
    HAL_GPIO_WritePin(mouse->IN1_Motor1_Port, mouse->IN1_Motor1_Pin, 1);
    HAL_GPIO_WritePin(mouse->IN2_Motor1_Port, mouse->IN2_Motor1_Pin, 0);
    HAL_GPIO_WritePin(mouse->IN1_Motor2_Port, mouse->IN1_Motor2_Pin, 0);
    HAL_GPIO_WritePin(mouse->IN2_Motor2_Port, mouse->IN2_Motor2_Pin, 1);

    __HAL_TIM_SET_COUNTER(mouse->htim_encoder_Motor1, 0);
    __HAL_TIM_SET_COUNTER(mouse->htim_encoder_Motor2, 0);

    __HAL_TIM_SET_COMPARE(mouse->htim_pwm_Motor1, mouse->pwm_channel_Motor1, mouse->base_ccr);
    __HAL_TIM_SET_COMPARE(mouse->htim_pwm_Motor2, mouse->pwm_channel_Motor2, mouse->base_ccr);

    uint8_t m1Flag = 0;
    uint8_t m2Flag = 0;
    uint32_t start_tick = HAL_GetTick();
    while (!m1Flag || m2Flag) {
        uint32_t count1 = encoder_abs(mouse->htim_encoder_Motor1);
        uint32_t count2 = encoder_abs(mouse->htim_encoder_Motor2);

        if (!m1Flag && count1 >= target1) {
            __HAL_TIM_SET_COMPARE(mouse->htim_pwm_Motor1, mouse->pwm_channel_Motor1, 0);
            m1Flag = 1;
        }

        if (!m2Flag && count2 >= target2) {
            __HAL_TIM_SET_COMPARE(mouse->htim_pwm_Motor2, mouse->pwm_channel_Motor2, 0);
            m2Flag = 1;
        }

        if (HAL_GetTick() - start_tick >= 3000U) {
            break;
        }
    }
    __HAL_TIM_SET_COMPARE(mouse->htim_pwm_Motor1, mouse->pwm_channel_Motor1, 0);
    __HAL_TIM_SET_COMPARE(mouse->htim_pwm_Motor2, mouse->pwm_channel_Motor2, 0);
}
void pwm_Init(Mouse* mouse) {
    if (HAL_TIM_PWM_Start(mouse->htim_pwm_Motor1, mouse->pwm_channel_Motor1) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_TIM_PWM_Start(mouse->htim_pwm_Motor2, mouse->pwm_channel_Motor2) != HAL_OK) {
        Error_Handler();
    }
}

void encoder_Init(Mouse* mouse) {
    if (HAL_TIM_Encoder_Start(mouse->htim_encoder_Motor1, mouse->encoder_channel_Motor1) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_TIM_Encoder_Start(mouse->htim_encoder_Motor2, mouse->encoder_channel_Motor2) != HAL_OK) {
        Error_Handler();
    }
}
