/* keyPadHiduino Advanced Example Code
   by: Jim Lindblom
   date: January 5, 2012
   license: MIT license - feel free to use this code in any way
   you see fit. If you go on to use it in another project, please
   keep this license on there.
*/

#include <HID.h>
#include <Keyboard.h>
#include "key_codes.h"


/** Get the size of a fixed size array in compile-time */
template <typename T, size_t N>
constexpr int dim(T(&)[N]){return N;}

template <typename T, size_t N>
void pin_status(T(& arr)[N])
{
  for (size_t i; i < N; i++){
    Serial.print(arr[i]);
    Serial.print(", ");
  }
  Serial.println("");
}

void sendKey(byte key, byte modifiers = 0);
// enable pin for main loop, active low.
int enable = 9;


// Currently in pins correspond to rows
int in_pins[] {4, 5};
// Currently out pins correspond to columns
int out_pins[]  {10, 16};

enum KeyState {KEY_STATE_UP=HIGH, KEY_STATE_DOWN=LOW};

unsigned long debounce_ms = 5;
const uint8_t KEY_REPORT_DONT_CLEAR_KEY = 255;
struct KeyStatus{
  KeyState state;
  unsigned long last_change;
  uint8_t report_key_i;
};

union KeyVal{
  byte key;
  byte mod;
};

struct KeySym{
  bool is_key;
  KeyVal val;

  KeySym(byte val): is_key(true){
    this->val.key=val;
  }
};

KeyStatus status_table[dim(in_pins)][dim(out_pins)];
const size_t layers = 1;
KeySym symbol_table[layers][dim(in_pins)][dim(out_pins)] =
  {
   {
    {KeySym(KEY_SW_A), KeySym(KEY_SW_B)},
    {KeySym(KEY_SW_B), KeySym(KEY_SW_C)},
   },
  };

// Create an empty KeyReport
KeyReport report = {0};
const uint8_t KEY_REPORT_KEY_AVAILABLE = 0;
size_t layer = 0;

void setup()
{
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
      status_table[in_i][out_i] = {KEY_STATE_UP, 0, KEY_REPORT_DONT_CLEAR_KEY};
    }
  }

  Serial.begin(9600);
}

bool key_ready(KeyStatus status){
  unsigned long time_now = millis();
  return ( (time_now - status.last_change) > debounce_ms );
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
      KeyStatus status = status_table[in_i][out_i];

      if (status.state == KEY_STATE_UP && in_val == KEY_STATE_DOWN){
        unsigned long time_now = millis();
        if ( (time_now - status.last_change) < debounce_ms )
          continue;

        Serial.println("Key UP -> DOWN");
        if ( !key_ready(status) )
        //set key in keyreport
        for (size_t key_i = 0; key_i < dim(report.keys); key_i++){
          if (report.keys[key_i] == KEY_REPORT_KEY_AVAILABLE){
            KeySym key_sym = symbol_table[layer][in_i][out_i];
            if (key_sym.is_key){
              report.keys[key_i] = key_sym.val.key;
              status.report_key_i = key_i;
            }
            status.last_change = time_now;
            has_keys_changed = true;
            status.state = KEY_STATE_DOWN;

            DEBUGV(key_i);
            DEBUGV(key_sym.val.key);
          }
        }
      }
      else if (status.state == KEY_STATE_DOWN && in_val == KEY_STATE_UP){
        unsigned long time_now = millis();
        if ( (time_now - status.last_change) < debounce_ms )
          continue;

        Serial.println("Key DOWN -> UP");
        size_t clear_key_i = status.report_key_i;
        if ( clear_key_i < dim(report.keys)){
          report.keys[clear_key_i] = KEY_REPORT_KEY_AVAILABLE;
          status.last_change = time_now;
          has_keys_changed = true;
          status.state = KEY_STATE_UP;

          DEBUGV(status.report_key_i);
        }
        else{
          //ASSERT SHOULD NOT HAPPEN => ERROR IN CODE
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
