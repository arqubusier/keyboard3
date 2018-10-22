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
union KeyIndex{
  uint8_t report_key;
  uint8_t mod_mask;
};

union KeyVal{
  byte key;
  byte mod;
};

struct KeySym{
  KeyState effect;
  KeyVal val;
};

struct KeyStatus{
  KeyState state;
  unsigned long last_change;
  KeyIndex index;

  bool isUp(){return state == KeyState::UP;}
  bool isDown(){return state != KeyState::UP;}

  bool no_change(KeyInValue read_val){
    return ( (isUp() && read_val == KeyInValue::UP)
             || (isDown() && read_val == KeyInValue::DOWN) );
  }

  void key_down(KeySym key_sym, KeyReport &report){
    for (size_t key_i = 0; key_i < dim(report.keys); key_i++){
      if (report.keys[key_i] == KEY_REPORT_KEY_AVAILABLE){
        report.keys[key_i] = key_sym.val.key;
        this->index.report_key = key_i;
        this->state = key_sym.effect;
        DEBUGV(key_i);
        DEBUGV(key_sym.val.key);
        return;
      }
    }
    this->state = KeyState::UP;
  }

  void up2down(KeySym key_sym, unsigned long time_up, KeyReport &report){
    Serial.println("Key UP -> DOWN");
    switch (key_sym.effect){
    case KeyState::KEY_DOWN:
      key_down(key_sym, report);
      return;
    case KeyState::MOD_DOWN:
      report.modifiers |= key_sym.val.mod;
      this->index.mod_mask = key_sym.val.mod;
      this->state = key_sym.effect;
      DEBUGV(report.modifiers);
      return;
    case KeyState::FUN_DOWN:
      this->state = key_sym.effect;
      return;
    default:
      this->state = KeyState::UP;
    }

    if (state != KeyState::UP)
      last_change = time_up;
  }

  void down2up(unsigned long time_down, KeyReport &report){
    Serial.println("Key DOWN -> UP");
    uint8_t clear_index;
    switch (state){
    case KeyState::KEY_DOWN:
      clear_index = index.report_key;
      // assert( clear_key_i < dim(report.keys))
      report.keys[clear_index] = KEY_REPORT_KEY_AVAILABLE;
      DEBUGV(index.report_key);
      break;
    case KeyState::MOD_DOWN:
      report.modifiers &= ~(index.mod_mask);
      DEBUGV(index.mod_mask);
      DEBUGV(~index.mod_mask);
      DEBUGV(report.modifiers);
      break;
    default:
      ;
    }
    state = KeyState::UP;
    last_change = time_down;
  }
};

void sendKey(byte key, byte modifiers = 0);

// Create an empty KeyReport
KeyReport report = {0};

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
    {{KeyState::MOD_DOWN, MOD_LSHIFT}, {KeyState::MOD_DOWN, MOD_LCTRL}},
    {{KeyState::KEY_DOWN, KEY_SW_C},   {KeyState::KEY_DOWN, KEY_SW_D}},
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
      status_table[in_i][out_i] = {KeyState::UP, 0, 0};
    }
  }

  Serial.begin(9600);
}



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
      KeyInValue read_val = static_cast<KeyInValue>(in_val);

      if (status.no_change(read_val))
        continue;

      unsigned long time_now = millis();
      if ( (time_now - status.last_change) < debounce_ms )
        continue;

      if (status.isUp() && read_val == KeyInValue::DOWN){
        KeySym key_sym = symbol_table[layer][in_i][out_i];
        status.up2down(key_sym, time_now, report);
        if (status.isUp())
          has_keys_changed = true;
      }
      else if (status.isDown() && read_val == KeyInValue::UP){
        status.down2up(time_now, report);
        has_keys_changed = true;
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
