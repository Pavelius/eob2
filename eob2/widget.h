#pragma once

#include "point.h"

enum resid : unsigned short;

void* choose_answer(point origin, resid background, int frame, int column_width);

void initialize_gui();
