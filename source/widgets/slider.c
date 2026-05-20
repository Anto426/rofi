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

#include "widgets/slider.h"
#include "keyb.h"
#include "theme.h"
#include "widgets/widget-internal.h"
#include "widgets/widget.h"
#include <float.h>
#include <glib.h>
#include <math.h>

#define DEFAULT_SLIDER_LENGTH 160
#define DEFAULT_SLIDER_TRACK_WIDTH 4
#define DEFAULT_SLIDER_HANDLE_WIDTH 12

struct _slider {
  widget widget;
  double min;
  double max;
  double value;
  double step;
  RofiOrientation orientation;
  RofiDistance track_width;
  RofiDistance handle_width;
  slider_changed_cb changed;
  void *changed_data;
};

static void slider_draw(widget *wid, cairo_t *draw);
static void slider_free(widget *wid);
static gboolean slider_motion_notify(widget *wid, gint x, gint y);
static int slider_get_desired_width(widget *wid, const int height);
static int slider_get_desired_height(widget *wid, const int width);
static WidgetTriggerActionResult
slider_trigger_action(widget *wid, MouseBindingMouseDefaultAction action,
                      gint x, gint y, void *user_data);

static double slider_normalize_value(const slider *sl, double value) {
  if (sl == NULL) {
    return 0.0;
  }

  value = CLAMP(value, sl->min, sl->max);
  if (sl->step > DBL_EPSILON && sl->max > sl->min) {
    value = sl->min + round((value - sl->min) / sl->step) * sl->step;
    value = CLAMP(value, sl->min, sl->max);
  }
  return value;
}

static gboolean slider_set_value_internal(slider *sl, double value,
                                          gboolean notify) {
  if (sl == NULL) {
    return FALSE;
  }

  double normalized = slider_normalize_value(sl, value);
  if (fabs(sl->value - normalized) <= DBL_EPSILON) {
    return FALSE;
  }

  sl->value = normalized;
  widget_queue_redraw(WIDGET(sl));
  if (notify && sl->changed != NULL) {
    sl->changed(sl, sl->value, sl->changed_data);
  }
  return TRUE;
}

static double slider_get_ratio(const slider *sl) {
  if (sl == NULL || sl->max <= sl->min) {
    return 0.0;
  }
  return (sl->value - sl->min) / (sl->max - sl->min);
}

static int slider_get_handle_size(const slider *sl) {
  return MAX(1, distance_get_pixel(sl->handle_width, sl->orientation));
}

static int slider_get_track_size(const slider *sl) {
  return MAX(1, distance_get_pixel(sl->track_width, sl->orientation));
}

static int slider_get_default_long_size(widget *wid, RofiOrientation ori) {
  RofiDistance d = rofi_theme_get_distance(wid, "length", DEFAULT_SLIDER_LENGTH);
  return MAX(1, distance_get_pixel(d, ori));
}

static void slider_draw_rounded_rect(cairo_t *draw, double x, double y,
                                     double width, double height,
                                     gboolean rounded) {
  if (width <= 0 || height <= 0) {
    return;
  }

  if (!rounded) {
    cairo_rectangle(draw, x, y, width, height);
    cairo_fill(draw);
    return;
  }

  double radius = MIN(width, height) / 2.0;
  cairo_new_sub_path(draw);
  cairo_arc(draw, x + width - radius, y + radius, radius, -G_PI_2, 0);
  cairo_arc(draw, x + width - radius, y + height - radius, radius, 0, G_PI_2);
  cairo_arc(draw, x + radius, y + height - radius, radius, G_PI_2, G_PI);
  cairo_arc(draw, x + radius, y + radius, radius, G_PI, 1.5 * G_PI);
  cairo_close_path(draw);
  cairo_fill(draw);
}

