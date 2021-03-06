/*
  RTCTimedEvent.cpp - Event-Based Library for Arduino.
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

#include "RTCTimedEvent.h"

RTCTimedEventClass::RTCTimedEventClass() {
	this->count = 0;
	this->mallocSize = 0;
	this->nextMillis = -1;
	this->initialCapacity = sizeof(RTCTimerInformation);
	Wire.begin();
}

void RTCTimedEventClass::readRTC() {
	//register
	Wire.beginTransmission(RTC_ADDRESS_DS1307);
	Wire.send(0x00);
	Wire.endTransmission();

	//request data
	Wire.requestFrom(RTC_ADDRESS_DS1307, 7);

	//read RTC data
	this->time.second = bcdToDec(Wire.receive() & 0x7f);
	this->time.minute = bcdToDec(Wire.receive());
	this->time.hour = bcdToDec(Wire.receive() & 0x3f);
	this->time.dayOfWeek  = bcdToDec(Wire.receive());
	this->time.day = bcdToDec(Wire.receive());
	this->time.month = bcdToDec(Wire.receive());
	this->time.year = bcdToDec(Wire.receive())+RTC_BASEYR_DS1307;
}

void RTCTimedEventClass::writeRTC() {
	//stop RTC
	this->switchRTC(false);

	//register
	Wire.beginTransmission(RTC_ADDRESS_DS1307);
	Wire.send(0x00);
	
	//set RTC data
	Wire.send(decToBcd(this->time.second));
	Wire.send(decToBcd(this->time.minute));
	Wire.send(decToBcd(this->time.hour));
	Wire.send(decToBcd(this->time.dayOfWeek));
	Wire.send(decToBcd(this->time.day));
	Wire.send(decToBcd(this->time.month));
	Wire.send(decToBcd(this->time.year-RTC_BASEYR_DS1307));
	
	Wire.endTransmission();
	
	//start RTC
	this->switchRTC(true);
}

void RTCTimedEventClass::switchRTC(bool turnOn) {
	//register
	Wire.beginTransmission(RTC_ADDRESS_DS1307);
	Wire.send(0x00);
	
	//set RTC data
	if (turnOn) {
		Wire.send(decToBcd(this->time.second));
	} else {
		Wire.send(decToBcd(this->time.second) | RTC_HALT_DS1307);
	}
	Wire.send(decToBcd(this->time.minute));
	Wire.send(decToBcd(this->time.hour));
	Wire.send(decToBcd(this->time.dayOfWeek));
	Wire.send(decToBcd(this->time.day));
	Wire.send(decToBcd(this->time.month));
	Wire.send(decToBcd(this->time.year-RTC_BASEYR_DS1307));
	
	Wire.endTransmission();
}

void RTCTimedEventClass::addTimer(byte minute, byte hour, byte day, byte month, byte dayOfWeek, void (*onEvent)(RTCTimerInformation* Sender)) {
	this->addTimer(DEFAULT_TIMER_ID, minute, hour, day, month, dayOfWeek, onEvent);
}

void RTCTimedEventClass::addTimer(short eventId, byte minute, byte hour, byte day, byte month, byte dayOfWeek, void (*onEvent)(RTCTimerInformation* Sender)) {
	if (this->count > 0) {
		//determine if the buffer free space fits the next object
		if (this->mallocSize < (sizeof(RTCTimerInformation)*(this->count+1))) {
			this->mallocSize = sizeof(RTCTimerInformation)*(this->count+1);
			//alocate more memory space
			this->timers = (RTCTimerInformation*) realloc(this->timers, this->mallocSize);
		}
	} else {
		//determine if initial capacity parameter fits the first object
		if (this->initialCapacity >= sizeof(RTCTimerInformation)) {
			this->mallocSize = this->initialCapacity;
		} else {
			this->mallocSize = sizeof(RTCTimerInformation);
		}
		//create the buffer size
		this->timers = (RTCTimerInformation*) malloc(this->mallocSize);
	}
	
	this->currentTimer = this->timers+this->count; //array index
	this->currentTimer->eventId = eventId;
	this->currentTimer->minute = minute;
	this->currentTimer->hour = hour;
	this->currentTimer->dayOfWeek = dayOfWeek;
	this->currentTimer->day = day;
	this->currentTimer->month = month;
	this->currentTimer->onEvent = onEvent;

	this->count++;
}

void RTCTimedEventClass::clear() {
	if (this->mallocSize > 0)
		free(this->timers);

	this->count = 0;
	this->mallocSize = 0;
}

void RTCTimedEventClass::loop() {
	this->currMillis = millis();
	
	if (this->nextMillis == -1 || this->currMillis < this->lastMillis) { //prepare first step and handle millis() overflow
		//prepare next step
		this->readRTC();                         //read current time
		this->nextMillis = 60-this->time.second; //seconds to next minute
		this->nextMillis *= 1000;                //convert to milliseconds
		this->nextMillis += this->currMillis;    //add current millis
		this->lastMinute = this->time.minute;    //copy minute to avoid repetitions
	} else if (this->currMillis>=this->nextMillis) { //execute
		this->readRTC();
		this->currMinute = this->time.minute;
		if ( this->lastMinute != this->currMinute ) {
			for (this->index = 0; this->index < this->count; this->index++) {
				this->currentTimer = this->timers+this->index;
				
				if (this->currentTimer->minute == this->currMinute || this->currentTimer->minute == TIMER_ANY) {
					if (this->currentTimer->hour == this->time.hour || this->currentTimer->hour == TIMER_ANY) {
						if (this->currentTimer->dayOfWeek == this->time.dayOfWeek || this->currentTimer->dayOfWeek == TIMER_ANY) {
							if (this->currentTimer->day == this->time.day || this->currentTimer->day == TIMER_ANY) {
								if (this->currentTimer->month == this->time.month || this->currentTimer->month == TIMER_ANY) {
									this->currentTimer->onEvent(this->currentTimer);
								}
							}
						}
					}
				}
			}
		}
		
		//prepare next step
		this->readRTC();                         //read current time
		this->nextMillis = 60-this->time.second; //seconds to next minute
		this->nextMillis *= 1000;                //convert to milliseconds
		this->nextMillis += this->currMillis;    //add current millis
		this->lastMinute = this->currMinute;     //copy minute to avoid repetitions
	}
	
	this->lastMillis = this->currMillis;
}

byte RTCTimedEventClass::decToBcd(byte value)
{
  return (value/10*16) + (value%10);
}

byte RTCTimedEventClass::bcdToDec(byte value)
{
  return (value/16*10) + (value%16);
}

RTCTimedEventClass RTCTimedEvent;