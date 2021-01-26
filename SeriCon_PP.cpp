/*
Copyright 2021, Shoji Yamamoto
Permission is hereby granted, free of charge, to any person obtaining a copy 
of this software and associated documentation files (the "Software"), to deal 
in the Software without restriction, including without limitation the rights 
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
of the Software, and to permit persons to whom the Software is furnished to do 
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
DEALINGS IN THE SOFTWARE.
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SeriCon_PP.h"

SeriConPP::SeriConPP(size_t rxWorkSize, uint8_t *rxWorkBuffer, size_t txWorkSize, uint8_t *txWorkBuffer, SeriConRxHandler *handler, const SeriConHAL *hal)
{
	SeriCon_init(&this->sc, rxWorkSize, rxWorkBuffer, txWorkSize, txWorkBuffer, handler, hal);
}

uint8_t *SeriConPP::getTxBuffer(void)
{
	return SeriCon_getTxBuffer(&this->sc);
}

int SeriConPP::send(size_t txSize, const uint8_t *txBuffer)
{
	return SeriCon_send(&this->sc, txSize, txBuffer);
}

uint32_t SeriConPP::getRxErrorCount(void)
{
	return SeriCon_getRxErrorCount(&this->sc);
}

uint32_t SeriConPP::getRxTimeout(void)
{
	return SeriCon_getRxTimeout(&this->sc);
}

void SeriConPP::setRxTimeout(uint16_t time)
{
	SeriCon_setRxTimeout(&this->sc, time);
}

void SeriConPP::task(void)
{
	SeriCon_task(&this->sc);
}
