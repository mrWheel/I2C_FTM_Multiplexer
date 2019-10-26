/*
    Program : I2Ctuff (part of I2C_Multiplexer)

    Copyright (C) 2019 Willem Aandewiel

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

//------------------------------------------------------------------
void startI2C()
{
  Wire.end();

  Wire.begin(registerStack.whoAmI);

  // (Re)Declare the Events.
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
  wdt_reset();

} // startI2C()


//------------------------------------------------------------------
boolean isConnected()
{
  Wire.beginTransmission((uint8_t)0);
  if (Wire.endTransmission() != 0) {
    return (false); // Master did not ACK
  }
  wdt_reset();
  return (true);

} // isConnected()


//------------------------------------------------------------------
void processCommand(byte command)
{
  byte GPIO_PIN, PINMODE, HIGH_LOW, GPIOSTATE;

  //Serial.print("Processing command ["); Serial.print(command, BIN); Serial.println("]");
  if ((command & (1<<CMD_PINMODE))) {
    //Serial.print("Command pinMode("); 
    GPIO_PIN = Wire.read();
    //Serial.print(GPIO_PIN);
    //Serial.print(", ");
    PINMODE = Wire.read();
    //Serial.print(HIGH_LOW);
    //Serial.println(")");
    if (GPIO_PIN >= 0 && GPIO_PIN < 16) {
      pinMode(p2r[GPIO_PIN], PINMODE);
    }
  }
  else if ((command & (1<<CMD_DIGITALWRITE))) {
    //Serial.print("Command digitalWrite("); 
    GPIO_PIN = Wire.read();
    //Serial.print(GPIO_PIN);
    //Serial.print(", ");
    HIGH_LOW = Wire.read();
    //Serial.print(HIGH_LOW);
    //Serial.println(")");
    if (GPIO_PIN >= 0 && GPIO_PIN < 16) {
      digitalWrite(p2r[GPIO_PIN], !HIGH_LOW);
    }
  }
  else if ((command & (1<<CMD_DIGITALREAD))) {
    //Serial.print("Command digitalRead("); 
    GPIO_PIN = Wire.read();
    //Serial.print(GPIO_PIN);
    //Serial.print(") => ");
    if (GPIO_PIN >= 0 && GPIO_PIN < 16) {
      registerStack.lastGpioState = !digitalRead(p2r[GPIO_PIN]);
      //Serial.print("State is ["); Serial.print(registerStack.lastGpioState); 
      //Serial.println("]");
    }
  }
  if ((command & (1<<CMD_TESTRELAYS))) {
    testRelays();
  }
  if ((command & (1<<CMD_WRITECONF))) {
    //writeConfig();
  }
  if ((command & (1<<CMD_READCONF))) {
    //readConfig();
  }
  //-----> execute reBoot always last!! <-----
  if ((command & (1<<CMD_REBOOT))) {
    //Serial.println("Got reboot command ...");
    reBoot();
  }

} // processCommand()

//------------------------------------------------------------------
//-- The master sends updated info that will be stored in the ------
//-- register(s)
//-- All Setters end up here ---------------------------------------
void receiveEvent(int numberOfBytesReceived) 
{
  //(void)numberOfBytesReceived;  // cast unused parameter to void to avoid compiler warning
  //Serial.println("receiveEvent() ..");

  wdt_reset();
  
  registerNumber = Wire.read(); // Get the memory map offset from the user

  if (registerNumber == _CMD_REGISTER) {   // eeprom command
    byte command = Wire.read(); // read the command
    processCommand(command);
    return;
  }

  //Begin recording the following incoming bytes to the temp memory map
  //starting at the registerNumber (the first byte received)
  for (byte x = 0 ; x < numberOfBytesReceived - 1 ; x++) {
    byte temp = Wire.read();
    if ( (x + registerNumber) < sizeof(registerLayout)) {
      //Store the result into the register map
      if ((registerNumber + x) >= 0 && (registerNumber + x) <= 3) {
        //Serial.println("Trying to write to readonly register!");
        return;
      }
      registerPointer[registerNumber + x] = temp;
    }
  }

} //  receiveEvent()

//------------------------------------------------------------------
//-- The master aks's for the data from registerNumber onwards -----
//-- in the register(s) --------------------------------------------
//-- All getters get there data from here --------------------------
void requestEvent()
{
  wdt_reset();
  //Serial.print("requestEvent() ..");

  //----- return max. 4 bytes to master, starting at registerNumber -------
  for (uint8_t x = 0; ( (x < 4) && (x + registerNumber) < (sizeof(registerLayout) - 1) ); x++) {
    if ((x + registerNumber) == 0x04) {
      //Serial.print(registerPointer[0x04]);
      //Serial.print(" ");
    }
    Wire.write(registerPointer[(x + registerNumber)]);
  }
  //Serial.println();
  //Serial.print(">> status       ["); Serial.print(registerPointer[0]);       Serial.println("]");
  //Serial.print(">> whoAmI       ["); Serial.print(registerPointer[1],HEX);   Serial.println("]");
  //Serial.print(">> majorRelease ["); Serial.print(registerPointer[2]); Serial.println("]");
  //Serial.print(">> minorRelease ["); Serial.print(registerPointer[3]); Serial.println("]");


} // requestEvent()


/***************************************************************************
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to permit
* persons to whom the Software is furnished to do so, subject to the
* following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
* OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
* THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
***************************************************************************/
