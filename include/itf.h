#pragma once

#include <stdbool.h>


typedef struct {
    int32_t temperature;
    int32_t relative_humdity;
} __attribute__((packed)) itf_measurements_t;


bool itf_send_nop(void);
bool itf_send_measurements(itf_measurements_t* measurements);
void itf_iterate(void);
