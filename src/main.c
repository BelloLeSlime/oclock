#include "eadk.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "O'clock";
const uint32_t eadk_api_level  __attribute__((section(".rodata.eadk_api_level"))) = 0;

// Normalise l'angle dans [-PI, PI]
static float normalize_angle(float x) {
  while (x > M_PI)  x -= 2.0f * M_PI;
  while (x < -M_PI) x += 2.0f * M_PI;
  return x;
}

// Approximation haute précision sur tout le cercle [-PI, PI]
float sinf(float x) {
  x = normalize_angle(x);

  // Réduction d'argument dans l'intervalle [-PI/2, PI/2]
  if (x > M_PI / 2.0f)  x = M_PI - x;
  if (x < -M_PI / 2.0f) x = -M_PI - x;

  float x2 = x * x;
  // Polynôme de Taylor à l'ordre 9
  return x * (1.0f - x2 * (0.16666667f - x2 * (0.008333333f - x2 * (0.000198412f - x2 * 0.0000027557f))));
}

float cosf(float x) {
  return sinf(x + (M_PI / 2.0f));
}

void format_two_digits(char * dest, int val) {
  dest[0] = '0' + (val / 10) % 10;
  dest[1] = '0' + val % 10;
}

void format_time(char * buffer, int h, int m, int s) {
  format_two_digits(buffer, h);
  buffer[2] = ':';
  format_two_digits(buffer + 3, m);
  buffer[5] = ':';
  format_two_digits(buffer + 6, s);
  buffer[8] = '\0';
}

float h = 10.0f;
float m = 10.0f;
float s = 0.0f;

const int ZERO_X = 160;
const int ZERO_Y = 120;

int hx = ZERO_X, hy = ZERO_Y;
int mx = ZERO_X, my = ZERO_Y;
int sx = ZERO_X, sy = ZERO_Y;

static char pressed_before = '\0';

char get_pressed_digit(eadk_keyboard_state_t kb) {
  char pressed = '\0';

  if (eadk_keyboard_key_down(kb, eadk_key_zero))  pressed = '0';
  if (eadk_keyboard_key_down(kb, eadk_key_one))   pressed = '1';
  if (eadk_keyboard_key_down(kb, eadk_key_two))   pressed = '2';
  if (eadk_keyboard_key_down(kb, eadk_key_three)) pressed = '3';
  if (eadk_keyboard_key_down(kb, eadk_key_four))  pressed = '4';
  if (eadk_keyboard_key_down(kb, eadk_key_five))  pressed = '5';
  if (eadk_keyboard_key_down(kb, eadk_key_six))   pressed = '6';
  if (eadk_keyboard_key_down(kb, eadk_key_seven)) pressed = '7';
  if (eadk_keyboard_key_down(kb, eadk_key_eight)) pressed = '8';
  if (eadk_keyboard_key_down(kb, eadk_key_nine))  pressed = '9';

  // Si c'est le MÊME état qu'avant (touche toujours appuyée ou toujours relâchée)
  if (pressed == pressed_before) {
    return '\0'; // On ne déclenche pas de nouveau chiffre
  }

  // Si l'état a changé, on sauvegarde le nouvel état
  pressed_before = pressed;

  // On renvoie le chiffre uniquement si une touche vient d'être ENFONCÉE
  return pressed;
}

void run_number_input() {
  char digits[7] = "000000";
  int pos = 0;

  // On dessine le fond et les instructions UNE SEULE FOIS pour éviter de surcharger le processeur graphique
  eadk_display_push_rect_uniform((eadk_rect_t){0, 0, EADK_SCREEN_WIDTH, EADK_SCREEN_HEIGHT}, eadk_color_white);
  eadk_display_draw_string("ENTRE L'HEURE (HH:MM:SS)", (eadk_point_t){80, 50}, false, eadk_color_black, eadk_color_white);
  eadk_display_draw_string("[0-9] Taper  [BACK] Effacer", (eadk_point_t){70, 170}, false, eadk_color_black, eadk_color_white);
  eadk_display_draw_string("[EXE] Valider", (eadk_point_t){100, 190}, false, eadk_color_black, eadk_color_white);

  bool dirty = true; // Indique s'il faut redessiner le texte de saisie

  while (true) {
    if (dirty) {
      // Effacement ciblé de la zone du texte d'heure uniquement
      eadk_display_push_rect_uniform((eadk_rect_t){ZERO_X - 50, ZERO_Y - 20, 100, 30}, eadk_color_white);

      char display_buf[9];
      display_buf[0] = digits[0]; display_buf[1] = digits[1]; display_buf[2] = ':';
      display_buf[3] = digits[2]; display_buf[4] = digits[3]; display_buf[5] = ':';
      display_buf[6] = digits[4]; display_buf[7] = digits[5]; display_buf[8] = '\0';

      eadk_display_draw_string(display_buf, (eadk_point_t){ZERO_X - 32, ZERO_Y - 10}, false, eadk_color_black, eadk_color_white);
      dirty = false;
    }

    eadk_keyboard_state_t kb = eadk_keyboard_scan();

    if (eadk_keyboard_key_down(kb, eadk_key_exe)) {
      break;
    }

    if (eadk_keyboard_key_down(kb, eadk_key_back)) {
      if (pos > 0) {
        pos--;
        digits[pos] = '0';
        dirty = true;
        eadk_timing_msleep(150);
      }
    }

    char key_char = get_pressed_digit(kb);
    if (key_char != '\0' && pos < 6) {
      digits[pos] = key_char;
      pos++;
      dirty = true;
      eadk_timing_msleep(150);
    }

    eadk_timing_msleep(20);
  }

  int parsed_h = (digits[0] - '0') * 10 + (digits[1] - '0');
  int parsed_m = (digits[2] - '0') * 10 + (digits[3] - '0');
  int parsed_s = (digits[4] - '0') * 10 + (digits[5] - '0');

  h = (float)(parsed_h % 24);
  m = (float)(parsed_m % 60);
  s = (float)(parsed_s % 60);
}

