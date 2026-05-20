/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2026 Qball Cow <qball@gmpclient.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef ROFI_SLIDER_H
#define ROFI_SLIDER_H

#include "rofi-types.h"
#include "widgets/widget.h"

/**
 * @defgroup Slider Slider
 * @ingroup widget
 *
 * A draggable, generic value slider widget.
 *
 * @{
 */

typedef struct _slider slider;

/**
 * @param sl The slider that changed.
 * @param value The new slider value.
 * @param user_data User data passed to slider_set_changed_handler().
 */
typedef void (*slider_changed_cb)(slider *sl, double value, void *user_data);

/**
 * @param parent The parent widget.
 * @param name The name of the widget.
 *
 * Create a new slider.
 *
 * Theme properties:
 * - min: minimum value, default 0.
 * - max: maximum value, default 100.
 * - value: initial value, default min.
 * - step: snap interval, default 0 for continuous values.
 * - orientation: horizontal or vertical, default horizontal.
 * - width/height: preferred size.
 * - length: preferred long side when width/height is not set.
 * - track-width: visual track thickness.
 * - handle-width: visual handle size.
 * - track-color/fill-color/handle-color: visual colors.
 */
slider *slider_create(widget *parent, const char *name);

/**
 * @param parent The parent widget.
 * @param name The name of the widget.
 * @param value Initial value supplied by the application.
 *
 * Create a new slider and override the theme `value` with an application
 * supplied initial value. The value is still clamped to the theme/API range and
 * snapped to the configured step.
 */
slider *slider_create_with_value(widget *parent, const char *name,
                                 double value);

/**
 * @param sl Slider object.
 * @param min Minimum value.
 * @param max Maximum value.
 *
 * Set the slider range. If min is greater than max they are swapped.
 *
 * @returns the current value after applying the new range.
 */
double slider_set_range(slider *sl, double min, double max);

double slider_get_min(const slider *sl);
double slider_get_max(const slider *sl);

/**
 * @param sl Slider object.
 * @param value New value.
 *
 * Set the slider value, clamped to the current range and snapped to the step.
 *
 * @returns the normalized value stored by the slider.
 */
double slider_set_value(slider *sl, double value);

double slider_get_value(const slider *sl);

/**
 * @param sl Slider object.
 * @param step Step interval. Values <= 0 disable snapping.
 *
 * @returns the current value after applying the new step.
 */
double slider_set_step(slider *sl, double step);

double slider_get_step(const slider *sl);

void slider_set_orientation(slider *sl, RofiOrientation orientation);
RofiOrientation slider_get_orientation(const slider *sl);

/**
 * @param sl Slider object.
 * @param cb Callback invoked when the value changes.
 * @param user_data User data passed to the callback.
 *
 * This is the communication surface for consumers: the widget stays generic,
 * and the owner decides what a changed value means.
 */
void slider_set_changed_handler(slider *sl, slider_changed_cb cb,
                                void *user_data);

/**
 * @param sl Slider object.
 * @param x X position relative to the slider widget.
 * @param y Y position relative to the slider widget.
 *
 * Convert a widget-relative pointer position into a clamped slider value.
 */
double slider_get_value_from_position(const slider *sl, int x, int y);

/**@}*/
#endif // ROFI_SLIDER_H
