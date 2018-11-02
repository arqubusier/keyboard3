/*
  Test example ardunio pro micro connected to mcp23017 via i2c/TWI.

  Toggle values 0xFF-0x00 on GPIOA.
 */
#include <Wire.h>

void setup() {
  Wire.begin();        // join i2c bus (address optional for master)
  Serial.begin(9600);  // start serial for output
}

const int MCP23017_SLAVE_ADDR = 0x20;
const int MCP23017_IODIR_ADDR_BANK0 = 0x00;
const int MCP23017_GPIOA_ADDR_BANK0 = 0x12;

byte val = 0xff;
void loop() {
  Wire.beginTransmission(MCP23017_SLAVE_ADDR);
  Wire.write(MCP23017_IODIR_ADDR_BANK0);
  Wire.write(0x00);
  Wire.endTransmission();

  delay(1);

  Wire.beginTransmission(MCP23017_SLAVE_ADDR);
  Wire.write(MCP23017_GPIOA_ADDR_BANK0);
  Wire.write(val);
  Wire.endTransmission();

  val = ~val;
  Serial.print("Val ");
  Serial.print(val);
  Serial.println("");

  delay(2000);
}
