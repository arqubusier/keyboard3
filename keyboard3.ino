/* Keyboard3 Keyboard firmware
   by: Herman Lundkvist
   date: 2018
   license: MIT license - feel free to use this code in any way
   you see fit. If you go on to use it in another project, please
   keep this license on there.


   Started from an example
   by: Jim Lindblom
   date: January 5, 2012
   license: MIT license - feel free to use this code in any way
   you see fit. If you go on to use it in another project, please
   keep this license on there.
*/

//#define DEBUG

#include <HID.h>
#include <Wire.h>
#include <Keyboard.h>
#include "key_codes.h"

#ifdef DEBUG
#define DEBUGV(var) Serial.print(#var); Serial.print(var); Serial.println("")
#else
#define DEBUGV(var)
#endif

/** Get the size of a fixed size array in compile-time */
template <typename T, size_t N>
constexpr int dim(T(&)[N]){return N;}

template <typename T, size_t N>
void print_array(T(& arr)[N])
{
  for (size_t i; i < N; i++){
    Serial.print(arr[i]);
    Serial.print(", ");
  }
  Serial.println("");
}

const int RXLED=17;
const uint8_t KEY_REPORT_KEY_AVAILABLE = 0;
const int ACTIVE_COLUMN = LOW;
const int DISABLED_COLUMN = HIGH;
const int IN_MODE = INPUT_PULLUP;
enum struct KeyInValue {UP=DISABLED_COLUMN, DOWN=ACTIVE_COLUMN};
enum struct KeyState {KEY_DOWN, MOD_DOWN, FUN_DOWN, UP};

const uint8_t FUN_PREFIX_MASK = 0xC0;
const uint8_t FUN_SUB_MASK = 0x3F;
const uint8_t FUN_LOFFS=0x00;
const uint8_t FUN_LBASE=0x40;
const uint8_t FUN_MACRO=0x80;
const uint8_t FUN_OTHER=0xC0;

struct KeyboardState{
  static const size_t N_LAYERS = 3;
  uint8_t layer_base;
  uint8_t layer_offset;
  KeyReport report;
  uint8_t layer(){
    uint8_t layer=layer_base+layer_offset;
    if (layer>N_LAYERS)
      return 0;
    return layer;
  }
};

struct KeySym{
  KeyState effect;
  byte val;
};

struct KeyStatus{
  KeyState state;
  unsigned long last_change;
  uint8_t reset_val;

  bool isUp(){return state == KeyState::UP;}
  bool isDown(){return state != KeyState::UP;}

  bool no_change(KeyInValue read_val){
    return ( (isUp() && read_val == KeyInValue::UP)
             || (isDown() && read_val == KeyInValue::DOWN) );
  }

  void key_down(KeySym key_sym, KeyReport &report){
    for (size_t key_i = 0; key_i < dim(report.keys); key_i++){
      if (report.keys[key_i] == KEY_REPORT_KEY_AVAILABLE){
        report.keys[key_i] = key_sym.val;
        reset_val = key_i;
        state = key_sym.effect;
        return;
      }
    }
    this->state = KeyState::UP;
  }

  void fun_down(byte fun_val, KeyboardState &keyboard){
    auto fun_content = fun_val & FUN_SUB_MASK;
    reset_val = 0;
    switch (FUN_PREFIX_MASK & fun_val){
    case FUN_LOFFS:
      keyboard.layer_offset = fun_content;
      break;
    case FUN_LBASE:
      break;
    case FUN_MACRO:
      break;
    case FUN_OTHER:
      break;
    default:
      ;
    }
    this->state = KeyState::FUN_DOWN;
  }

  void fun_up(KeyboardState &keyboard){
    reset_val = 0;
    switch (FUN_PREFIX_MASK & reset_val){
    case FUN_LOFFS:
      keyboard.layer_offset = 0;
      break;
    case FUN_LBASE:
      break;
    case FUN_MACRO:
      break;
    case FUN_OTHER:
      break;
    default:
      ;
    }
    this->state = KeyState::FUN_DOWN;
  }

  bool up2down(KeySym key_sym, unsigned long time_up, KeyboardState &keyboard){
    #ifdef DEBUG
    Serial.println("Key UP -> DOWN");
    #endif
    switch (key_sym.effect){
    case KeyState::KEY_DOWN:
      key_down(key_sym, keyboard.report);
      DEBUGV(reset_val);
      DEBUGV(key_sym.val);
      break;
    case KeyState::MOD_DOWN:
      keyboard.report.modifiers |= key_sym.val;
      reset_val = key_sym.val;
      state = key_sym.effect;
      DEBUGV(keyboard.report.modifiers);
      break;
    case KeyState::FUN_DOWN:
      fun_down(key_sym.val, keyboard);
      DEBUGV(keyboard.layer_offset);
      break;
    default:
      this->state = KeyState::UP;
    }

    if (state != KeyState::UP){
      last_change = time_up;
      if (state != KeyState::FUN_DOWN)
        return true;
    }
    return false;
  }