slider *slider_create(widget *parent, const char *name) {
  slider *sl = g_malloc0(sizeof(slider));
  widget_init(WIDGET(sl), parent, WIDGET_TYPE_SLIDER, name);
  sl->widget.cursor_type =
      rofi_theme_get_cursor_type(WIDGET(sl), "cursor", ROFI_CURSOR_POINTER);

  sl->min = rofi_theme_get_double(WIDGET(sl), "min", 0.0);
  sl->max = rofi_theme_get_double(WIDGET(sl), "max", 100.0);
  if (sl->min > sl->max) {
    double tmp = sl->min;
    sl->min = sl->max;
    sl->max = tmp;
  }
  sl->step = MAX(0.0, rofi_theme_get_double(WIDGET(sl), "step", 0.0));
  sl->orientation = rofi_theme_get_orientation(
      WIDGET(sl), "orientation", ROFI_ORIENTATION_HORIZONTAL);
  sl->track_width = rofi_theme_get_distance(WIDGET(sl), "track-width",
                                            DEFAULT_SLIDER_TRACK_WIDTH);
  sl->handle_width = rofi_theme_get_distance(WIDGET(sl), "handle-width",
                                             DEFAULT_SLIDER_HANDLE_WIDTH);
  sl->value = slider_normalize_value(
      sl, rofi_theme_get_double(WIDGET(sl), "value", sl->min));

  sl->widget.draw = slider_draw;
  sl->widget.free = slider_free;
  sl->widget.trigger_action = slider_trigger_action;
  sl->widget.motion_notify = slider_motion_notify;
  sl->widget.get_desired_width = slider_get_desired_width;
  sl->widget.get_desired_height = slider_get_desired_height;
  sl->widget.w = slider_get_desired_width(WIDGET(sl), 0);
  sl->widget.h = slider_get_desired_height(WIDGET(sl), sl->widget.w);

  return sl;
}

static void slider_free(widget *wid) { g_free((slider *)wid); }

void slider_set_range(slider *sl, double min, double max) {
  if (sl == NULL) {
    return;
  }
  if (min > max) {
    double tmp = min;
    min = max;
    max = tmp;
  }
  sl->min = min;
  sl->max = max;
  slider_set_value_internal(sl, sl->value, TRUE);
  widget_queue_redraw(WIDGET(sl));
}

double slider_get_min(const slider *sl) { return sl != NULL ? sl->min : 0.0; }

double slider_get_max(const slider *sl) { return sl != NULL ? sl->max : 0.0; }

void slider_set_value(slider *sl, double value) {
  slider_set_value_internal(sl, value, TRUE);
}

double slider_get_value(const slider *sl) {
  return sl != NULL ? sl->value : 0.0;
}

void slider_set_step(slider *sl, double step) {
  if (sl == NULL) {
    return;
  }
  sl->step = MAX(0.0, step);
  slider_set_value_internal(sl, sl->value, TRUE);
  widget_queue_redraw(WIDGET(sl));
}

double slider_get_step(const slider *sl) {
  return sl != NULL ? sl->step : 0.0;
}

void slider_set_orientation(slider *sl, RofiOrientation orientation) {
  if (sl == NULL || sl->orientation == orientation) {
    return;
  }
  sl->orientation = orientation;
  widget_update(WIDGET(sl));
  widget_update(WIDGET(sl)->parent);
  widget_queue_redraw(WIDGET(sl));
}

RofiOrientation slider_get_orientation(const slider *sl) {
  return sl != NULL ? sl->orientation : ROFI_ORIENTATION_HORIZONTAL;
}

void slider_set_changed_handler(slider *sl, slider_changed_cb cb,
                                void *user_data) {
  if (sl == NULL) {
    return;
  }
  sl->changed = cb;
  sl->changed_data = user_data;
}

double slider_get_value_from_position(const slider *sl, int x, int y) {
  if (sl == NULL || sl->max <= sl->min) {
    return sl != NULL ? sl->min : 0.0;
  }

  const widget *wid = WIDGET(sl);
  int handle = slider_get_handle_size(sl);
  double ratio = 0.0;

  if (sl->orientation == ROFI_ORIENTATION_HORIZONTAL) {
    int width = MAX(1, widget_padding_get_remaining_width(wid));
    handle = MIN(handle, width);
    double range = MAX(1.0, width - handle);
    double pos = x - widget_padding_get_left(wid) - handle / 2.0;
    ratio = CLAMP(pos / range, 0.0, 1.0);
  } else {
    int height = MAX(1, widget_padding_get_remaining_height(wid));
    handle = MIN(handle, height);
    double range = MAX(1.0, height - handle);
    double pos = y - widget_padding_get_top(wid) - handle / 2.0;
    ratio = CLAMP(pos / range, 0.0, 1.0);
  }

  return slider_normalize_value(sl, sl->min + ratio * (sl->max - sl->min));
}

static void slider_apply_position(slider *sl, int x, int y) {
  slider_set_value_internal(sl, slider_get_value_from_position(sl, x, y), TRUE);
}

static WidgetTriggerActionResult
slider_trigger_action(widget *wid, MouseBindingMouseDefaultAction action,
                      gint x, gint y, G_GNUC_UNUSED void *user_data) {
  slider *sl = (slider *)wid;
  switch (action) {
  case MOUSE_CLICK_DOWN:
    slider_apply_position(sl, x, y);
    return WIDGET_TRIGGER_ACTION_RESULT_GRAB_MOTION_BEGIN;
  case MOUSE_CLICK_UP:
    slider_apply_position(sl, x, y);
    return WIDGET_TRIGGER_ACTION_RESULT_GRAB_MOTION_END;
  case MOUSE_DCLICK_DOWN:
  case MOUSE_DCLICK_UP:
    break;
  }
  return WIDGET_TRIGGER_ACTION_RESULT_IGNORED;
}

