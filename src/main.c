#include "pebble.h"

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;

static int pox_x;
static int pox_y;
static int locOK;

char *sys_locale ;
//Pour la carte

static GBitmap *s_bitmap_sea;
static GBitmap *s_bitmap_map;
static GBitmap *s_bitmap_cutted_map;

static BitmapLayer *s_bitmap_layer_map;

#ifndef PBL_PLATFORM_APLITE
  static GBitmap *s_bitmap_circle;
  static BitmapLayer *s_bitmap_layer_circle;
  static BitmapLayer *s_bitmap_layer_sea;
#endif
  
static Layer *s_layer_lines;
 
#define KEY_LONGITUDE 0
#define KEY_LATITUDE 1

#define COMPOSITING PBL_IF_COLOR_ELSE(GCompOpSet, GCompOpAssign)
#define MAP_RECT_X PBL_IF_COLOR_ELSE(11, 12)
 
char *translate_error(AppMessageResult result) {
  switch (result) {
    case APP_MSG_OK: return "APP_MSG_OK";
    case APP_MSG_SEND_TIMEOUT: return "APP_MSG_SEND_TIMEOUT";
    case APP_MSG_SEND_REJECTED: return "APP_MSG_SEND_REJECTED";
    case APP_MSG_NOT_CONNECTED: return "APP_MSG_NOT_CONNECTED";
    case APP_MSG_APP_NOT_RUNNING: return "APP_MSG_APP_NOT_RUNNING";
    case APP_MSG_INVALID_ARGS: return "APP_MSG_INVALID_ARGS";
    case APP_MSG_BUSY: return "APP_MSG_BUSY";
    case APP_MSG_BUFFER_OVERFLOW: return "APP_MSG_BUFFER_OVERFLOW";
    case APP_MSG_ALREADY_RELEASED: return "APP_MSG_ALREADY_RELEASED";
    case APP_MSG_CALLBACK_ALREADY_REGISTERED: return "APP_MSG_CALLBACK_ALREADY_REGISTERED";
    case APP_MSG_CALLBACK_NOT_REGISTERED: return "APP_MSG_CALLBACK_NOT_REGISTERED";
    case APP_MSG_OUT_OF_MEMORY: return "APP_MSG_OUT_OF_MEMORY";
    case APP_MSG_CLOSED: return "APP_MSG_CLOSED";
    case APP_MSG_INTERNAL_ERROR: return "APP_MSG_INTERNAL_ERROR";
    default: return "UNKNOWN ERROR";
  }
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);
  
  // Create a long-lived buffer
  static char buffer_time[] = "00:00";
  static char buffer_date[] = "00/00";

  // Set the TextLayer's contents depending on locale
  if (strcmp("fr_FR", sys_locale) == 0) {
    strftime(buffer_date, sizeof("00/00"), "%d/%m", tick_time);
  } else if (strcmp("us_US", sys_locale) == 0 ) {
    strftime(buffer_date, sizeof("00/00"), "%m/%d", tick_time); 
  } else {
    // Fall back to ISO
    strftime(buffer_date, sizeof("00/00"), "%d/%m", tick_time); 
  }
  
  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(buffer_time, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    // Use 12 hour format
    strftime(buffer_time, sizeof("00:00"), "%I:%M", tick_time);
  }

  text_layer_set_text(s_time_layer,buffer_time);
  text_layer_set_text(s_date_layer,buffer_date);
}

