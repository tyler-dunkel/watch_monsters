#include <pebble.h>
#include "combat.c"

//Globals
static Window *navigation_window;
Layer *title_layer;
TextLayer *event_layer;
TextLayer *prog_layer;
static GBitmap     *logo;
static BitmapLayer *logo_layer;

//Prototypes
void* timer_data;
void* user_death_timer_data;
AppTimer* event_timer;

void time_up(void* data);
void user_death_time_up(void* data);
void user_ran_time_up(void*data);

static void user_death_select(ClickRecognizerRef recognizer, void *context) {
  user_level = DEFAULT_USER_LEVEL;
  user_experience = DEFAULT_USER_EXPERIENCE;
  user_experience_to_level_up = DEFAULT_MAX_EXPERIENCE;
  user_health = MAX_HEALTH;
  monster_health = MAX_HEALTH;
  int delay = rand() % user_level;
   if (delay == 0) {
    delay = delay + 1;
  }
  delay = delay * 3000;
  //app_timer_cancel(event_timer);
  //app_timer_reschedule(event_timer, delay);
  window_stack_pop(false);
  app_timer_register(delay, user_death_time_up, user_death_timer_data);
}

static void draw_title(Layer *layer, GContext *ctx)
{
  GRect title_bounds = layer_get_bounds(layer);
  
  logo = gbitmap_create_with_resource(RESOURCE_ID_LOGO);
  
  logo_layer = bitmap_layer_create(title_bounds);
  bitmap_layer_set_bitmap(logo_layer, logo);
  layer_add_child(layer, bitmap_layer_get_layer(logo_layer));
}

static void user_death_click_config_provider(void* context) {
//   window_single_click_subscribe(BUTTON_ID_SELECT, user_death_select);
  window_single_click_subscribe(BUTTON_ID_SELECT, user_death_select);
//   window_single_click_subscribe(BUTTON_ID_BACK, user_death_select);
//   window_single_click_subscribe(BUTTON_ID_DOWN, user_death_select);

}

void user_death_window_unload(Window *window){
  bitmap_layer_destroy(game_over_layer);
  gbitmap_destroy(game_over);
  window_destroy(user_death_window);
}

void user_death_window_load(Window *window) {
  //game_over_window = window_create();
  if(ran_away) {
    window_stack_pop(false);
    int delay = rand() % user_level;
    if (delay == 0) {
      delay = delay + 1;
    }
    
    delay = delay * 3000;
    //app_timer_cancel(event_timer);
    //app_timer_reschedule(event_timer, delay);
    window_stack_pop(false);
    app_timer_register(delay, user_ran_time_up, user_death_timer_data);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "in the if we are");
    ran_away = false;
    user_death_window_unload(user_death_window);
    return;
    
  }
    
  window_set_click_config_provider(user_death_window, user_death_click_config_provider);
  window_set_background_color(window, GColorBlack);
  
  /* window_set_window_handlers(main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  }); */
  
  //window_stack_push(window, true);
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  game_over = gbitmap_create_with_resource(RESOURCE_ID_GAME_OVER);
  
  game_over_layer = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(game_over_layer, game_over);
  layer_add_child(window_layer, bitmap_layer_get_layer(game_over_layer));
}

void main_window_load(Window *window){
  /*
    WINDOW HIERARCHY
    navigation_window  > (push&pop) > combat_window
      - title_layer
      - event_layer
      - prog_layer
  */
  
    APP_LOG(APP_LOG_LEVEL_DEBUG, "out the if we are");
  //TITLE LAYER -> Replace me with a function
    //title_layer = text_layer_create(GRect(0, 0, 144, 154));
    title_layer = layer_create(GRect(26, 0, 93, 40));
    // Set the text, font, and text alignment
    static char user_health_text[16];
    snprintf(user_health_text, sizeof(user_health_text), "Health %u", user_health);
    //     text_layer_set_text(title_layer, user_health_text);
    //     text_layer_set_font(title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
    //     text_layer_set_text_alignment(title_layer, GTextAlignmentCenter);
    // Add the text layer to the window
    layer_add_child(window_get_root_layer(window), title_layer);
    layer_set_update_proc(title_layer, draw_title);
  
  //EVENT LAYER -> Replace me with a function
    event_layer = text_layer_create(GRect(0,50, 144, 50));
    text_layer_set_font(event_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
    text_layer_set_text_alignment(event_layer, GTextAlignmentCenter);
    text_layer_set_text(event_layer, "Prepare for battle!");
    // Add the text layer to the window
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(event_layer));
  
  //PROG LAYER -> Replace me with a function
    prog_layer = text_layer_create(GRect(0, 130, 144, 14));
    text_layer_set_font(prog_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
    text_layer_set_text_alignment(prog_layer, GTextAlignmentCenter);
    // Add the text layer to the window
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(prog_layer));
  
  int delay = rand() % user_level;
  if (delay == 0) 
    delay = delay + 1;
  delay = delay * 10000;
  event_timer = app_timer_register(delay, time_up, timer_data);
}
void main_window_unload(Window *window){
  //text_layer_destroy(title_layer);
  bitmap_layer_destroy(logo_layer);
  text_layer_destroy(prog_layer);
  text_layer_destroy(event_layer);
//   window_destroy(navigation_window);
}


