#include <pebble.h>

static Window *window;
static TextLayer *time_layer;
static TextLayer *date_layer;
static TextLayer *btc_layer;
static Layer *rounded_layer;

static AppSync sync;
static uint8_t sync_buffer[64];

enum BtcKey {
  BTC_PRICE_KEY = 0x0 // TUPLE_CSTRING
};

static void sync_error_callback(DictionaryResult dict_error,
                                AppMessageResult app_message_error,
                                void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

static void sync_tuple_changed_callback(const uint32_t key,
                                        const Tuple *new_tuple,
                                        const Tuple *old_tuple, void *context) {
  switch (key) {
    case BTC_PRICE_KEY:
      text_layer_set_text(btc_layer, new_tuple->value->cstring);
      break;
  }
}

static void handle_timechanges(struct tm *tick_time, TimeUnits units_changed) {
  static char time_buffer[10];
  static char date_buffer[15];

  strftime(time_buffer, sizeof(time_buffer), "%k:%M", tick_time);
  strftime(date_buffer, sizeof(date_buffer), "%A %e", tick_time);

  text_layer_set_text(time_layer, time_buffer);
  text_layer_set_text(date_layer, date_buffer);
}

static void send_cmd(void) {
  Tuplet value = TupletInteger(1, 1);

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (iter == NULL) {
    return;
  }

  dict_write_tuplet(iter, &value);
  dict_write_end(iter);

  app_message_outbox_send();
}

static void rounded_layer_update_callback(Layer *me, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(0, 112, 144, 56), 6, GCornersAll);
}

static void window_load(Window *window) {
  window_set_background_color(window, GColorBlack);
  Layer *root_layer = window_get_root_layer(window);

  // Create the Time text_layer
  time_layer = text_layer_create(GRect(0, 0, 144, 56));
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  text_layer_set_font(time_layer,
                      fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
  text_layer_set_background_color(time_layer, GColorBlack);
  text_layer_set_text_color(time_layer, GColorWhite);
  layer_add_child(root_layer, text_layer_get_layer(time_layer));


  // Create the Date text_layer
  date_layer = text_layer_create(GRect(0, 56, 144, 56));
  text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
  text_layer_set_font(date_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_24 ));
  text_layer_set_background_color(date_layer, GColorBlack);
  text_layer_set_text_color(date_layer, GColorWhite);
  layer_add_child(root_layer, text_layer_get_layer(date_layer));

  Layer *window_layer = root_layer;
  GRect frame = layer_get_frame(window_layer);

  rounded_layer = layer_create(frame);
  layer_set_update_proc(rounded_layer, rounded_layer_update_callback);
  layer_add_child(window_layer, rounded_layer);

  // Create the btc text_layer
  btc_layer = text_layer_create(GRect(8, 124, 128, 36));
  text_layer_set_text_alignment(btc_layer, GTextAlignmentCenter);
  text_layer_set_font(btc_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  layer_add_child(root_layer, text_layer_get_layer(btc_layer));

  // Subscribe to tick_timer_service
  tick_timer_service_subscribe(MINUTE_UNIT, handle_timechanges);

  // Initialize app_message
  const int inbound_size = 128;
  const int outbound_size = 128;
  app_message_open(inbound_size, outbound_size);

  Tuplet initial_values[] = {
    TupletCString(BTC_PRICE_KEY, "$ ---")
  };

  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values,
                ARRAY_LENGTH(initial_values),
    sync_tuple_changed_callback, sync_error_callback, NULL);

  send_cmd();

  window_set_background_color(window, GColorBlack);

  // Push the window
  window_stack_push(window, true);
}

static void window_unload(Window *window) {
  // Deinit sync
  app_sync_deinit(&sync);

  layer_destroy(rounded_layer);

  // Destroy the text layer
  text_layer_destroy(time_layer);

  // Destroy the btc_layer
  text_layer_destroy(btc_layer);

  // Destroy the date_layer
  text_layer_destroy(date_layer);
}

static void init(void) {
  // Create a window
  window = window_create();

  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  const bool animated = false;
  window_stack_push(window, animated);
}

static void deinit(void) {
  // Destroy the window
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
