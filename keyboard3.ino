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


enum struct KeyState {UP=HIGH, DOWN=LOW};

struct KeyStatus{
  KeyState state;
  unsigned long last_change;
  uint8_t report_key_i;
};

enum struct KeyType {KEY, MOD, FUN};

union KeyVal{
  byte key;
  byte mod;
};

struct KeySym{
  KeyType type;
  KeyVal val;

  /*
    KeySym(byte val): type(KeyType::KEY){
    this->val.key=val;
    }
  */
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
KeySym symbol_table[layers][dim(in_pins)][dim(out_pins)] =
  {
   {
    {{KeyType::KEY, KEY_SW_A}, {KeyType::KEY, KEY_SW_B}},
    {{KeyType::KEY, KEY_SW_C}, {KeyType::KEY, KEY_SW_D}},
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
      status_table[in_i][out_i] = {KeyState::UP, 0, KEY_REPORT_DONT_CLEAR_KEY};
    }
  }

  Serial.begin(9600);
}

#define DEBUGV(var) Serial.print(#var); Serial.print(var); Serial.println("")
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
        Serial.println("Key UP -> DOWN");
        //set key in keyreport
        for (size_t key_i = 0; key_i < dim(report.keys); key_i++){
          if (report.keys[key_i] == KEY_REPORT_KEY_AVAILABLE){
            KeySym key_sym = symbol_table[layer][in_i][out_i];
            switch (key_sym.type){
            case KeyType::KEY:
              report.keys[key_i] = key_sym.val.key;
              status.report_key_i = key_i;
              break;
            case KeyType::MOD:
              break;
            case KeyType::FUN:
              break;
            default:
              ;//SHOULD ONLY HAPPEN WITH INVALID SYMBOL_TABLE
            }

            status.last_change = time_now;
            status.state = KeyState::DOWN;
            has_keys_changed = true;

            DEBUGV(key_i);
            DEBUGV(key_sym.val.key);
            break;
          }
        }
      }
      else if (status.state == KeyState::DOWN && new_state == KeyState::UP){
        Serial.println("Key DOWN -> UP");
        size_t clear_key_i = status.report_key_i;
        if ( clear_key_i < dim(report.keys)){
          report.keys[clear_key_i] = KEY_REPORT_KEY_AVAILABLE;
          status.last_change = time_now;
          has_keys_changed = true;
          status.state = KeyState::UP;

          DEBUGV(status.report_key_i);
        }
        else{
          //SHOULD NOT HAPPEN => ERROR IN CODE
        }
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