  bool down2up(unsigned long time_down, KeyboardState &keyboard){
    #ifdef DEBUG
    Serial.println("Key DOWN -> UP");
    #endif
    uint8_t clear_index;
    switch (state){
    case KeyState::KEY_DOWN:
      clear_index = reset_val;
      // assert( clear_key_i < dim(report.keys))
      keyboard.report.keys[clear_index] = KEY_REPORT_KEY_AVAILABLE;
      DEBUGV(reset_val);
      break;
    case KeyState::MOD_DOWN:
      keyboard.report.modifiers &= ~(reset_val);
      DEBUGV(keyboard.report.modifiers);
      break;
    case KeyState::FUN_DOWN:
      fun_up(keyboard);
      DEBUGV(keyboard.layer_offset);
    default:
      ;
    }

    bool notify_key_change = (state != KeyState::FUN_DOWN);
    state = KeyState::UP;
    last_change = time_down;
    return notify_key_change;
  }
};

void sendKey(byte key, byte modifiers = 0);

// Create an empty KeyReport
KeyReport report = {0};

// Currently out pins correspond to rows
const int OUT_PINS[] {4, 5, 6, 7};
// Currently in pins correspond to columns
const int IN_PINS[]  {10, 16, 14, 15, 18, 19};

const unsigned long DEBOUNCE_MS = 5;
const uint8_t KEY_REPORT_DONT_CLEAR_KEY = 255;

KeyboardState keyboard_state;
KeyStatus status_table[dim(OUT_PINS)][dim(IN_PINS)];
KeyStatus status_table1[dim(OUT_PINS)][dim(IN_PINS)];
//NOTE: for keymaps with duplicate keys, duplicate key reports may be sent.
  const KeySym SYMBOL_TABLE[KeyboardState::N_LAYERS][dim(OUT_PINS)][dim(IN_PINS)] =
    {
#include "left.hpp"
    };

const size_t MATRIX1_N_OUTS = 4;
const size_t MATRIX1_N_INS = 6;
const size_t MATRIX1_N_COLS = MATRIX1_N_INS;
const size_t MATRIX1_N_ROWS = MATRIX1_N_OUTS;
  const KeySym SYMBOL_TABLE1[KeyboardState::N_LAYERS][MATRIX1_N_ROWS][MATRIX1_N_COLS] =
    {
#include "right.hpp"
    };

/**
 * Check if key is pressed or relasead, and update its status accordingly.
 *
 * Returns true if there was a change compared to the key's last status.
 */
template<size_t N_LAYERS, size_t N_COLS, size_t N_ROWS>
bool update_key(KeyInValue read_val, size_t row, size_t col,
                const  KeySym (&SYM_TABLE) [N_LAYERS][N_ROWS][N_COLS],
                KeyStatus(&stat_table)[N_ROWS][N_COLS],
                KeyboardState &keyboard_state){
  KeyStatus &key_status = stat_table[row][col];
  if (key_status.no_change(read_val))
    return false;

  unsigned long time_now = millis();
  if ( (time_now - key_status.last_change) < DEBOUNCE_MS )
    return false;

  DEBUGV(row);
  DEBUGV(col);


  if (key_status.isUp() && read_val == KeyInValue::DOWN){
    KeySym key_sym = SYM_TABLE[keyboard_state.layer()][row][col];
    DEBUGV(keyboard_state.layer());
    if (key_status.up2down(key_sym, time_now, keyboard_state))
      return true;
  }
  else if (key_status.isDown() && read_val == KeyInValue::UP){
    if (key_status.down2up(time_now, keyboard_state))
      return true;
  }

  return false;
}

const int MCP23017_SLAVE_ADDR = 0x20;
const int MCP23017_IODIRA_ADDR_BANK0 = 0x00;
const int MCP23017_IODIRB_ADDR_BANK0 = 0x01;
const int MCP23017_GPIOA_ADDR_BANK0 = 0x12;
const int MCP23017_GPIOB_ADDR_BANK0 = 0x13;
const int MCP23017_GPPUA_ADDR_BANK0 = 0x0C;
const int MCP23017_GPPUB_ADDR_BANK0 = 0x0D;

