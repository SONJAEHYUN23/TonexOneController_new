#ifndef ENCODER_CONTROL_H
#define ENCODER_CONTROL_H

#define ENCODER1_A   0
#define ENCODER1_B   1
#define ENCODER1_SW  2

#define ENCODER2_A   3
#define ENCODER2_B   4
#define ENCODER2_SW  5

#define ENCODER3_A   6
#define ENCODER3_B   7
#define ENCODER3_SW  8

#define ENCODER4_A   9
#define ENCODER4_B   10
#define ENCODER4_SW  11

#define ENCODER5_A   12
#define ENCODER5_B   13
#define ENCODER5_SW  14


void encoder_control_init(void);
void encoder_control_task(void *arg);

#endif
