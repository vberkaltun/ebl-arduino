/*
  LedControl.cpp - Event-Based Library for Arduino.
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

#include "LedControl.h"

LedControlClass::LedControlClass() {
	this->count = 0;
}

void LedControlClass::addLed(short pin) {
	if (this->count > 0) {
		this->leds = (LedInformation*) realloc(this->leds, sizeof(LedInformation)*(this->count+1));
	} else {
		this->leds = (LedInformation*) malloc(sizeof(LedInformation));
	}

	this->setPosition(this->count);
	this->currentLed->pin = pin;
	this->currentLed->lastEventMicroseconds = 0;
	
	pinMode(this->currentLed->pin, OUTPUT);
	
	this->count++;
}

void LedControlClass::setPosition(short position) {
	this->currentLed = this->leds+position;
}

void LedControlClass::findPin(short pin) {
	for (this->index = 0; this->index < this->count; this->index++) {
		this->setPosition(this->index);
		
		if (this->currentLed->pin == pin)
			return;
	}
	this->addLed(pin);
}

void LedControlClass::loop() {
	for (this->index = 0; this->index < this->count; this->index++) {
		this->setPosition(this->index);
		
		if (this->currentLed->mode != MODE_NONE) {
			this->lastMicros = micros();
		
			if (this->currentLed->lastEventMicroseconds+(this->currentLed->intervalMilliseconds*1000L) <= this->lastMicros) {

				if (this->currentLed->mode == MODE_BLINK_OFF) {
					this->turnOn(this->currentLed->pin);
					this->currentLed->mode = MODE_BLINK_ON;
					this->currentLed->lastEventMicroseconds = this->lastMicros;
				} else if (this->currentLed->mode == MODE_BLINK_ON) {
					this->turnOff(this->currentLed->pin);
					this->currentLed->mode = MODE_BLINK_OFF;
					this->currentLed->lastEventMicroseconds = this->lastMicros;
				} else if (this->currentLed->mode == MODE_FADE_IN) {
					if (this->currentLed->pwmLevel <= 255) {
						this->currentLed->lastEventMicroseconds += this->currentLed->syncDelay;
						analogWrite(this->currentLed->pin, this->currentLed->pwmLevel);
						this->currentLed->pwmLevel++;
					} else {
						this->currentLed->mode = MODE_FADE_OUT;
						this->currentLed->pwmLevel = 255;
						this->currentLed->lastEventMicroseconds = this->lastMicros;
					}
				} else if (this->currentLed->mode == MODE_FADE_OUT) {
					if (this->currentLed->pwmLevel >= 0) {
						this->currentLed->lastEventMicroseconds += this->currentLed->syncDelay;
						analogWrite(this->currentLed->pin, this->currentLed->pwmLevel);
						this->currentLed->pwmLevel--;
					} else {
						this->currentLed->mode = MODE_FADE_IN;
						this->currentLed->pwmLevel = 0;
						this->currentLed->lastEventMicroseconds = this->lastMicros;
					}
				}
			} else if (this->lastMicros < this->currentLed->lastEventMicroseconds) {
				//avoid micros() overflow problem
				this->currentLed->lastEventMicroseconds = this->lastMicros;
			}
		}
	}
}

void LedControlClass::turnOn(short pin) {
	digitalWrite(pin, HIGH);
}

void LedControlClass::turnOff(short pin) {
	digitalWrite(pin, LOW);
}

void LedControlClass::turnPercent(short pin, short percent) {
	if (percent <= 0)
		digitalWrite(pin, LOW);
	else if (percent >= 100)
		digitalWrite(pin, HIGH);
	else {
		analogWrite(pin, map(percent, 0, 100, 0, 255));
	}
}

void LedControlClass::turnOn(short pin, int delayMilliseconds) {
	if (delayMilliseconds>4000) {
		this->syncMicros = false;
		this->syncDelay = delayMilliseconds/255;
	} else {
		this->syncMicros = true;
		this->syncDelay = (delayMilliseconds*1000L)/255;
	}
	
	for (this->index = 0; this->index <= 255; this->index++) {
		analogWrite(pin, this->index);
		
		if (this->syncMicros)
			delayMicroseconds(this->syncDelay);
		else 
			delay(this->syncDelay);
	}
}

void LedControlClass::turnOff(short pin, int delayMilliseconds) {
	if (delayMilliseconds>4000) {
		this->syncMicros = false;
		this->syncDelay = delayMilliseconds/255;
	} else {
		this->syncMicros = true;
		this->syncDelay = (delayMilliseconds*1000L)/255;
	}
	
	for (this->index = 255; this->index >= 0; this->index--) {
		analogWrite(pin, this->index);
		
		if (this->syncMicros)
			delayMicroseconds(this->syncDelay);
		else 
			delay(this->syncDelay);
	}
}

void LedControlClass::blink(short pin, int times, int intervalMilliseconds) {
	for (this->index = 0; this->index < times; this->index++) {
		if (this->index != 0)
			delay(intervalMilliseconds);
			
		this->turnOn(pin);
		delay(intervalMilliseconds);
		this->turnOff(pin);
	}
}

void LedControlClass::blink(short pin, int times, int intervalMilliseconds, int delayMilliseconds) {
	for (this->index2 = 0; this->index2 < times; this->index2++) {
		if (this->index2 != 0)
			delay(intervalMilliseconds);
			
		this->turnOn(pin, delayMilliseconds);
		delay(intervalMilliseconds);
		this->turnOff(pin, delayMilliseconds);
	}
}

void LedControlClass::startBlink(short pin, int intervalMilliseconds) {
	this->findPin(pin);
	this->turnOn(this->currentLed->pin);
	this->currentLed->mode = MODE_BLINK_ON;
	this->currentLed->intervalMilliseconds = intervalMilliseconds;
	
	//starts immediately
	this->currentLed->lastEventMicroseconds = micros()-(this->currentLed->intervalMilliseconds*1000L);
}

void LedControlClass::startBlink(short pin, int intervalMilliseconds, int delayMilliseconds) {
	this->findPin(pin);
	this->turnOff(this->currentLed->pin);
	this->currentLed->mode = MODE_FADE_IN;
	this->currentLed->intervalMilliseconds = intervalMilliseconds;
	this->currentLed->delayMilliseconds = delayMilliseconds;
	this->currentLed->syncDelay = (delayMilliseconds*1000L)/255;
	this->currentLed->pwmLevel = 0;
	
	//starts immediately
	this->currentLed->lastEventMicroseconds = micros()-(this->currentLed->intervalMilliseconds*1000L);
}

void LedControlClass::stopBlink(short pin) {
	this->findPin(pin);
	this->turnOff(this->currentLed->pin);
	this->currentLed->mode = MODE_NONE;
}

LedControlClass LedControl;