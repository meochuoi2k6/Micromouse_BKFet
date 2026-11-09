#ifndef MOVE_H
#define MOVE_H
#include "main.h"
typedef struct {
    int32_t base_ccr; // Base PWM duty cycle
    GPIO_TypeDef* IN1_Motor1_Port;
    uint16_t IN1_Motor1_Pin;
    GPIO_TypeDef* IN2_Motor1_Port;
    uint16_t IN2_Motor1_Pin;
    GPIO_TypeDef* IN1_Motor2_Port;
    uint16_t IN1_Motor2_Pin;
    GPIO_TypeDef* IN2_Motor2_Port;
    uint16_t IN2_Motor2_Pin;
    TIM_HandleTypeDef* htim_pwm_Motor1;
    uint32_t pwm_channel_Motor1;
    TIM_HandleTypeDef* htim_pwm_Motor2;
    uint32_t pwm_channel_Motor2;
    TIM_HandleTypeDef* htim_encoder_Motor1;
    uint32_t encoder_channel_Motor1;
    TIM_HandleTypeDef* htim_encoder_Motor2;
    uint32_t encoder_channel_Motor2;
    int32_t pwm1;
    int32_t pwm2;
}Mouse;

// Latest signed encoder count deltas, exposed for VS Code Live Watch.
extern volatile int32_t encoder_motor1_delta_debug;
extern volatile int32_t encoder_motor2_delta_debug;

void move_forward(Mouse* mouse);
void move_backward(Mouse* mouse);
void turn_left_90(Mouse* mouse);
void turn_right_90(Mouse* mouse);
void motion_reset(Mouse* mouse);
void pwm_Init(Mouse* mouse);
void encoder_Init(Mouse* mouse);
#endif
