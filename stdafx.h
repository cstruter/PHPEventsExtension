#pragma once

#include "zend_config.w32.h"
#include "php.h"

#define event_extname    "Events"
#define event_extver    "0.1"

extern zend_module_entry event_module_entry;
#define event_ptr &event_module_entry