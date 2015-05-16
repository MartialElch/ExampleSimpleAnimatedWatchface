#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer;

static PropertyAnimation *s_animation;

static int s_current_stage = 0;
static bool s_animation_running;

// prototype declarations
static void next_animation();
static void update_time();

static void anim_stopped_handler(Animation *animation, bool finished, void *context) {
  // free the animation
  property_animation_destroy(s_animation);

  // schedule the next one, unless the app is exiting
  if (finished) {
    s_animation_running = false;

    if (s_current_stage == 0) {
      s_current_stage = 1;
      next_animation();
    } else if (s_current_stage == 1) {
      s_current_stage = 2;
      update_time();
      next_animation();
    } else if (s_current_stage == 2) {
      s_current_stage = 3;
      next_animation();
    } else {
      s_current_stage = 0;
    }
  }
}
static void update_time() {
  // get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // create a long-lived buffer
  static char buffer[] = "00:00";
  // use 24 hour format
  strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);

  // display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);
}

static void next_animation() {
  int target;
  int DURATION;

  // don't run animation twice
  if (s_animation_running) {
    return;
  }
  s_animation_running = true;

  switch (s_current_stage) {
    case 0:
      target = 0;
      DURATION = 0;
      break;
    case 1:
      target = -144;
      DURATION = 2000;
      break;
    case 2:
      target = 144;
      DURATION = 0;
      break;
    case 3:
      target = 0;
      DURATION = 2000;
      break;
    default:
      target = 0;
      DURATION = 0;
      break;
  }
  GRect from_frame = layer_get_frame((Layer*)s_time_layer);
  GRect to_frame = GRect(target, from_frame.origin.y, from_frame.size.w, from_frame.size.h);

  // create the animation
  s_animation = property_animation_create_layer_frame((Layer*)s_time_layer, &from_frame, &to_frame);
  animation_set_duration((Animation*)s_animation, DURATION);
  animation_set_delay((Animation*)s_animation, 300);
  animation_set_curve((Animation*)s_animation, AnimationCurveEaseInOut);
  animation_set_handlers((Animation*)s_animation, (AnimationHandlers) {
    .stopped = anim_stopped_handler
  }, NULL);
  // schedule to occur ASAP with default settings
  animation_schedule((Animation*) s_animation);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "update time %d:%d:%d %d", tick_time->tm_hour, tick_time->tm_min, tick_time->tm_sec, tick_time->tm_mday);

  // start animation loop
  next_animation();
}

static void main_window_load(Window *window) {
  // create time TextLayer
  s_time_layer = text_layer_create(GRect(0, 55, 144, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  // improve the layout to be more like a watchface
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  // add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
}

  static void main_window_unload(Window *window) {
  // destroy TextLayer
  text_layer_destroy(s_time_layer);
}

static void init() {
  // create main Window element and assign to pointer
  s_main_window = window_create();
  // set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);

  // initialize
  s_animation_running = false;

  // make sure the time is displayed from the start
  update_time();

  // register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit() {
  // destroy window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}