static gboolean slider_motion_notify(widget *wid, gint x, gint y) {
  slider_apply_position((slider *)wid, x, y);
  return TRUE;
}

static int slider_get_desired_width(widget *wid,
                                    G_GNUC_UNUSED const int height) {
  slider *sl = (slider *)wid;
  RofiDistance w = rofi_theme_get_distance(wid, "width", 0);
  int width = distance_get_pixel(w, ROFI_ORIENTATION_HORIZONTAL);
  if (width > 0) {
    return width;
  }

  if (sl->orientation == ROFI_ORIENTATION_HORIZONTAL) {
    width = slider_get_default_long_size(wid, ROFI_ORIENTATION_HORIZONTAL);
  } else {
    width = MAX(slider_get_handle_size(sl), slider_get_track_size(sl));
  }
  return width + widget_padding_get_padding_width(wid);
}

static int slider_get_desired_height(widget *wid,
                                     G_GNUC_UNUSED const int width) {
  slider *sl = (slider *)wid;
  RofiDistance h = rofi_theme_get_distance(wid, "height", 0);
  int height = distance_get_pixel(h, ROFI_ORIENTATION_VERTICAL);
  if (height > 0) {
    return height;
  }

  if (sl->orientation == ROFI_ORIENTATION_VERTICAL) {
    height = slider_get_default_long_size(wid, ROFI_ORIENTATION_VERTICAL);
  } else {
    height = MAX(slider_get_handle_size(sl), slider_get_track_size(sl));
  }
  return height + widget_padding_get_padding_height(wid);
}

static void slider_draw(widget *wid, cairo_t *draw) {
  slider *sl = (slider *)wid;
  const int left = widget_padding_get_left(wid);
  const int top = widget_padding_get_top(wid);
  const int width = MAX(1, widget_padding_get_remaining_width(wid));
  const int height = MAX(1, widget_padding_get_remaining_height(wid));
  const gboolean track_rounded =
      rofi_theme_get_boolean(wid, "track-rounded-corners", TRUE);
  const gboolean handle_rounded =
      rofi_theme_get_boolean(wid, "handle-rounded-corners", TRUE);

  int handle = slider_get_handle_size(sl);
  int track = slider_get_track_size(sl);
  const double ratio = slider_get_ratio(sl);

  cairo_set_source_rgba(draw, 0.35, 0.35, 0.35, 0.55);
  rofi_theme_get_color(wid, "track-color", draw);

  if (sl->orientation == ROFI_ORIENTATION_HORIZONTAL) {
    handle = MIN(handle, MIN(width, height));
    track = MIN(track, height);
    const double track_x = left + handle / 2.0;
    const double track_y = top + (height - track) / 2.0;
    const double track_length = MAX(0.0, width - handle);
    const double center = track_x + ratio * track_length;

    slider_draw_rounded_rect(draw, track_x, track_y, track_length, track,
                             track_rounded);

    cairo_set_source_rgba(draw, 0.45, 0.60, 0.85, 1.0);
    rofi_theme_get_color(wid, "fill-color", draw);
    slider_draw_rounded_rect(draw, track_x, track_y, center - track_x, track,
                             track_rounded);

    cairo_set_source_rgba(draw, 0.90, 0.90, 0.90, 1.0);
    rofi_theme_get_color(wid, "handle-color", draw);
    slider_draw_rounded_rect(draw, center - handle / 2.0,
                             top + (height - handle) / 2.0, handle, handle,
                             handle_rounded);
  } else {
    handle = MIN(handle, MIN(width, height));
    track = MIN(track, width);
    const double track_x = left + (width - track) / 2.0;
    const double track_y = top + handle / 2.0;
    const double track_length = MAX(0.0, height - handle);
    const double center = track_y + ratio * track_length;

    slider_draw_rounded_rect(draw, track_x, track_y, track, track_length,
                             track_rounded);

    cairo_set_source_rgba(draw, 0.45, 0.60, 0.85, 1.0);
    rofi_theme_get_color(wid, "fill-color", draw);
    slider_draw_rounded_rect(draw, track_x, track_y, track, center - track_y,
                             track_rounded);

    cairo_set_source_rgba(draw, 0.90, 0.90, 0.90, 1.0);
    rofi_theme_get_color(wid, "handle-color", draw);
    slider_draw_rounded_rect(draw, left + (width - handle) / 2.0,
                             center - handle / 2.0, handle, handle,
                             handle_rounded);
  }
}
