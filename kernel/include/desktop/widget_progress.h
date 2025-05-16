#pragma once

#include "desktop/widget.h"
#include "gui/basics.h"
#include "common.h"

typedef struct Widget_Progress {
    size_t current;
} Widget_Progress_t;

void widget_progress_renderer(struct Widget* this, SAYORI_UNUSED struct Window* container);
Widget_t* new_widget_progress();
void destroy_widget_progress(Widget_t* widget);