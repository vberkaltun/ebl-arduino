/*
  RTCTimedEvent.h - Event-Based Library for Arduino.
  Copyright (c) 2011, Renato A. Ferreira
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
      * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      * Neither the name of the <organization> nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef TimedEvent_h
#define TimedEvent_h

#include <stdlib.h>
#include "WProgram.h"
#include <Wire.h>

#define RTC_ADDRESS_DS1307 0x68
#define RTC_HALT_DS1307 0x80
#define RTC_BASEYR_DS1307 2000
#define TIMER_ANY 0xFF
#define DEFAULT_TIMER_ID -99

struct TimeInformation {
	byte second;
	byte minute;
	byte hour;
	byte dayOfWeek;
	byte day;
	byte month;
	short year;
};

struct RTCTimerInformation {
	short eventId;
	byte minute;
	byte hour;
	byte dayOfWeek;
	byte day;
	byte month;
	void (*onEvent)(RTCTimerInformation* Sender);
};

class RTCTimedEventClass
{
  public:
    RTCTimedEventClass();
	short initialCapacity;
	TimeInformation time;
	void readRTC();
	void writeRTC();
	void addTimer(byte minute, byte hour, byte day, byte month, byte dayOfWeek, void (*onEvent)(RTCTimerInformation* Sender));
	void addTimer(short eventId, byte minute, byte hour, byte day, byte month, byte dayOfWeek, void (*onEvent)(RTCTimerInformation* Sender));
	void clear();
	void loop();
	
  private:
	short count;
	short mallocSize;
	short index;
	short lastMinute;
	short currMinute;
	unsigned long nextMillis;
	unsigned long lastMillis;
	unsigned long currMillis;
    RTCTimerInformation* timers;
	RTCTimerInformation* currentTimer;
	byte decToBcd(byte value);
	byte bcdToDec(byte value);
	void switchRTC(bool turnOn);
};

//global instance
extern RTCTimedEventClass RTCTimedEvent;

#endif