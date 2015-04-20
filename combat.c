#include <pebble.h>

#define USER_LEVEL_PKEY 1
#define USER_EXPERIENCE_PKEY 2
#define USER_HEALTH_PKEY 3
#define DEFAULT_USER_LEVEL 1
#define DEFAULT_USER_EXPERIENCE 0
#define DEFAULT_MAX_EXPERIENCE 100
#define MAX_HEALTH 100

//Main window
static Window      *combat_window;
static Window      *user_death_window;
static GBitmap     *game_over;
static BitmapLayer *game_over_layer;

//Menu layers
static Layer *rectangle_layer;
static TextLayer *opt1;
static TextLayer *opt2;
static TextLayer *opt3;
static TextLayer *opt4;
static TextLayer *selector;
static TextLayer *user_prompt;
static TextLayer *monster_prompt;
static TextLayer *run_prompt;

//Health bar
//NB - 3:30
static GRect max_health_player;
static GRect max_health_enemy;
static GRect current_health_player;
static GRect current_health_enemy;
static Layer *user_health_bar;
static Layer *user_health_bar_inner;
static Layer *enemy_health_bar;
static Layer *enemy_health_bar_inner;

//Player Image
static GBitmap *player_img;
static BitmapLayer *player_img_layer;

//Enemy Image
static GBitmap *enemy_img;
static BitmapLayer *enemy_img_layer;

//Properties of menu layers
static GRect rectangle_bounds;
static GRect selector_bounds;
static ushort menu_position = 0;

//Temporary layers for testing
TextLayer *level_layer;
TextLayer *experience_layer;
TextLayer *user_health_layer;
TextLayer *monster_health_layer;

//User properties
static ushort user_level = DEFAULT_USER_LEVEL;
static int user_experience = DEFAULT_USER_EXPERIENCE;
static int user_experience_to_level_up = DEFAULT_MAX_EXPERIENCE;
static ushort user_health = MAX_HEALTH;

//Monster properties
static ushort monster_level;
static ushort monster_health = MAX_HEALTH;

static bool ran_away = false;

//Determines user damage based on level
static ushort calculate_user_damage()
{
  ushort user_roll = rand() % 100;
  ushort user_damage = user_level + 10;
  
  if (user_roll < 11)
    user_damage = 0;
  else if (user_roll >= 11 && user_roll < 31)
    user_damage /= 2;
  else if (user_roll >= 86)
    user_damage *= 2;
  
  return user_damage;
}

//Determines monster damage based on level
static ushort calculate_monster_damage()
{
  ushort monster_roll = rand() % 100;
  ushort monster_damage = monster_level + 5;
  if (monster_roll < 11)
    monster_damage = 0;
  else if (monster_roll >= 11 && monster_roll < 31)
    monster_damage /= 2;
  else if (monster_roll >= 86)
    monster_damage *= 2;
  
  return monster_damage;
}

//Sets monster level
//TODO: reset monster health
static void generate_monster()
{  
  int monster_difficulty = 0;
  if (user_level < 3) { 
    monster_difficulty = rand() % 3;
      switch (monster_difficulty) {
        case 0 : monster_level = user_level - 1;  break;
        case 2 : monster_level = user_level + 1; break;
        default: monster_level = user_level;
      }
  } else {
    monster_difficulty = rand() % 5;
    switch (monster_difficulty) {
        case 0 : monster_level = user_level - 2; break;
        case 1 : monster_level = user_level - 1; break;
        case 3 : monster_level = user_level + 1; break;
        case 4 : monster_level = user_level + 2; break;
        default: monster_level = user_level;
      }
  }
  if (monster_level == 0)
    monster_level++;
}

//Applies user damage to monster health
static void user_attack()
{
  ushort user_damage = calculate_user_damage();
  if (monster_health - user_damage <= 0) {
    generate_monster();
    monster_health = 100;
    return;
  } else
    monster_health -= user_damage;
  
  static char monster_health_text[4];
  snprintf(monster_health_text, sizeof(monster_health_text), "%u", monster_health);
  text_layer_set_text(monster_health_layer, monster_health_text);
  
  layer_set_hidden(text_layer_get_layer(user_prompt), false);
  static char user_damage_text[21];
  snprintf(user_damage_text, sizeof(user_damage_text), "You dealt %u damage", user_damage);
  text_layer_set_text(user_prompt, user_damage_text);
}