void update_time(float delta_sec) {
  s += delta_sec;
  m = ((int)m)+s/60;
  h = ((int)h)+m/60;

  if (s >= 60.0f) {
    s -= 60.0f;
  }
  if (m >= 60.0f) {
    m -= 60.0f;
  }
  if (h >= 24.0f) {
    h -= 24.0f;
  }
}

void line(int x0, int y0, int x1, int y1, eadk_color_t color) {
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy, e2;

  while (true) {
    eadk_display_push_rect_uniform((eadk_rect_t){x0, y0, 1, 1}, color);
    if (x0 == x1 && y0 == y1) break;
    e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

eadk_point_t get_point(float t, char ttype, int r, int cx, int cy) {
  float angle;
  if (ttype == 'h') {
    angle = (t - 3.0f) * (M_PI / 6.0f);
  } else {
    angle = (t - 15.0f) * (M_PI / 30.0f);
  }
  int x = (int)(r * cosf(angle)) + cx;
  int y = (int)(r * sinf(angle)) + cy;
  return (eadk_point_t){x, y};
}

void circle(int x, int y, int radius, eadk_color_t color) {
  for (int i = 0; i < 600; i++) {
    eadk_point_t point = get_point((float)i / 10.0f, 's', radius, x, y);
    eadk_display_push_rect_uniform((eadk_rect_t){point.x, point.y, 1, 1}, color);
  }
}

void draw_clock() {


  // Formater et réafficher l'heure
  char time_str[16];
  format_time(time_str, (int)h, (int)m, (int)s);
  eadk_display_draw_string(time_str, (eadk_point_t){ZERO_X - 32, ZERO_Y - 8}, false, eadk_color_black, eadk_color_white);

  // Effacer les anciennes aiguilles
  line(ZERO_X, ZERO_Y, sx, sy, eadk_color_white);
  line(ZERO_X, ZERO_Y, mx, my, eadk_color_white);
  line(ZERO_X, ZERO_Y, hx, hy, eadk_color_white);

  // Calculer et dessiner les nouvelles aiguilles
  eadk_point_t p_s = get_point(s, 's', 85, ZERO_X, ZERO_Y);
  eadk_point_t p_m = get_point(m, 'm', 75, ZERO_X, ZERO_Y);
  eadk_point_t p_h = get_point(h, 'h', 50, ZERO_X, ZERO_Y);

  line(ZERO_X, ZERO_Y, p_s.x, p_s.y, eadk_color_red);
  line(ZERO_X, ZERO_Y, p_m.x, p_m.y, eadk_color_black);
  line(ZERO_X, ZERO_Y, p_h.x, p_h.y, eadk_color_black);

  sx = p_s.x; sy = p_s.y;
  mx = p_m.x; my = p_m.y;
  hx = p_h.x; hy = p_h.y;
}

int main(int argc, char * argv[]) {
  // Phase 1 : Saisie utilisateur
  run_number_input();

  // Phase 2 : Nettoyage et dessin de l'horloge
  eadk_display_push_rect_uniform((eadk_rect_t){0, 0, EADK_SCREEN_WIDTH, EADK_SCREEN_HEIGHT}, eadk_color_white);

  circle(ZERO_X, ZERO_Y, 100, eadk_color_black);
  circle(ZERO_X, ZERO_Y, 110, eadk_color_black);

  for (int i = 0; i < 60; i++) {
    eadk_point_t p1 = get_point((float)i, 's', 95, ZERO_X, ZERO_Y);
    eadk_point_t p2 = get_point((float)i, 's', 100, ZERO_X, ZERO_Y);
    line(p1.x, p1.y, p2.x, p2.y, eadk_color_black);
  }

  for (int i = 0; i < 12; i++) {
    eadk_point_t p1 = get_point((float)i, 'h', 90, ZERO_X, ZERO_Y);
    eadk_point_t p2 = get_point((float)i, 'h', 100, ZERO_X, ZERO_Y);
    line(p1.x, p1.y, p2.x, p2.y, eadk_color_black);
  }

  uint64_t last_time = eadk_timing_millis();

  while (true) {
    eadk_keyboard_state_t keyboard = eadk_keyboard_scan();
    if (eadk_keyboard_key_down(keyboard, eadk_key_home) || eadk_keyboard_key_down(keyboard, eadk_key_back)) {
      break;
    }

    uint64_t current_time = eadk_timing_millis();
    float delta_sec = (float)(current_time - last_time) / 1000.0f;
    last_time = current_time;

    update_time(delta_sec);
    draw_clock();

    eadk_timing_msleep(50);
  }

  return 0;
}