static void update_map(){
  
  GRect sub_rect = GRect(pox_x,0,120,120);
  GRect map_rect = GRect(MAP_RECT_X,37,120,120);
  
  if(s_bitmap_cutted_map != NULL) gbitmap_destroy(s_bitmap_cutted_map);
  s_bitmap_cutted_map = gbitmap_create_as_sub_bitmap(s_bitmap_map, sub_rect);
  if(s_bitmap_layer_map != NULL) bitmap_layer_destroy(s_bitmap_layer_map);
  s_bitmap_layer_map = bitmap_layer_create(map_rect);
  bitmap_layer_set_bitmap(s_bitmap_layer_map, s_bitmap_cutted_map);
  bitmap_layer_set_compositing_mode(s_bitmap_layer_map, COMPOSITING);

#ifndef PBL_PLATFORM_APLITE
  layer_add_child(bitmap_layer_get_layer(s_bitmap_layer_circle),bitmap_layer_get_layer(s_bitmap_layer_map));
  layer_insert_below_sibling(bitmap_layer_get_layer(s_bitmap_layer_map),bitmap_layer_get_layer(s_bitmap_layer_circle));     
  layer_insert_below_sibling(bitmap_layer_get_layer(s_bitmap_layer_sea),bitmap_layer_get_layer(s_bitmap_layer_map));
#else
  layer_add_child(s_layer_lines, bitmap_layer_get_layer(s_bitmap_layer_map));
  layer_insert_below_sibling(bitmap_layer_get_layer(s_bitmap_layer_map) ,s_layer_lines);
#endif
}

static void generate_map(Layer *window_layer, GRect bounds){
  //init circle$
#ifndef PBL_PLATFORM_APLITE
  s_bitmap_circle = gbitmap_create_with_resource(RESOURCE_ID_STARS); 
  s_bitmap_layer_circle = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_bitmap_layer_circle, s_bitmap_circle);
  bitmap_layer_set_compositing_mode(s_bitmap_layer_circle,COMPOSITING );
#endif

  GRect sub_rect = GRect(pox_x,0,120,120);
  GRect map_rect = GRect(11,37,120,120);
  
  s_bitmap_map = gbitmap_create_with_resource(RESOURCE_ID_DOUBLEMAPDECALE); 
  s_bitmap_cutted_map = gbitmap_create_as_sub_bitmap(s_bitmap_map, sub_rect);
  s_bitmap_layer_map = bitmap_layer_create(map_rect);
  bitmap_layer_set_bitmap(s_bitmap_layer_map, s_bitmap_cutted_map);
  bitmap_layer_set_compositing_mode(s_bitmap_layer_map, COMPOSITING);
  
  //init sea
#ifndef PBL_PLATFORM_APLITE
  s_bitmap_sea = gbitmap_create_with_resource(RESOURCE_ID_SEA);
  s_bitmap_layer_sea = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_bitmap_layer_sea, s_bitmap_sea);
  bitmap_layer_set_compositing_mode(s_bitmap_layer_sea, COMPOSITING);
#endif

  //init time
  s_time_layer = text_layer_create(GRect(0, 0, 136, 28));
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentRight);
  
    //init time
  s_date_layer = text_layer_create(GRect(0, 30, 135, 16));
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentRight);
  
  //add layers
#ifndef PBL_PLATFORM_APLITE
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer_circle));
#endif
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer)); 
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
#ifndef PBL_PLATFORM_APLITE
  layer_add_child(bitmap_layer_get_layer(s_bitmap_layer_circle),bitmap_layer_get_layer(s_bitmap_layer_map));
  layer_add_child(bitmap_layer_get_layer(s_bitmap_layer_map),bitmap_layer_get_layer(s_bitmap_layer_sea));
#endif
  layer_add_child(window_layer, s_layer_lines); 

  //sort layers
#ifndef PBL_PLATFORM_APLITE
  layer_insert_above_sibling(text_layer_get_layer(s_time_layer) ,bitmap_layer_get_layer(s_bitmap_layer_circle));     
  layer_insert_above_sibling(text_layer_get_layer(s_date_layer) ,bitmap_layer_get_layer(s_bitmap_layer_circle));     
  layer_insert_above_sibling(s_layer_lines ,bitmap_layer_get_layer(s_bitmap_layer_circle));     
  layer_insert_below_sibling(bitmap_layer_get_layer(s_bitmap_layer_map),bitmap_layer_get_layer(s_bitmap_layer_circle));     
  layer_insert_below_sibling(bitmap_layer_get_layer(s_bitmap_layer_sea),bitmap_layer_get_layer(s_bitmap_layer_map));
