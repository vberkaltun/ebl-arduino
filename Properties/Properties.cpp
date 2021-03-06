/*
  Properties.cpp - Event-Based Library for Arduino.
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

#include "Properties.h"

PropertiesClass::PropertiesClass() {
	this->mainPosition = DEFAULT_POSITION;
	this->count = 0;
	this->size = 0;
	this->mallocSize = 0;
}

bool PropertiesClass::load() {
	//free any previous loaded data
	this->flush();

	//load variable count
	this->readEEPROM(this->mainPosition, (byte*)&this->count, sizeof(this->count));
	
	//validate variable count
	if ( this->count > 0 && this->count < MAX_VARIABLES ) {
		//load data size
		this->readEEPROM(this->mainPosition+sizeof(this->count), (byte*)&this->size, sizeof(this->size));
		
		//copy data
		if (this->size > 0) {
			this->properties = (PropertyInformation*) malloc(this->size);
			this->readEEPROM(this->mainPosition+sizeof(this->count)+sizeof(this->size), (byte*)this->properties, this->size);
			this->fixPointers();
		}
		
		return true;
	} else {
		this->count = 0;
		return false;
	}
}

bool PropertiesClass::load(short position) {
	this->mainPosition = position;
	return this->load();
}

void PropertiesClass::save() {
	this->writeEEPROM(this->mainPosition, (byte*)&this->count, sizeof(this->count));
	this->writeEEPROM(this->mainPosition+sizeof(this->count), (byte*)&this->size, this->mainPosition+sizeof(this->size));
	this->writeEEPROM(this->mainPosition+sizeof(this->count)+sizeof(this->size), (byte*)this->properties, this->size);
}

void PropertiesClass::save(short position) {
	this->mainPosition = position;
	this->save();
}

void PropertiesClass::readEEPROM(short position, byte* data, short size) {
	for (this->index = 0; this->index < size; this->index++) {
		*data++ = eeprom_read_byte((unsigned char *) position++);
	}
}

void PropertiesClass::writeEEPROM(short position, byte* data, short size) {
	for (this->index = 0; this->index < size; this->index++) {
		eeprom_write_byte((unsigned char *) position++, *data++);
	}
}

void PropertiesClass::addProperty(short propertyId, short dataSize) {
	if ( this->count < MAX_VARIABLES ) {
		if (this->count > 0) {
			this->size += sizeof(PropertyInformation)+dataSize;
			this->properties = (PropertyInformation*) realloc(this->properties, this->size);
			this->mallocSize = this->size;
		} else {
			this->size = sizeof(PropertyInformation)+dataSize;
			if ( this->mallocSize < this->size ) {
				this->properties = (PropertyInformation*) malloc(this->size);
				this->mallocSize = this->size;
			}
		}
	
		this->setPosition(this->count);
		this->currentProperty->propertyId = propertyId;
		this->currentProperty->valueSize = dataSize;
		this->currentProperty->value = (this->currentProperty)+1; //the value is at end of structure
		this->count++;
		this->fixPointers();
	}
}

void PropertiesClass::flush() {
	if (this->mallocSize > 0)
		free(this->properties);

	this->count = 0;
	this->size = 0;
	this->mallocSize = 0;
}

void PropertiesClass::setPosition(short position) {
	this->currentProperty = this->properties;
	if (position > 0)
		for (this->index = 1; this->index <= position; this->index++)
			this->currentProperty = (PropertyInformation*) (((unsigned short)this->currentProperty)+sizeof(PropertyInformation)+this->currentProperty->valueSize);
}

bool PropertiesClass::findProperty(short propertyId) {
	if (this->count > 0) {
		//try first one
		this->index = 0;
		this->currentProperty = this->properties;
		if (this->currentProperty->propertyId == propertyId)
			return true;

		//the remaining
		for (this->index = 1; this->index < this->count; this->index++) {
			this->currentProperty = (PropertyInformation*) (((unsigned short)this->currentProperty)+sizeof(PropertyInformation)+this->currentProperty->valueSize);
			if (this->currentProperty->propertyId == propertyId)
				return true;		
		}
	}
	
	//not found
	return false;
}

void PropertiesClass::remove(short propertyId) {
	if (this->findProperty(propertyId)) {
		this->size -= (this->currentProperty->valueSize+sizeof(PropertyInformation));

		if (this->count > 1) {
			//move data if needed
			if (this->index < this->count-1) {
				//copy destination location
				this->moveProperty = this->currentProperty;
				
				//calculate data size to move
				short moveSize=0;
				for (;this->index < (this->count-1); this->index++) {
					this->currentProperty = (PropertyInformation*) (((unsigned short)this->currentProperty)+sizeof(PropertyInformation)+this->currentProperty->valueSize);
					moveSize += sizeof(PropertyInformation)+this->currentProperty->valueSize;
				}
				
				memmove(this->moveProperty,(void*) ((unsigned short)this->moveProperty+sizeof(PropertyInformation)+this->moveProperty->valueSize),moveSize);
			}
		}
		
		this->count--;
		
		if (this->size <= 0)
			this->flush();

		this->fixPointers();
	}
}

void PropertiesClass::fixPointers() {
	for (this->index = 0; this->index < this->count; this->index++) {
		if (this->index == 0)
			this->currentProperty = this->properties;
		else
			this->currentProperty = (PropertyInformation*) (((unsigned short)this->currentProperty)+sizeof(PropertyInformation)+this->currentProperty->valueSize);
			
		this->currentProperty->value = (this->currentProperty)+1; //the value is at end of structure
	}
}

void PropertiesClass::set(short propertyId, int value) {
	this->set(propertyId,(void*)&value,sizeof(int));
}

void PropertiesClass::set(short propertyId, long value) {
	this->set(propertyId,(void*)&value,sizeof(long));
}

void PropertiesClass::set(short propertyId, void* data, short dataSize) {
	if (this->findProperty(propertyId)) {
		//verify if size was changed
		if (dataSize != this->currentProperty->valueSize) {
			this->remove(propertyId);
			this->addProperty(propertyId, dataSize);
		}
	} else {
		this->addProperty(propertyId, dataSize);
	}
	
	memcpy(this->currentProperty->value,data,dataSize);
}

int PropertiesClass::getInt(short propertyId) {
	if (this->findProperty(propertyId)) {
		memcpy(&this->lastInt,this->currentProperty->value,this->currentProperty->valueSize);
		return this->lastInt;
	} else {
		return -1;
	}
}

long PropertiesClass::getLong(short propertyId) {
	if (this->findProperty(propertyId)) {
		memcpy(&this->lastLong,this->currentProperty->value,this->currentProperty->valueSize);
		return this->lastLong;
	} else {
		return -1;
	}
}

PropertyInformation* PropertiesClass::get(short propertyId) {
	if (this->findProperty(propertyId)) {
		return this->currentProperty;
	} else {
		return NULL;
	}
}

PropertiesClass Properties;