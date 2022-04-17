/*
 *  © 2020, Chris Harlow. All rights reserved.
 *  
 *  This file is part of Asbelos DCC API
 *
 *  This is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  It is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with CommandStation.  If not, see <https://www.gnu.org/licenses/>.
 */
/**********************************************************************

DCC++ BASE STATION supports Sensor inputs that can be connected to any Arduino Pin
not in use by this program.  Sensors can be of any type (infrared, magentic, mechanical...).
The only requirement is that when "activated" the Sensor must force the specified Arduino
Pin LOW (i.e. to ground), and when not activated, this Pin should remain HIGH (e.g. 5V),
or be allowed to float HIGH if use of the Arduino Pin's internal pull-up resistor is specified.

To ensure proper voltage levels, some part of the Sensor circuitry
MUST be tied back to the same ground as used by the Arduino.

The Sensor code below utilizes exponential smoothing to "de-bounce" spikes generated by
mechanical switches and transistors.  This avoids the need to create smoothing circuitry
for each sensor.  You may need to change these parameters through trial and error for your specific sensors.

To have this sketch monitor one or more Arduino pins for sensor triggers, first define/edit/delete
sensor definitions using the following variation of the "S" command:

  <S ID PIN PULLUP>:           creates a new sensor ID, with specified PIN and PULLUP
                               if sensor ID already exists, it is updated with specificed PIN and PULLUP
                               returns: <O> if successful and <X> if unsuccessful (e.g. out of memory)

  <S ID>:                      deletes definition of sensor ID
                               returns: <O> if successful and <X> if unsuccessful (e.g. ID does not exist)

  <S>:                         lists all defined sensors
                               returns: <Q ID PIN PULLUP> for each defined sensor or <X> if no sensors defined

where

  ID: the numeric ID (0-32767) of the sensor
  PIN: the arduino pin number the sensor is connected to
  PULLUP: 1=use internal pull-up resistor for PIN, 0=don't use internal pull-up resistor for PIN

Once all sensors have been properly defined, use the <E> command to store their definitions to EEPROM.
If you later make edits/additions/deletions to the sensor definitions, you must invoke the <E> command if you want those
new definitions updated in the EEPROM.  You can also clear everything stored in the EEPROM by invoking the <e> command.

All sensors defined as per above are repeatedly and sequentially checked within the main loop of this sketch.
If a Sensor Pin is found to have transitioned from one state to another, one of the following serial messages are generated:

  <Q ID>     - for transition of Sensor ID from HIGH state to LOW state (i.e. the sensor is triggered)
  <q ID>     - for transition of Sensor ID from LOW state to HIGH state (i.e. the sensor is no longer triggered)

Depending on whether the physical sensor is acting as an "event-trigger" or a "detection-sensor," you may
decide to ignore the <q ID> return and only react to <Q ID> triggers.

**********************************************************************/

#include "StringFormatter.h"
#include "Sensors.h"
#include "EEStore.h"


void Sensor::begin()
{
  
}

///////////////////////////////////////////////////////////////////////////////
//
// checks one defined sensors and prints _changed_ sensor state
// to stream unless stream is NULL in which case only internal
// state is updated. Then advances to next sensor which will
// be checked att next invocation.
//
///////////////////////////////////////////////////////////////////////////////
uint16_t ctr = 0;

/*
 *  uint32_t outStatus;
    uint32_t inpStatusABCD;
    uint32_t verifyStatus;
    uint8_t currentSensor = 0;
    byte latchdelay;

 */
void Sensor::setEnable(bool newStatus)
{
  isActive = newStatus;
}

void Sensor::checkAll(Print *stream)
{
//  if (latchdelay >0)
//  {
//    latchdelay--;
//    return;
//  }
  if (!isActive)
    return;
  uint16_t portMask = PORTB & 0x0F;
  ctr++;
  if ((portMask == 0))// && (ctr >= 1000))
  {
    ctr = 0;
    uint32_t compRes = ~(inpStatusABCD ^ verifyStatus); //flag unchanged bits
    uint32_t compSent = (verifyStatus ^ outStatus) & compRes; //flag everything unchanged but not yet sent
    uint32_t verifyMask = 0x00000001;
    for (uint8_t i = 0; i < 32; i++)
    {
      if ((compSent & verifyMask) > 0)
      {
        if (stream != NULL) 
          StringFormatter::send(stream, F("<%c %d>\n"), (verifyStatus & verifyMask) > 0 ? 'Q' : 'q', i);
      }
      verifyMask = verifyMask << 1;
    }
    outStatus = verifyStatus;
    verifyStatus = inpStatusABCD; //roll up new data to verifyStatusfor next loop
  }
  uint16_t bitMask = 0x0001 << portMask;
  uint16_t* inpArray = (uint16_t*)&inpStatusABCD;
  if (analogRead(PortBank1) > 200) //Banks A,B
  {
    inpArray[0] |= bitMask;
  }
  else
  {
    inpArray[0] &= ~bitMask;
  }

  if (analogRead(PortBank2) > 200) //Banks C,D
    inpArray[1] |= bitMask;
  else
    inpArray[1] &= ~bitMask;
  portMask = (portMask + 1) & 0x0F;
  PORTB = (PORTB & 0xF0) | portMask;
//  latchdelay = 0;
} // Sensor::checkAll

///////////////////////////////////////////////////////////////////////////////
//
// prints all sensor states to stream
//
///////////////////////////////////////////////////////////////////////////////

void Sensor::printAll(Print *stream)
{
  uint32_t verifyMask = 0x00000001;
  for (uint8_t i = 0; i < 32; i++)
  {
    if (stream != NULL) 
      StringFormatter::send(stream, F("<%c %d>\n"), (inpStatusABCD & verifyMask) > 0 ? 'Q' : 'q', i);
  }
  verifyMask = verifyMask << 1;
} // Sensor::printAll

///////////////////////////////////////////////////////////////////////////////