int counter = 0;
int debug_led_state;
void setup()
{
  keyboard_state.layer_base = 0;
  keyboard_state.layer_offset = 0;
  for (size_t i = 0; i < dim(report.keys); i++){
    keyboard_state.report.keys[i] = KEY_REPORT_KEY_AVAILABLE;
  }

  for (size_t out_i = 0; out_i < dim(OUT_PINS); out_i++){
    int out_pin = OUT_PINS[out_i];
    pinMode(out_pin, OUTPUT);
    digitalWrite(out_pin, DISABLED_COLUMN);
  }

  for (size_t in_i = 0; in_i < dim(IN_PINS); in_i++){
    int in_pin = IN_PINS[in_i];
    pinMode(in_pin, IN_MODE);
  }

  for (size_t in_i = 0; in_i < dim(IN_PINS); in_i++){
    for (size_t out_i = 0; out_i < dim(OUT_PINS); out_i++){
      status_table[out_i][in_i] = {KeyState::UP, 0, 0};
      status_table1[out_i][in_i] = {KeyState::UP, 0, 0};
    }
  }

  Serial.begin(9600);
  //RXLED is used for debugging purposes.
  pinMode(RXLED, OUTPUT);

  Wire.begin();
  //Set GPIOA to be output (register bitval 0 <=> pin == 0)
  //GPIOB is input by default (IODIRB POR = 0xFF).
  Wire.beginTransmission(MCP23017_SLAVE_ADDR);
  Wire.write(MCP23017_IODIRA_ADDR_BANK0);
  Wire.write(0x00);
  Wire.endTransmission();
  delayMicroseconds(2);
  //Set GPIOA to enable pull-ups
  Wire.beginTransmission(MCP23017_SLAVE_ADDR);
  Wire.write(MCP23017_GPPUB_ADDR_BANK0);
  Wire.write(0xFF);
  Wire.endTransmission();
}



void loop()
{
  counter++;
  if ((counter % 500) == 0){
    debug_led_state = (debug_led_state == HIGH) ? LOW: HIGH;
    digitalWrite(RXLED, debug_led_state);
  }

  bool notify_key_change = false;
  //
  //    Keys on the circuit local to the mcu
  //

  for (size_t out_i = 0; out_i < dim(OUT_PINS); out_i++){
#ifdef DEBUG
    Serial.print("------------ SCAN 0 ROW ");
    Serial.print(out_i);
    Serial.print(" of ");
    Serial.print(dim(OUT_PINS) - 1);
    Serial.println("------------");
#endif
    int out_pin = OUT_PINS[out_i];
    size_t prev_out_i = (out_i + dim(OUT_PINS) - 1 ) % dim(OUT_PINS);
    int prev_out_pin = OUT_PINS[prev_out_i];
    digitalWrite(prev_out_pin, DISABLED_COLUMN);
    digitalWrite(out_pin, ACTIVE_COLUMN);

    for (size_t in_i = 0; in_i < dim(IN_PINS); in_i++){
      int in_pin = IN_PINS[in_i];
      int in_val = digitalRead(in_pin);
      KeyInValue read_val = static_cast<KeyInValue>(in_val);
      if (update_key(read_val, out_i, in_i, SYMBOL_TABLE, status_table, keyboard_state))
        notify_key_change = true;
    }
#ifdef DEBUG
    Serial.println("------------ SCAN 0 DONE --------------");
    //wait for the user to send something before continuing
    while (Serial.available() == 0){
      ;
    }
    Serial.read();
#endif
  }

  //
  // Keys on the circuit connected via i2c
  //
  for (size_t out_i = 0; out_i < MATRIX1_N_OUTS; out_i++){
    byte out_val = ~(1 << out_i);
    byte in_val = 0;
#ifdef DEBUG
    Serial.print("------------ SCAN 1 ROW ");
    Serial.print(out_i);
    Serial.print(" of ");
    Serial.print(dim(OUT_PINS) - 1);
    Serial.println("");
    DEBUGV(out_val);
    Serial.println("------------");
#endif
    Wire.beginTransmission(MCP23017_SLAVE_ADDR);
    Wire.write(MCP23017_GPIOA_ADDR_BANK0);
    Wire.write(out_val);
    Wire.endTransmission();
    delayMicroseconds(100);
    Wire.requestFrom(MCP23017_SLAVE_ADDR, 1);
    while(Wire.available()){
      in_val = Wire.read();
    }

    for (size_t in_i = 0; in_i < MATRIX1_N_INS; in_i++){
      KeyInValue read_val = static_cast<KeyInValue>( (in_val >> in_i) & 0x01 );
      if (update_key(read_val, out_i, in_i, SYMBOL_TABLE1, status_table1, keyboard_state))
        notify_key_change = true;
    }
#ifdef DEBUG
    Serial.println("------------ SCAN 1 DONE --------------");
    //wait for the user to send something before continuing
    while (Serial.available() == 0){
      ;
    }
    Serial.read();
#endif
  }

  if (notify_key_change){
    Serial.println("Send Key Report");
#ifdef DEBUG
    Serial.println("Send Key Report");
#else
    Keyboard.sendReport(&keyboard_state.report);  // send the KeyReport
#endif
  }


}

void sendKey(byte key, byte modifiers)
{
  KeyReport report = {0};  // Create an empty KeyReport

  /* First send a report with the keys and modifiers pressed */
  report.keys[0] = key;  // set the KeyReport to key
  report.modifiers = modifiers;  // set the KeyReport's modifiers
  report.reserved = 1;
  Keyboard.sendReport(&report);  // send the KeyReport

  /* Now we've got to send a report with nothing pressed */
  for (int i=0; i<6; i++)
    report.keys[i] = 0;  // clear out the keys
  report.modifiers = 0x00;  // clear out the modifires
  report.reserved = 0;
  Keyboard.sendReport(&report);  // send the empty key report
}
