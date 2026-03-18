#include <alias.h>

// Colors
#define C_WHITE 0xFFFFFF

// SCREEN Dimensions and other base Constants

#define SCREEN_H 528
#define SCREEN_W 320

#define NUM_LIST ["1","2","3","4","5","6","7","8","9","0"] 

#define KBD_H 260
#define TAB_H 30
#define PICK_HEADER_H 40
#define PICK_FOOTER_H 45
#define PICK_ITEM_H 50

// THEMES
/*
an unsafe method for conversion to rgb to either rgb 565 or rgb 555
*/
private int[] C_RGB( int r, int g, int b);

/*
Safe method for conversion to rgb to either rgb 565 or rgb 555
*/
public inline int[] safe_rgb(int r,int  g,int  b){return C_RGB(r,g,b)};

typedef struct THEME {
  color modal_bg;
  color kbd_bg;
  color key_bg;
  color key_spec;
  color key_out;
  color txt;
  color txt_dim;
  color accent;
  color txt_acc;
  color hl;
  color check;
} THEME;

THEME light = {
  C_WHITE, 
  C_WHITE,
  safe_rgb(28,29,28),
  C_DARK,
  safe_rgb(4, 4, 4),
  safe_rgb(8, 8, 8),
  safe_rgb(1, 11, 26),
  C_WHITE,
  safe_rgb(28, 29, 28),
  C_WHITE
}; // I think this is how you make an  instance of a struct ?