void time_up(void* data) {
  
  //PUSH EVENTS HERE
  //COMBAT BEGINS
  window_stack_push(combat_window, true);
  //combat_begin();
}

void user_death_time_up(void* data) {
  navigation_window = window_create();
  
  //create combat window and assign click handlers 
  combat_window = window_create();
  window_set_click_config_provider(combat_window, click_config_provider);
  window_set_window_handlers(combat_window, (WindowHandlers){
    .load = combat_window_load,
    .unload = combat_window_unload,
  });
  user_death_window = window_create();
  window_set_window_handlers(user_death_window, (WindowHandlers) {
    .load = user_death_window_load,
    .unload = user_death_window_unload,
  });
  
  user_level = persist_exists(USER_LEVEL_PKEY) ? persist_read_int(USER_LEVEL_PKEY) : DEFAULT_USER_LEVEL;
  user_experience = persist_exists(USER_EXPERIENCE_PKEY) ? persist_read_int(USER_EXPERIENCE_PKEY) : DEFAULT_USER_EXPERIENCE;
  user_experience_to_level_up = user_level * 25;
  
  
  window_set_window_handlers(navigation_window, (WindowHandlers){
    .load = main_window_load,
    .unload = main_window_unload,
  });
  
  window_stack_push(combat_window, true);
  
}

void user_ran_time_up(void* data) {
  navigation_window = window_create();
  
  //create combat window and assign click handlers 
  combat_window = window_create();
  window_set_click_config_provider(combat_window, click_config_provider);
  window_set_window_handlers(combat_window, (WindowHandlers){
    .load = combat_window_load,
    .unload = combat_window_unload,
  });
  user_death_window = window_create();
  window_set_window_handlers(user_death_window, (WindowHandlers) {
    .load = user_death_window_load,
    .unload = user_death_window_unload,
  });
  
  user_level = persist_exists(USER_LEVEL_PKEY) ? persist_read_int(USER_LEVEL_PKEY) : DEFAULT_USER_LEVEL;
  user_experience = persist_exists(USER_EXPERIENCE_PKEY) ? persist_read_int(USER_EXPERIENCE_PKEY) : DEFAULT_USER_EXPERIENCE;
  user_experience_to_level_up = user_level * 25;
  
  
  window_set_window_handlers(navigation_window, (WindowHandlers){
    .load = main_window_load,
    .unload = main_window_unload,
  });
  
  window_stack_push(combat_window, true);
  
}

void handle_init(void) {
  // Create a window and text layer
  navigation_window = window_create();
  
  //create combat window and assign click handlers 
  combat_window = window_create();
  window_set_click_config_provider(combat_window, click_config_provider);
  window_set_window_handlers(combat_window, (WindowHandlers){
    .load = combat_window_load,
    .unload = combat_window_unload,
  });
  user_death_window = window_create();
  window_set_window_handlers(user_death_window, (WindowHandlers) {
    .load = user_death_window_load,
    .unload = user_death_window_unload,
  });
  
  user_level = persist_exists(USER_LEVEL_PKEY) ? persist_read_int(USER_LEVEL_PKEY) : DEFAULT_USER_LEVEL;
  user_experience = persist_exists(USER_EXPERIENCE_PKEY) ? persist_read_int(USER_EXPERIENCE_PKEY) : DEFAULT_USER_EXPERIENCE;
  user_experience_to_level_up = user_level * 25;
  user_health = persist_exists(USER_HEALTH_PKEY) ? persist_read_int(USER_HEALTH_PKEY) : MAX_HEALTH;
  
  window_set_window_handlers(navigation_window, (WindowHandlers){
    .load = main_window_load,
    .unload = main_window_unload,
  });
  
  // Push the window
  window_stack_push(navigation_window, true);
  // App Logging!
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "Just pushed a window!");
}

void handle_deinit(void) {
  // Destroy the text layer
  //text_layer_destroy(title_layer);
  bitmap_layer_destroy(logo_layer);
  text_layer_destroy(event_layer);
  text_layer_destroy(prog_layer);
  
  // Destroy the window
  window_destroy(navigation_window);
  window_destroy(user_death_window);
  window_destroy(combat_window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}

