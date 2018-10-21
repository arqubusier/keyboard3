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


int in_pins[] {4, 5};
int out_pins[]  {10, 16};
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

  Serial.begin(9600);
}

void loop()
{
  //  Only continue when enable pin is low
  if (digitalRead(enable))
  {
    return;
  }

  for (size_t out_i = 0; out_i < dim(out_pins); out_i++){
    int out_pin = out_pins[out_i];
    size_t prev_out_i = (out_i + dim(out_pins) - 1 ) % dim(out_pins);
    int prev_out_pin = out_pins[prev_out_i];
    digitalWrite(prev_out_pin, HIGH);
    digitalWrite(out_pin, LOW);

    for (size_t in_i = 0; in_i < dim(in_pins); in_i++){
      int in_pin = in_pins[in_i];
      if (digitalRead(in_pin) == LOW){
        sendKey(KEY_SW_A, 0);
      }
      pin_status(in_pins);
      pin_status(out_pins);
      delay(1000);  // Delay so as not to spam the computer
    }
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