//Applies monster damage to user health
static void monster_attack()
{  
  ushort monster_damage = calculate_monster_damage();
  if (user_health - monster_damage <= 0) {
    //DEBUG, REVIVE USER
    //user_health = 100;
    window_stack_pop(false);
    window_stack_push(user_death_window, true);
    return;
  } else
    user_health -= monster_damage;
  
  static char user_health_text[4];
  snprintf(user_health_text, sizeof(user_health_text), "%u", user_health);
  text_layer_set_text(user_health_layer, user_health_text);
  
  layer_set_hidden(text_layer_get_layer(monster_prompt), false);
  static char monster_damage_text[25];
  snprintf(monster_damage_text, sizeof(monster_damage_text), "Monster dealt %u damage", monster_damage);
  text_layer_set_text(monster_prompt, monster_damage_text);
}

//User attempts to escape battle from menu
static void menu_run()
{  
  layer_set_hidden(text_layer_get_layer(selector), true);
  ushort escape_chance = rand() % 4;
  layer_insert_above_sibling(text_layer_get_layer(monster_prompt), text_layer_get_layer(user_prompt));
  layer_insert_above_sibling(text_layer_get_layer(run_prompt), text_layer_get_layer(monster_prompt));
  layer_set_hidden(text_layer_get_layer(run_prompt), false);
  switch (escape_chance) {
    case 0:
      text_layer_set_text(run_prompt, "Escaped successfully!");
      return;
    case 1:
      if (monster_level > user_level) {
        text_layer_set_text(run_prompt, "Failed to escape!");
        monster_attack();
      } else {
        text_layer_set_text(run_prompt, "Escaped successfully!");
      }
      return;
    default:
      if (user_level > monster_level) {
        text_layer_set_text(run_prompt, "Escaped successfully!");
      } else {
        text_layer_set_text(run_prompt, "Failed to escape!");
        monster_attack();
      }
  }
}

//User chooses to attack from menu
static void menu_attack()
{  
  layer_set_hidden(text_layer_get_layer(selector), true);

  if (monster_level > user_level) {
    layer_insert_above_sibling(text_layer_get_layer(user_prompt), text_layer_get_layer(run_prompt));
    layer_insert_above_sibling(text_layer_get_layer(monster_prompt), text_layer_get_layer(user_prompt));
    monster_attack();
    user_attack();
  } else {
    layer_insert_above_sibling(text_layer_get_layer(monster_prompt), text_layer_get_layer(run_prompt));
    layer_insert_above_sibling(text_layer_get_layer(user_prompt), text_layer_get_layer(monster_prompt));
    user_attack();
    monster_attack();
  }
}

//Refreshes level displayed on screen
static void update_level()
{
  static char level_text[4];
  snprintf(level_text, sizeof(level_text), "%u", user_level);
  text_layer_set_text(level_layer, level_text);
}

//Refreshes experience displayed on screen
static void update_experience()
{
  static char experience_text[6];
  snprintf(experience_text, sizeof(experience_text), "%u", user_experience);
  text_layer_set_text(experience_layer, experience_text);
}

//Draws and fills menu background
static void draw_menu_rectangle(Layer *layer, GContext *ctx)
{
  GRect bounds = layer_get_bounds(layer);
  
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 45, GCornersAll);
}

//Draws player health bar container (outer part)
//NB - 3:30
static void draw_player_health_bar_container(Layer *layer, GContext *ctx)
{
  max_health_player = layer_get_bounds(layer);

  graphics_context_set_stroke_color(ctx, GColorBlack);
  //graphics_context_set_fill_color(ctx, GColorBlack);
  //graphics_fill_rect(ctx, max_health_player, 0, 0000);
  
  graphics_draw_rect(ctx, max_health_player);
}

//Draws player health bar (inner)
//NB - 3:30
static void draw_player_health_bar(Layer *layer, GContext *ctx)
{  
    current_health_player = GRect(max_health_player.origin.x + 2, max_health_player.origin.y + 2, 
                               ((max_health_player.size.w - 4) - ((100 - user_health)/2)), max_health_player.size.h - 4);
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, current_health_player, 0, GCornerNone);
}

static void draw_enemy_health_bar_container(Layer *layer, GContext *ctx)
{
  max_health_enemy = layer_get_bounds(layer);
  
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_rect(ctx, max_health_enemy);
}

static void draw_enemy_health_bar(Layer *layer, GContext *ctx)
{
    //current_health_enemy = GRect(max_health_enemy.origin.x + 2, max_health_enemy.origin.y + 2, 
                               //((max_health_enemy.size.w - 4) - ((100 - monster_health)/2)), max_health_enemy.size.h - 4);
    current_health_enemy = GRect(max_health_enemy.origin.x + 2, max_health_enemy.origin.y + 2, 
                               50, max_health_enemy.size.h - 4);
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, current_health_enemy, 0, GCornerNone);
}

