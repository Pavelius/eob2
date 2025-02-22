#pragma once

#include "point.h"

const int walls_frames = 9;
const int walls_count = 6;
const int door_offset = 1 + walls_frames * walls_count;
const int decor_offset = door_offset + 9;
const int decor_count = 21;
const int decor_frames = 10;
const int scrx = 22 * 8;
const int scry = 15 * 8;

typedef void(*fnevent)();
typedef void(*fnanswer)(int index, const void* value, const char* text, unsigned key, fnevent press_event);
typedef void (*fnoutput)(const char* format);

enum directions : unsigned char;
enum resid : unsigned short;
enum wearn : unsigned char;

struct hotkeyi;
struct creaturei;

extern unsigned long current_cpu_time;
extern bool need_update_animation;

void* choose_dialog(const char* title, int dx);
void* choose_generate_box(const char* header, const char* footer);
void* choose_generate_box(fnevent proc);
void* choose_generate_dialog(const char* header, bool random);
void* choose_large_menu(const char* header, const char* cancel);
void* choose_main_menu();
void* choose_small_menu(const char* header, const char* cancel);
void* choose_small_menu_spells(const char* header, const char* cancel);
void* show_message(const char* format, bool add_anaswers, const char* cancel = 0, unsigned cancel_key = 27);
void* dialogv(const char* cancel, const char* format);
void* dialogv(const char* cancel, const char* format, const char* format_param);
void* dialog(const char* cancel, const char* format, ...);
void choose_spells(const char* title, const char* cancel, int spell_type);
void show_scene(fnevent before_paint, fnevent input, void* focus);

bool adventure_input(const hotkeyi* hotkeys);
bool alternate_focus_input();
void animation_update();
void button_label(int index, const void* data, const char* format, unsigned key, fnevent proc);
bool choose_avatar();
void city_input(const hotkeyi* hotkeys);
void clear_input();
bool confirm(const char* format);
void fix_animate();
void fix_attack(const creaturei* attacker, wearn slot, int hits);
void fix_damage(const creaturei* p, int value);
void fix_monster_attack(const creaturei* target);
void fix_monster_attack_end(const creaturei* target);
void fix_monster_damage(const creaturei* target);
void fix_monster_damage_end();
void header_yellow(const char* format);
bool hotkey_input(const hotkeyi* hotkeys);
void initialize_gui();
void message_box(const char* format);
void paint_adventure();
void paint_arrow(point camera, directions direct, int mpg);
void paint_avatars();
void paint_avatars_no_focus();
void paint_avatars_no_focus_hilite();
void paint_city();
void paint_city_menu();
void paint_character_edit();
void paint_choose_avatars();
void paint_dungeon();
void paint_main_menu();
void paint_party_status();
void paint_small_menu();
void paint_test_mode();
void pick_up_item();
void set_dungeon_tiles(resid type);
void set_focus_by_player();
void set_player_by_focus();
void set_small_font();
void show_dungeon_images();
void text_label(int index, const void* data, const char* format, unsigned key, fnevent proc);
void text_label_menu(int index, const void* data, const char* format, unsigned key, fnevent proc);

int thrown_side(int avatar_thrown, int side);