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

#include <HID.h>
#include <Keyboard.h>
#include "key_codes.h"

#define DEBUGV(var) Serial.print(#var); Serial.print(var); Serial.println("")

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

const uint8_t KEY_REPORT_KEY_AVAILABLE = 0;
enum struct KeyInValue {UP=HIGH, DOWN=LOW};
enum struct KeyState {KEY_DOWN, MOD_DOWN, FUN_DOWN, UP};

const uint8_t FUN_PREFIX_MASK = 0xC0;
const uint8_t FUN_SUB_MASK = 0x3F;
const uint8_t FUN_LOFFS=0x00;
const uint8_t FUN_LBASE=0x04;
const uint8_t FUN_MACRO=0x80;
const uint8_t FUN_OTHER=0xC0;

struct KeyboardState{
  static const size_t N_LAYERS = 64;
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
      reset_val = keyboard.layer_offset;
      keyboard.layer_offset = fun_content;
      Serial.println("LayerOffset Down");
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
    uint8_t old_layer=0;
    auto fun_content = reset_val & FUN_SUB_MASK;
    switch (FUN_PREFIX_MASK & reset_val){
    case FUN_LOFFS:
      old_layer = FUN_SUB_MASK & fun_content;
      reset_val = 0;
      keyboard.layer_offset = old_layer;
      Serial.println("LayerOffset UP");
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
    Serial.println("Key UP -> DOWN");
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
    Serial.println("Key DOWN -> UP");
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

// enable pin for main loop, active low.
const int ENABLE_PIN = 9;
// Currently in pins correspond to rows
const int IN_PINS[] {4, 5};
// Currently out pins correspond to columns
const int OUT_PINS[]  {10, 16};

const unsigned long DEBOUNCE_MS = 5;
const uint8_t KEY_REPORT_DONT_CLEAR_KEY = 255;

KeyboardState keyboard_state;
KeyStatus status_table[dim(IN_PINS)][dim(OUT_PINS)];
//NOTE: for keymaps with duplicate keys, duplicate key reports may be sent.
#define EMPTY_ROW { {{KeyState::UP, 0}, {KeyState::UP, 0}}, {{KeyState::UP, 0}, {KeyState::UP, 0}} }
#define K_UP KeyState::UP
#define K_DN KeyState::KEY_DOWN
#define M_DN KeyState::MOD_DOWN
#define F_DN KeyState::FUN_DOWN
#define F_LO FUN_LOFFS
#define F_LB FUN_LBASE
#define F_MO FUN_MACRO
#define F_OT FUN_OTHER
  const KeySym SYMBOL_TABLE[KeyboardState::N_LAYERS][dim(IN_PINS)][dim(OUT_PINS)] =
  {
   {
    {{M_DN, MOD_LSHIFT}, {F_DN, 0x01}},
    {{K_DN, KEY_SW_C  }, {K_DN, KEY_SW_D }},
   },
   {
    {{K_DN, KEY_SW_A  }, {F_DN, F_LO&0x01}},
    {{F_DN, 0x02 }, {K_DN, KEY_SW_B }},
   },
   {
    {{M_DN, MOD_LCTRL }, {F_DN, F_LO&0x01}},
    {{F_DN, F_LO&0x02 }, {K_DN, KEY_SW_E }},
   },
   EMPTY_ROW,
   EMPTY_ROW, EMPTY_ROW, EMPTY_ROW, EMPTY_ROW, EMPTY_ROW,
   EMPTY_ROW, EMPTY_ROW, EMPTY_ROW, EMPTY_ROW, EMPTY_ROW,
   EMPTY_ROW, EMPTY_ROW, EMPTY_ROW, EMPTY_ROW, EMPTY_ROW,
   EMPTY_ROW, EMPTY_ROW, EMPTY_ROW, EMPTY_ROW, EMPTY_ROW,
   EMPTY_ROW, EMPTY_ROW, EMPTY_ROW, EMPTY_ROW, EMPTY_ROW,
   EMPTY_ROW, EMPTY_ROW, EMPTY_ROW, EMPTY_ROW, EMPTY_ROW,
   EMPTY_ROW, EMPTY_ROW, EMPTY_ROW, EMPTY_ROW, EMPTY_ROW,
   EMPTY_ROW, EMPTY_ROW, EMPTY_ROW, EMPTY_ROW, EMPTY_ROW,
   EMPTY_ROW, EMPTY_ROW, EMPTY_ROW, EMPTY_ROW, EMPTY_ROW,
   EMPTY_ROW, EMPTY_ROW, EMPTY_ROW, EMPTY_ROW, EMPTY_ROW,
   EMPTY_ROW, EMPTY_ROW, EMPTY_ROW, EMPTY_ROW, EMPTY_ROW,
   EMPTY_ROW, EMPTY_ROW, EMPTY_ROW, EMPTY_ROW, EMPTY_ROW,
  };
#undef K_UP
#undef K_DN
#undef M_DN
#undef F_DN
#undef F_LO
#undef F_LB
#undef F_MO
#undef F_OT


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
    digitalWrite(out_pin, HIGH);
  }

  for (size_t in_i = 0; in_i < dim(IN_PINS); in_i++){
    int in_pin = IN_PINS[in_i];
    pinMode(in_pin, INPUT);
    digitalWrite(in_pin, HIGH);
  }

  pinMode(ENABLE_PIN, INPUT);
  digitalWrite(ENABLE_PIN, HIGH);

  for (size_t in_i = 0; in_i < dim(IN_PINS); in_i++){
    for (size_t out_i = 0; out_i < dim(OUT_PINS); out_i++){
      status_table[in_i][out_i] = {KeyState::UP, 0, 0};
    }
  }

  Serial.begin(9600);
}



void loop()
{

  //  Only continue when enable pin is low
  if (digitalRead(ENABLE_PIN))
    return;

  bool notify_key_change = false;
  for (size_t out_i = 0; out_i < dim(OUT_PINS); out_i++){
    int out_pin = OUT_PINS[out_i];
    size_t prev_out_i = (out_i + dim(OUT_PINS) - 1 ) % dim(OUT_PINS);
    int prev_out_pin = OUT_PINS[prev_out_i];
    digitalWrite(prev_out_pin, HIGH);
    digitalWrite(out_pin, LOW);

    for (size_t in_i = 0; in_i < dim(IN_PINS); in_i++){
      int in_pin = IN_PINS[in_i];
      int in_val = digitalRead(in_pin);
      KeyStatus &key_status = status_table[in_i][out_i];
      KeyInValue read_val = static_cast<KeyInValue>(in_val);

      if (key_status.no_change(read_val))
        continue;

      unsigned long time_now = millis();
      if ( (time_now - key_status.last_change) < DEBOUNCE_MS )
        continue;

      if (key_status.isUp() && read_val == KeyInValue::DOWN){
        KeySym key_sym = SYMBOL_TABLE[keyboard_state.layer()][in_i][out_i];
        DEBUGV(keyboard_state.layer());
        if (key_status.up2down(key_sym, time_now, keyboard_state))
          notify_key_change = true;
      }
      else if (key_status.isDown() && read_val == KeyInValue::UP){
        if (key_status.down2up(time_now, keyboard_state))
          notify_key_change = true;
      }
    }
    delay(100);  // Delay so as not to spam the computer
  }

  if (notify_key_change){
    Serial.println("Send Key Report");
    // Send key report
  }
  else{
    //Serial.println("No Change");
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
