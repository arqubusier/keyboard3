/*
  Test example ardunio pro micro connected to a mcp23017 via i2c/TWI.

  GPIOA pins connected to GPIOB pins on the mcp23017.

  Toggle values 0xFF-0x00 on GPIOA.
  Read values 0xFF-0x00 on GPIOA.
 */
#include <Wire.h>


const int MCP23017_SLAVE_ADDR = 0x20;
const int MCP23017_IODIR_ADDR_BANK0 = 0x00;
const int MCP23017_GPIOA_ADDR_BANK0 = 0x12;
byte out_val = 0xff;

void setup() {
  Wire.begin();        // join i2c bus (address optional for master)
  Serial.begin(9600);  // start serial for output

  //Set GPIOA to be output. GPIOB is input by default.
  Wire.beginTransmission(MCP23017_SLAVE_ADDR);
  Wire.write(MCP23017_IODIR_ADDR_BANK0);
  Wire.write(0x00);
  Wire.endTransmission();
}

//Note sequential operation is enabled by default.
//Thus the read request will be from the register address of the last write plus one.
//Since the address of GPIOB follows that of GPIOB when BANK=0 (which it is by default),
//The read will be from GPIOB.
void loop() {
  Serial.print("Out Val ");
  Serial.print(out_val);
  Serial.println("");

  Wire.beginTransmission(MCP23017_SLAVE_ADDR);
  Wire.write(MCP23017_GPIOA_ADDR_BANK0);
  Wire.write(out_val);
  Wire.endTransmission();
  delay(1);

  Wire.requestFrom(MCP23017_SLAVE_ADDR, 1);    // request 6 bytes from slave device #2
  while(Wire.available())    // slave may send less than requested
  {
    byte in_val = Wire.read();    // receive a byte as character
    Serial.print("in_val: ");
    Serial.print(in_val);         // print the character
    Serial.println("");
  }

  out_val = ~out_val;

  delay(2000);
}
