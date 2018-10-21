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

enum struct KeyType {KEY, MOD, FUN, NONE};
enum struct KeyState {UP=HIGH, DOWN=LOW};

union KeyIndex{
  uint8_t report_key;
  uint8_t mod_mask;
};

struct KeyStatus{
  KeyState state;
  unsigned long last_change;
  KeyType pressed_type;
  KeyIndex index;
};


union KeyVal{
  byte key;
  byte mod;
};

struct KeySym{
  KeyType type;
  KeyVal val;
};


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

void sendKey(byte key, byte modifiers = 0);

// Create an empty KeyReport
KeyReport report = {0};
const uint8_t KEY_REPORT_KEY_AVAILABLE = 0;

// enable pin for main loop, active low.
int enable = 9;
// Currently in pins correspond to rows
int in_pins[] {4, 5};
// Currently out pins correspond to columns
int out_pins[]  {10, 16};

unsigned long debounce_ms = 5;
const uint8_t KEY_REPORT_DONT_CLEAR_KEY = 255;

KeyStatus status_table[dim(in_pins)][dim(out_pins)];
const size_t layers = 1;
size_t layer = 0;
//NOTE: for keymaps with duplicate keys, duplicate key reports may be sent.
KeySym symbol_table[layers][dim(in_pins)][dim(out_pins)] =
  {
   {
    {{KeyType::MOD, MOD_LSHIFT}, {KeyType::MOD, MOD_LCTRL}},
    {{KeyType::KEY, KEY_SW_C},   {KeyType::KEY, KEY_SW_D}},
   },
  };


void setup()
{
  for (size_t i = 0; i < dim(report.keys); i++){
    report.keys[i] = KEY_REPORT_KEY_AVAILABLE;
  }

  for (size_t out_i = 0; out_i < dim(out_pins); out_i++){
    int out_pin = out_pins[out_i];
    pinMode(out_pin, OUTPUT);
    digitalWrite(out_pin, HIGH);
  }

  for (size_t in_i = 0; in_i < dim(in_pins); in_i++){
    int in_pin = in_pins[in_i];
    pinMode(in_pin, INPUT);
    digitalWrite(in_pin, HIGH);
  }

  //Enable pin
  pinMode(enable, INPUT);
  digitalWrite(enable, HIGH);

  for (size_t in_i = 0; in_i < dim(in_pins); in_i++){
    for (size_t out_i = 0; out_i < dim(out_pins); out_i++){
      status_table[in_i][out_i] = {KeyState::UP, 0, KeyType::NONE, 0};
    }
  }

  Serial.begin(9600);
}

#define DEBUGV(var) Serial.print(#var); Serial.print(var); Serial.println("")

bool key_down(KeySym key_sym, KeyStatus& status){
  for (size_t key_i = 0; key_i < dim(report.keys); key_i++){
    if (report.keys[key_i] == KEY_REPORT_KEY_AVAILABLE){
      report.keys[key_i] = key_sym.val.key;
      status.index.report_key = key_i;
      status.pressed_type = key_sym.type;
      DEBUGV(key_i);
      DEBUGV(key_sym.val.key);
      return true;
    }
  }
  return false;
}

bool handle_up2down(KeySym key_sym, KeyStatus& status, unsigned long time_now){
  bool update_key = false;
  Serial.println("Key UP -> DOWN");
  switch (key_sym.type){
  case KeyType::KEY:
    update_key = key_down(key_sym, status);
    break;
  case KeyType::MOD:
    report.modifiers |= key_sym.val.mod;
    status.index.mod_mask = key_sym.val.mod;
    status.pressed_type = KeyType::MOD;
    DEBUGV(report.modifiers);
    update_key = true;
    break;
  case KeyType::FUN:
    break;
  default:
    ;//SHOULD ONLY HAPPEN WITH INVALID SYMBOL_TABLE
  }

  if (update_key){
    status.last_change = time_now;
    status.state = KeyState::DOWN;
  }
  return update_key;
}

bool handle_down2up(KeyStatus& status, unsigned long time_now){
  Serial.println("Key DOWN -> UP");
  uint8_t clear_index;
  switch (status.pressed_type){
  case KeyType::KEY:
    clear_index = status.index.report_key;
    // assert( clear_key_i < dim(report.keys))
    report.keys[clear_index] = KEY_REPORT_KEY_AVAILABLE;
    DEBUGV(status.index.report_key);
    break;
  case KeyType::MOD:
    report.modifiers &= ~(status.index.mod_mask);
    DEBUGV(status.index.mod_mask);
    DEBUGV(~status.index.mod_mask);
    DEBUGV(report.modifiers);
    break;
  default:;
  }
  status.last_change = time_now;
  status.state = KeyState::UP;

  return true;
}



//TODO: merge status.state and pressed type?
void loop()
{

  //  Only continue when enable pin is low
  if (digitalRead(enable))
  {
    return;
  }

  bool has_keys_changed = false;
  for (size_t out_i = 0; out_i < dim(out_pins); out_i++){
    int out_pin = out_pins[out_i];
    size_t prev_out_i = (out_i + dim(out_pins) - 1 ) % dim(out_pins);
    int prev_out_pin = out_pins[prev_out_i];
    digitalWrite(prev_out_pin, HIGH);
    digitalWrite(out_pin, LOW);

    for (size_t in_i = 0; in_i < dim(in_pins); in_i++){
      int in_pin = in_pins[in_i];
      int in_val = digitalRead(in_pin);
      KeyStatus &status = status_table[in_i][out_i];
      KeyState new_state = static_cast<KeyState>(in_val);

      if (status.state == new_state)
        continue;

      unsigned long time_now = millis();
      if ( (time_now - status.last_change) < debounce_ms )
        continue;

      if (status.state == KeyState::UP && new_state == KeyState::DOWN){
        KeySym key_sym = symbol_table[layer][in_i][out_i];
        has_keys_changed = has_keys_changed
          || handle_up2down(key_sym, status, time_now);
      }
      else if (status.state == KeyState::DOWN && new_state == KeyState::UP){
        has_keys_changed = has_keys_changed
          || handle_down2up(status, time_now);
      }
    }
    delay(100);  // Delay so as not to spam the computer
  }

  if (has_keys_changed){
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