#else
  layer_insert_above_sibling(text_layer_get_layer(s_time_layer) ,s_layer_lines);     
  layer_insert_above_sibling(text_layer_get_layer(s_date_layer) ,s_layer_lines); 
  layer_insert_below_sibling(bitmap_layer_get_layer(s_bitmap_layer_map) ,s_layer_lines); 
#endif
    
  update_time();
  
}

#ifndef PBL_PLATFORM_APLITE
static void update_line_proc(Layer *this_layer, GContext *ctx) {

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(71, 30, 1, pox_y), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(71, 30, 63, 1), 0, GCornerNone);
    
}
#else
  
float my_sqrt( float num ){
  float a, p, e = 0.001, b;

  a = num;
  p = a * a;
  while( p - num >= e ) { 
    b = ( a + ( num / a ) ) / 2;
    a = b;
    p = a * a;
  }
  return a;
}
  
 static void draw_circle(GContext *ctx){
  int r = 3600;
  for(int i = 12;i<72;i++){
      int x = 60-i+12;
      int y = (int) my_sqrt(r -x*x);
    //up left
      graphics_fill_rect(ctx, GRect(i, 36, 1, 60-y), 0, GCornerNone);
    // down left  
      graphics_fill_rect(ctx, GRect(i, 96+y, 1, 60-y), 0, GCornerNone);
    //up right
      graphics_fill_rect(ctx, GRect(144-i, 36, 1, 60-y), 0, GCornerNone);
    //down right
      graphics_fill_rect(ctx, GRect(144-i, 96+y, 1, 60-y), 0, GCornerNone);
  }
} 
 static void update_line_proc(Layer *this_layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx,GColorBlack);
  graphics_fill_rect(ctx, GRect(0,0,144,36), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(0,156,144,12), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(0,0,12,168), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(132,0,12,168), 0, GCornerNone);
   
   draw_circle(ctx);

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(71, 30, 1, pox_y), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(71, 30, 63, 1), 0, GCornerNone);
  
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(70, 30, 1,pox_y), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(72, 31, 1, pox_y), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(71,  30 + pox_y+ 1 , 1, 1), 0, GCornerNone);
  
} 
  
#endif

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}  

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    Tuple* long_tuple = dict_find(iterator, KEY_LONGITUDE);
    Tuple* lat_tuple = dict_find(iterator, KEY_LATITUDE);
    pox_x = long_tuple->value->uint32;
    pox_y = lat_tuple->value->uint32;
    update_map();
}
  
static void main_window_load(Window *window) {
    
  Layer *window_layer  = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  //init lines
  s_layer_lines = layer_create(bounds);
  layer_set_update_proc(s_layer_lines,update_line_proc);
          
  generate_map(window_layer, bounds);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped! %s", translate_error(reason));
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void main_window_unload(Window *window) {
  gbitmap_destroy(s_bitmap_map);
  gbitmap_destroy(s_bitmap_cutted_map);
  gbitmap_destroy(s_bitmap_sea);
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  bitmap_layer_destroy(s_bitmap_layer_map);
  layer_destroy(s_layer_lines);
  
  #ifdef PBL_PLATFORM_BASALT
    gbitmap_destroy(s_bitmap_circle);
    bitmap_layer_destroy(s_bitmap_layer_circle);
    bitmap_layer_destroy(s_bitmap_layer_sea);
  #endif
}

static void init(void) {
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Start init ");

  // Register with TickTimerService
  s_main_window = window_create(); 
  
  #ifdef PBL_PLATFORM_BASALT
    sys_locale = setlocale(LC_ALL, "");
  #endif
    
  pox_x = 0;
  pox_y = 0;
  locOK = 0;
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  //message location open
  // Register callbacks
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);
    // Open AppMessage
    app_message_open(64, APP_MESSAGE_OUTBOX_SIZE_MINIMUM);

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  // Make sure the time is displayed from the start
  update_time();
}

static void deinit(void) {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

