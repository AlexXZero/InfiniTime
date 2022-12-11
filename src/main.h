#pragma once

#include <nrfx_gpiote.h>

void nrfx_gpiote_evt_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action);