//Updates selected menu item upon button clicks
static void update_menu()
{  
  switch (menu_position) {
    case 1 : selector_bounds = GRect(rectangle_bounds.size.w / 2 + 5, rectangle_bounds.origin.y, 5, rectangle_bounds.size.h / 2); break;
    case 2 : selector_bounds = GRect(rectangle_bounds.origin.x + 5, rectangle_bounds.size.h / 2 , 5, rectangle_bounds.size.h / 2); break;
    case 3 : selector_bounds = GRect(rectangle_bounds.size.w / 2 + 5, rectangle_bounds.size.h / 2, 5, rectangle_bounds.size.h / 2); break;
    default: selector_bounds = GRect(rectangle_bounds.origin.x + 5, rectangle_bounds.origin.y, 5, rectangle_bounds.size.h / 2);
  }
  text_layer_destroy(selector);
  selector = text_layer_create(selector_bounds);
  text_layer_set_text(selector, ">");
  text_layer_set_text_color(selector, GColorWhite);
  text_layer_set_background_color(selector, GColorClear);
  layer_add_child(rectangle_layer, text_layer_get_layer(selector));
}

static void menu_scroll_forward(ClickRecognizerRef recognizer, void *context)
{
  if (menu_position == 3)
    menu_position = 0;
  else
    menu_position++;
  update_menu();
}

static void menu_scroll_back(ClickRecognizerRef recognizer, void *context)
{
  if (menu_position == 0)
    menu_position = 3;
  else
    menu_position--;
  update_menu();  
}

static bool prompts_present()
{
  if (!layer_get_hidden(text_layer_get_layer(run_prompt)) || !layer_get_hidden(text_layer_get_layer(user_prompt)) || !layer_get_hidden(text_layer_get_layer(monster_prompt)))
    return true;
  else
    return false;
}

//Hides any combat related prompts covering the menu
static void hide_combat_prompts()
{
  if (monster_level <= user_level) {
    if (!layer_get_hidden(text_layer_get_layer(run_prompt))) {
      layer_set_hidden(text_layer_get_layer(run_prompt), true);
      char first_letter = text_layer_get_text(run_prompt)[0];
      if (first_letter == 'E') {
        ran_away = true;
        window_stack_push(user_death_window, false);
        window_stack_pop(false);
      }
      return;
    } else if (!layer_get_hidden(text_layer_get_layer(user_prompt))) {
      layer_set_hidden(text_layer_get_layer(user_prompt), true);
      return;
    } else if (!layer_get_hidden(text_layer_get_layer(monster_prompt))) {
      layer_set_hidden(text_layer_get_layer(monster_prompt), true);
      layer_set_hidden(text_layer_get_layer(selector), false);
      return;
    }
  } else if (monster_level > user_level) {
    if (!layer_get_hidden(text_layer_get_layer(run_prompt))) {
      layer_set_hidden(text_layer_get_layer(run_prompt), true);
      char first_letter = text_layer_get_text(run_prompt)[0];
      if (first_letter == 'E') {
        ran_away = true;
        window_stack_push(user_death_window, false);
        window_stack_pop(false);
        
      }
      return;
    } else if (!layer_get_hidden(text_layer_get_layer(monster_prompt))) {
      layer_set_hidden(text_layer_get_layer(monster_prompt), true);
      if (menu_position == 3)
        layer_set_hidden(text_layer_get_layer(selector), false);
      return;
    } else if (!layer_get_hidden(text_layer_get_layer(user_prompt))) {
      layer_set_hidden(text_layer_get_layer(user_prompt), true);
      layer_set_hidden(text_layer_get_layer(selector), false);
      return;
    }
  }
}

//Hub for other functions launched from the menu based
static void menu_select(ClickRecognizerRef recognizer, void *context)
{
  if (prompts_present()) {
    hide_combat_prompts();
    return;
  }
  switch(menu_position) {
    case 1 : break;
    case 2 : break;
    case 3 : menu_run(); break;
    default: menu_attack();
  }
}

static void click_config_provider(void *context)
{
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 50, menu_scroll_back);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 50, menu_scroll_forward);
  window_single_click_subscribe(BUTTON_ID_SELECT, menu_select);  
}

