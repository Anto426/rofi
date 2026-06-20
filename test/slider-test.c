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

#include "display.h"
#include "helper.h"
#include "rofi-icon-fetcher.h"
#include "rofi.h"
#include "xrmoptions.h"
#include <assert.h>
#include <glib.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <widgets/icon.h>
#include <widgets/listview.h>
#include <widgets/slider.h>
#include <widgets/textbox.h>
#include <widgets/widget-internal.h>
#include <widgets/widget.h>

unsigned int test = 0;
#define TASSERT(a)                                                             \
  {                                                                            \
    assert(a);                                                                 \
    printf("Test %3u passed (%s)\n", ++test, #a);                              \
  }

#define TASSERTD(a, b)                                                         \
  {                                                                            \
    double va = (a);                                                           \
    double vb = (b);                                                           \
    if (fabs(va - vb) <= 0.000001) {                                           \
      printf("Test %u passed (%s == %s) (%f == %f)\n", ++test, #a, #b, va,     \
             vb);                                                             \
    } else {                                                                   \
      printf("Test %u failed (%s == %s) (%f != %f)\n", ++test, #a, #b, va,     \
             vb);                                                             \
      abort();                                                                 \
    }                                                                          \
  }

int rofi_is_in_dmenu_mode = 0;
ThemeWidget *rofi_configuration = NULL;

uint32_t rofi_icon_fetcher_query(G_GNUC_UNUSED const char *name,
                                 G_GNUC_UNUSED const int size) {
  return 0;
}
uint32_t rofi_icon_fetcher_query_advanced(G_GNUC_UNUSED const char *name,
                                          G_GNUC_UNUSED const int wsize,
                                          G_GNUC_UNUSED const int hsize) {
  return 0;
}

cairo_surface_t *rofi_icon_fetcher_get(G_GNUC_UNUSED const uint32_t uid) {
  return NULL;
}

int monitor_active(G_GNUC_UNUSED workarea *mon) { return 0; }

char *helper_get_theme_path(const char *file, G_GNUC_UNUSED const char **ext,
                            G_GNUC_UNUSED const char *parent_file) {
  return g_strdup(file);
}
gboolean config_parse_set_property(G_GNUC_UNUSED const Property *p,
                                   G_GNUC_UNUSED char **error) {
  return FALSE;
}
void rofi_add_error_message(G_GNUC_UNUSED GString *msg) {}
void rofi_add_warning_message(G_GNUC_UNUSED GString *msg) {}

char *rofi_expand_path(G_GNUC_UNUSED const char *path) { return NULL; }
double textbox_get_estimated_char_height(void) { return 16; }
double textbox_get_estimated_ch(void) { return 8.0; }

void listview_set_selected(G_GNUC_UNUSED listview *lv,
                           G_GNUC_UNUSED unsigned int selected) {}
void rofi_view_get_current_monitor(G_GNUC_UNUSED int *width,
                                   G_GNUC_UNUSED int *height) {}

static double callback_value = 0.0;
static unsigned int callback_count = 0;

static void slider_changed(G_GNUC_UNUSED slider *sl, double value,
                           G_GNUC_UNUSED void *user_data) {
  callback_value = value;
  callback_count++;
}

int main(G_GNUC_UNUSED int argc, G_GNUC_UNUSED char **argv) {
  slider *sl = slider_create(NULL, "slider");
  widget_resize(WIDGET(sl), 100, 20);

  TASSERTD(slider_set_range(NULL, 0.0, 10.0), 0.0);
  TASSERTD(slider_set_value(NULL, 10.0), 0.0);
  TASSERTD(slider_set_step(NULL, 1.0), 0.0);
  slider_set_orientation(NULL, ROFI_ORIENTATION_VERTICAL);
  slider_set_changed_handler(NULL, slider_changed, NULL);

  TASSERTD(slider_get_min(sl), 0.0);
  TASSERTD(slider_get_max(sl), 100.0);
  TASSERTD(slider_get_value(sl), 0.0);

  TASSERTD(slider_set_range(sl, 0.0, 100.0), 0.0);
  TASSERTD(slider_set_value(sl, 40.0), 40.0);
  TASSERTD(slider_get_value(sl), 40.0);
  TASSERTD(slider_set_value(sl, -10.0), 0.0);
  TASSERTD(slider_get_value(sl), 0.0);
  TASSERTD(slider_set_value(sl, 110.0), 100.0);
  TASSERTD(slider_get_value(sl), 100.0);

  TASSERTD(slider_set_step(sl, 5.0), 100.0);
  TASSERTD(slider_get_step(sl), 5.0);
  TASSERTD(slider_set_value(sl, 42.0), 40.0);
  TASSERTD(slider_get_value(sl), 40.0);
  TASSERTD(slider_set_value(sl, 43.0), 45.0);
  TASSERTD(slider_get_value(sl), 45.0);

  slider_set_changed_handler(sl, slider_changed, NULL);
  TASSERTD(slider_set_value(sl, 50.0), 50.0);
  TASSERTD(callback_value, 50.0);
  TASSERT(callback_count == 1);
  TASSERTD(slider_set_value(sl, 50.0), 50.0);
  TASSERT(callback_count == 1);

  TASSERTD(slider_set_step(sl, 0.0), 50.0);
  TASSERTD(slider_get_value_from_position(sl, 0, 10), 0.0);
  TASSERTD(slider_get_value_from_position(sl, 50, 10), 50.0);
  TASSERTD(slider_get_value_from_position(sl, 99, 10), 100.0);

  TASSERT(widget_trigger_action(WIDGET(sl), MOUSE_CLICK_DOWN, 50, 10) ==
          WIDGET_TRIGGER_ACTION_RESULT_GRAB_MOTION_BEGIN);
  TASSERTD(slider_get_value(sl), 50.0);
  TASSERT(widget_motion_notify(WIDGET(sl), 99, 10));
  TASSERTD(slider_get_value(sl), 100.0);
  TASSERT(widget_trigger_action(WIDGET(sl), MOUSE_CLICK_UP, 0, 10) ==
          WIDGET_TRIGGER_ACTION_RESULT_GRAB_MOTION_END);
  TASSERTD(slider_get_value(sl), 0.0);

  slider_set_orientation(sl, ROFI_ORIENTATION_VERTICAL);
  widget_resize(WIDGET(sl), 20, 100);
  TASSERT(slider_get_orientation(sl) == ROFI_ORIENTATION_VERTICAL);
  TASSERTD(slider_get_value_from_position(sl, 10, 0), 0.0);
  TASSERTD(slider_get_value_from_position(sl, 10, 50), 50.0);
  TASSERTD(slider_get_value_from_position(sl, 10, 99), 100.0);

  TASSERTD(slider_set_range(sl, 100.0, 0.0), 0.0);
  TASSERTD(slider_get_min(sl), 0.0);
  TASSERTD(slider_get_max(sl), 100.0);

  widget_free(WIDGET(sl));

  sl = slider_create_with_value(NULL, "slider", 37.0);
  TASSERTD(slider_get_value(sl), 37.0);
  widget_free(WIDGET(sl));

  rofi_theme_parse_string("slider-themed {"
                          "  min: -10;"
                          "  max: 10;"
                          "  value: 4.2;"
                          "  step: 0.5;"
                          "  orientation: vertical;"
                          "  width: 24px;"
                          "  height: 120px;"
                          "  padding: 2px;"
                          "  track-width: 6px;"
                          "  handle-width: 14px;"
                          "}");
  sl = slider_create(NULL, "slider-themed");
  TASSERTD(slider_get_min(sl), -10.0);
  TASSERTD(slider_get_max(sl), 10.0);
  TASSERTD(slider_get_value(sl), 4.0);
  TASSERTD(slider_get_step(sl), 0.5);
  TASSERT(slider_get_orientation(sl) == ROFI_ORIENTATION_VERTICAL);
  TASSERT(widget_get_desired_width(WIDGET(sl), 0) == 24);
  TASSERT(widget_get_desired_height(WIDGET(sl), 24) == 120);
  widget_resize(WIDGET(sl), 24, 120);
  TASSERTD(slider_get_value_from_position(sl, 12, 60), 0.0);
  widget_free(WIDGET(sl));

  sl = slider_create_with_value(NULL, "slider-themed", 11.0);
  TASSERTD(slider_get_value(sl), 10.0);
  widget_free(WIDGET(sl));
}