static void combat_window_load(Window *window)
{
//   user_level = persist_exists(USER_LEVEL_PKEY) ? persist_read_int(USER_LEVEL_PKEY) : DEFAULT_USER_LEVEL;
//   user_experience = persist_exists(USER_EXPERIENCE_PKEY) ? persist_read_int(USER_EXPERIENCE_PKEY) : DEFAULT_USER_EXPERIENCE;
//   user_experience_to_level_up = user_level * 25;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Just pushed combat window");
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);
  
  generate_monster();
  
  //User Health as text
  user_health_layer = text_layer_create(GRect(30, 5, 20, 20));
  static char user_health_text[4];
  snprintf(user_health_text, sizeof(user_health_text), "%u", user_health);
  text_layer_set_text(user_health_layer, user_health_text);
  //layer_add_child(window_layer, text_layer_get_layer(user_health_layer));
  
  //User Health as Bar
  //NB - 3:30
  user_health_bar = layer_create(GRect(90, 70, 54, 14));
  layer_add_child(window_layer, user_health_bar);
  layer_set_update_proc(user_health_bar, draw_player_health_bar_container);
  
  user_health_bar_inner = layer_create(GRect(92, 72, 50, 10));
  layer_add_child(window_layer, user_health_bar_inner);
  layer_set_update_proc(user_health_bar_inner, draw_player_health_bar);
  //NB - 3:30
  
  //Player Img
  GRect player_bounds = GRect(5, 65, 48, 48);
  player_img = gbitmap_create_with_resource(RESOURCE_ID_PLAYER);
  player_img_layer = bitmap_layer_create(player_bounds);
  bitmap_layer_set_bitmap(player_img_layer, player_img);
  layer_add_child(window_layer, bitmap_layer_get_layer(player_img_layer));
  
  //Enemy
  //monster_health_layer = text_layer_create(GRect(55, 5, 20, 20));
  //static char monster_health_text[4];
  //snprintf(monster_health_text, sizeof(monster_health_text), "%u", monster_health);
  //text_layer_set_text(monster_health_layer, monster_health_text);
  //layer_add_child(window_layer, text_layer_get_layer(monster_health_layer));
  
  
  //Enemy Health Bar
  enemy_health_bar = layer_create(GRect(10, 10, 54, 14));
  layer_add_child(window_layer, user_health_bar);
  layer_set_update_proc(enemy_health_bar, draw_enemy_health_bar_container);
  
  enemy_health_bar_inner = layer_create(GRect(8, 8, 50, 10));
  layer_add_child(window_layer, enemy_health_bar);
  layer_set_update_proc(enemy_health_bar_inner, draw_enemy_health_bar);
  
  //Enemy image
  int rand_monster = rand() % 3;
  switch(rand_monster)
  {
    case 1: 
    enemy_img = gbitmap_create_with_resource(RESOURCE_ID_GHOUL);
    break;
    case 2:
    enemy_img = gbitmap_create_with_resource(RESOURCE_ID_DUCK);
    break;
    default:
    enemy_img = gbitmap_create_with_resource(RESOURCE_ID_BEAR);
  }
  
  GRect enemy_bounds = GRect(95, 5, 48, 48);
  enemy_img_layer = bitmap_layer_create(enemy_bounds);
  bitmap_layer_set_bitmap(enemy_img_layer, enemy_img);
  layer_add_child(window_layer, bitmap_layer_get_layer(enemy_img_layer));
  
  //level_layer = text_layer_create(GRect(5, 5, 20, 20));
  update_level();
  //layer_add_child(window_layer, text_layer_get_layer(level_layer));
  
  //experience_layer = text_layer_create(GRect(5, 30, 20, 20));
  update_experience();
  //layer_add_child(window_layer, text_layer_get_layer(experience_layer));
  
  rectangle_layer = layer_create(GRect(5, 2 * (window_bounds.size.h / 3), window_bounds.size.w - 10, window_bounds.size.h / 3));
  layer_add_child(window_layer, rectangle_layer);
  layer_set_update_proc(rectangle_layer, draw_menu_rectangle);
  rectangle_bounds = layer_get_bounds(rectangle_layer);
  
  selector_bounds = GRect(rectangle_bounds.origin.x + 5, rectangle_bounds.origin.y, 5, rectangle_bounds.size.h / 2);
  selector = text_layer_create(selector_bounds);
  text_layer_set_text(selector, ">");
  text_layer_set_text_color(selector, GColorWhite);
  text_layer_set_background_color(selector, GColorClear);
  layer_add_child(rectangle_layer, text_layer_get_layer(selector));
  
  GRect opt1_bounds = GRect(rectangle_bounds.origin.x, rectangle_bounds.origin.y, rectangle_bounds.size.w / 2, rectangle_bounds.size.h / 2);
  opt1 = text_layer_create(opt1_bounds);
  text_layer_set_text(opt1, "ATK");
  text_layer_set_text_color(opt1, GColorWhite);
  text_layer_set_background_color(opt1, GColorClear);
  text_layer_set_text_alignment(opt1, GTextAlignmentCenter);
  layer_add_child(rectangle_layer, text_layer_get_layer(opt1));
  
  GRect opt2_bounds = GRect(rectangle_bounds.size.w / 2, rectangle_bounds.origin.y, rectangle_bounds.size.w / 2, rectangle_bounds.size.h / 2);
  opt2 = text_layer_create(opt2_bounds);
  text_layer_set_text(opt2, "ITEM");
  text_layer_set_text_color(opt2, GColorWhite);
  text_layer_set_background_color(opt2, GColorClear);
  text_layer_set_text_alignment(opt2, GTextAlignmentCenter);
  layer_add_child(rectangle_layer, text_layer_get_layer(opt2));
  
  GRect opt3_bounds = GRect(rectangle_bounds.origin.x, rectangle_bounds.size.h / 2, rectangle_bounds.size.w / 2, rectangle_bounds.size.h / 2);
  opt3 = text_layer_create(opt3_bounds);
  text_layer_set_text(opt3, "GEAR");
  text_layer_set_text_color(opt3, GColorWhite);
  text_layer_set_background_color(opt3, GColorClear);
  text_layer_set_text_alignment(opt3, GTextAlignmentCenter);
  layer_add_child(rectangle_layer, text_layer_get_layer(opt3));
  
  GRect opt4_bounds = GRect(rectangle_bounds.size.w / 2, rectangle_bounds.size.h / 2, rectangle_bounds.size.w / 2, rectangle_bounds.size.h / 2);
  opt4 = text_layer_create(opt4_bounds);
  text_layer_set_text(opt4, "RUN");
  text_layer_set_text_color(opt4, GColorWhite);
  text_layer_set_background_color(opt4, GColorClear);
  text_layer_set_text_alignment(opt4, GTextAlignmentCenter);
  layer_add_child(rectangle_layer, text_layer_get_layer(opt4));
  
  GRect user_prompt_bounds = GRect(rectangle_bounds.origin.x + 5, rectangle_bounds.origin.y + 5, rectangle_bounds.size.w - 10, rectangle_bounds.size.h - 10);
  user_prompt = text_layer_create(user_prompt_bounds);
  layer_set_hidden(text_layer_get_layer(user_prompt), true);
  text_layer_set_text_color(user_prompt, GColorWhite);
  text_layer_set_background_color(user_prompt, GColorBlack);
  layer_add_child(rectangle_layer, text_layer_get_layer(user_prompt));
  
  GRect monster_prompt_bounds = GRect(rectangle_bounds.origin.x + 5, rectangle_bounds.origin.y + 5, rectangle_bounds.size.w - 10, rectangle_bounds.size.h - 10);
  monster_prompt = text_layer_create(monster_prompt_bounds);
  layer_set_hidden(text_layer_get_layer(monster_prompt), true);
  text_layer_set_text_color(monster_prompt, GColorWhite);
  text_layer_set_background_color(monster_prompt, GColorBlack);
  layer_add_child(rectangle_layer, text_layer_get_layer(monster_prompt));
  
  GRect run_prompt_bounds = GRect(rectangle_bounds.origin.x + 5, rectangle_bounds.origin.y + 5, rectangle_bounds.size.w - 10, rectangle_bounds.size.h - 10);
  run_prompt = text_layer_create(run_prompt_bounds);
  layer_set_hidden(text_layer_get_layer(run_prompt), true);
  text_layer_set_text_color(run_prompt, GColorWhite);
  text_layer_set_background_color(run_prompt, GColorBlack);
  layer_add_child(rectangle_layer, text_layer_get_layer(run_prompt));
}

static void combat_window_unload(Window *window)
{ 
  persist_write_int(USER_LEVEL_PKEY, user_level);
  persist_write_int(USER_EXPERIENCE_PKEY, user_experience);
  persist_write_int(USER_HEALTH_PKEY, user_health);
  bitmap_layer_destroy(enemy_img_layer);
  gbitmap_destroy(enemy_img);
  bitmap_layer_destroy(player_img_layer);
  gbitmap_destroy(player_img);
  text_layer_destroy(opt1);
  text_layer_destroy(opt2);
  text_layer_destroy(opt3);
  text_layer_destroy(opt4);
  layer_destroy(rectangle_layer);
  layer_destroy(enemy_health_bar);
  layer_destroy(enemy_health_bar_inner);
  layer_destroy(user_health_bar);
  layer_destroy(user_health_bar_inner);
  text_layer_destroy(selector);
  text_layer_destroy(user_prompt);
  text_layer_destroy(monster_prompt);
  text_layer_destroy(run_prompt);
  window_destroy(combat_window);
}

