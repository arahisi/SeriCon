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
#include "SeriCon.h"
#include "CRC16_CCITT.h"

void SeriCon_init(SeriCon *sc,
                  size_t rxWorkSize, uint8_t *rxWorkBuffer, size_t txWorkSize, uint8_t *txWorkBuffer,
                  SeriConRxHandler *handler, const SeriConHAL *hal)
{
    sc->RxCRC = CRC16_CCITT.Initial;
    sc->RxSize = 0;
    sc->TxIndex = 0;
    sc->RxIndex = 0;
    sc->RxTime = 0;
    sc->RxTimeout = 0;
    sc->TxSize = 0;
    sc->RxState = SeriConRxState_Normal;
    sc->RxErrorCount = 0;
#if SERICON_MESSAGE_ID_SUPPORT
    sc->RxID = 0;
    sc->TxID = 0;
#endif
#if SERICON_MESSAGE_RESEND_SUPPORT
    sc->TxLastSize = 0;
#endif
    sc->RX_TIMEOUT = SERICON_DEFAULT_RX_TIMEOUT;
    sc->RxWorkSize = (int)rxWorkSize;
    sc->TxWorkSize = (int)txWorkSize;
    sc->RxWorkBuffer = rxWorkBuffer;
    sc->TxWorkBuffer = txWorkBuffer;
    sc->Handler = handler;
    sc->HAL = hal;
}

uint8_t *SeriCon_getTxBuffer(SeriCon *sc)
{
    uint8_t *ptr = (sc->TxSize) ? NULL : &sc->TxWorkBuffer[4];
    return ptr;
}

int SeriCon_send(SeriCon *sc, size_t txSize, const uint8_t *txBuffer)
{
    uint16_t txCRC;
    if (sc->TxSize || (txSize > 256) || ((int)(txSize + 5) > sc->TxWorkSize) || (0 == txSize))
    {
        return 0;
    }
#if SERICON_MESSAGE_ID_SUPPORT
    sc->TxID++;
#endif
    sc->TxWorkBuffer[0] = 0xFF;
    sc->TxWorkBuffer[1] = 0x55;
    sc->TxWorkBuffer[2] =
#if SERICON_MESSAGE_ID_SUPPORT
        sc->TxID;
#else
        0
#endif
    ;
    sc->TxWorkBuffer[3] = (uint8_t)(txSize - 1);
    if (&sc->TxWorkBuffer[4] != txBuffer)
    {
        memcpy(&sc->TxWorkBuffer[4], txBuffer, txSize);
    }
    txCRC = uint16_t_GetCRC(&CRC16_CCITT, sc->TxWorkBuffer, txSize + 4);
    sc->TxWorkBuffer[txSize + 4] = (uint8_t)(txCRC);
    sc->TxWorkBuffer[txSize + 5] = (uint8_t)(txCRC >> 8);
    sc->TxIndex = 0;
    sc->TxSize = (int)(txSize + 6);
#if SERICON_MESSAGE_RESEND_SUPPORT
    sc->TxLastSize = sc->TxSize;
#endif
    return (int)txSize;
}

uint16_t SeriCon_getRxErrorCount(SeriCon *sc)
{
    return sc->RxErrorCount;
}

uint16_t SeriCon_getRxTimeout(SeriCon *sc)
{
    return sc->RX_TIMEOUT;
}

void SeriCon_setRxTimeout(SeriCon *sc, uint16_t time)
{
    sc->RX_TIMEOUT = time;
}

void SeriCon_task(SeriCon *sc)
{
    while (sc->TxSize)
    {
        if (sc->HAL->Tx(sc->TxWorkBuffer[sc->TxIndex]))
        {
            break;
        }
        sc->TxSize--;
        sc->TxIndex++;
    }
    if ((SERICON_RX_STATE_BIT_DONE & sc->RxState) && 0 == sc->TxSize)
    {
        uint8_t *tx_msg = SeriCon_getTxBuffer(sc);
        int tx_sz = sc->Handler(sc->RxWorkBuffer[3] + 1, &sc->RxWorkBuffer[4], tx_msg);
        if (tx_sz > 0)
        {
            SeriCon_send(sc, tx_sz, tx_msg);
            sc->RxState = SeriConRxState_Normal;
            sc->RxIndex = 0;
            sc->RxSize = 0;
        }
    }
    int continue_rx = 1;
    while (continue_rx)
    {
        if (0 == sc->RxIndex || sc->RxTimeout)
        {
            int c = EOF;
            uint16_t now = sc->HAL->Timer();
            if (SERICON_RX_STATE_BIT_JOB & sc->RxState)
            {
                c = sc->HAL->Rx();
            }
            if (EOF != c)
            {
                sc->RxTimeout = sc->RX_TIMEOUT;
                if (sc->RxIndex >= sc->RxWorkSize)
                {
                    sc->RxState = SeriConRxState_Error;
                }
                else if (SERICON_RX_STATE_BIT_JOB & sc->RxState)
                {
                    if (0 == sc->RxIndex)
                    {
                        sc->RxCRC = CRC16_CCITT.Initial;
                    }
                    sc->RxWorkBuffer[sc->RxIndex] = (uint8_t)c;
                    sc->RxCRC = uint16_t_CRC(&CRC16_CCITT, sc->RxWorkBuffer[sc->RxIndex], sc->RxCRC);
                    if (0 == sc->RxIndex)
                    { // head0
#if SERICON_MESSAGE_RESEND_SUPPORT
                        if (0x00 == c)
                        {
                            sc->RxState |= SERICON_RX_STATE_BIT_RESEBD;
                        }
                        else
#endif
                            if (0xFF != c)
                        { // head0 error
                            sc->RxState = SeriConRxState_Error;
                        }
                    }
                    else if (1 == sc->RxIndex)
                    { // head1
#if SERICON_MESSAGE_RESEND_SUPPORT
                        if (SERICON_RX_STATE_BIT_RESEBD & sc->RxState)
                        {
                            if (0xAA == c)
                            {
                                sc->RxState |= (SERICON_RX_STATE_BIT_RESEBD << 1);
                            }
                            else
                            {
                                sc->RxState = SeriConRxState_Error;
                            }
                        }
                        else
#endif
                            if (0x55 != c)
                        { // head1 error
                            sc->RxState = SeriConRxState_Error;
                        }
                    }
                    else if (2 == sc->RxIndex)
                    { // ID
#if SERICON_MESSAGE_RESEND_SUPPORT
                        if (SERICON_RX_STATE_BIT_RESEBD & sc->RxState)
                        {
                            sc->RxState = SeriConRxState_Error;
                        }
#endif
                    }
                    else if (3 == sc->RxIndex)
                    { // size
                        sc->RxSize = (int)c;
                        sc->RxSize += 7;
                        if (sc->RxSize > sc->RxWorkSize)
                        { // size error
                            sc->RxState = SeriConRxState_Error;
                        }
                    }
                    else if (4 <= sc->RxIndex && sc->RxIndex <= (sc->RxSize - 2))
                    { // data & CRC16(LOW)
                    }
                    else if (sc->RxIndex == (sc->RxSize - 1))
                    { // CRC16(HIGH)
                        if (sc->RxCRC)
                        { // CRC error
                            sc->RxState = SeriConRxState_Error;
                        }
                        else
                        {
#if SERICON_MESSAGE_ID_SUPPORT
                            if (sc->RxID == sc->RxWorkBuffer[2])
                            { // ID error
                                sc->RxState = SeriConRxState_Error;
                            }
                            else
#endif
                            {
                                sc->RxState = SeriConRxState_Done;
                            }
#if SERICON_MESSAGE_ID_SUPPORT
                            sc->RxID = sc->RxWorkBuffer[2];
#endif
                        }
                    }
                }
                sc->RxIndex++;
            }
            else
            {
                uint16_t diff;
                if (now >= sc->RxTime)
                {
                    diff = now - sc->RxTime;
                }
                else
                {
                    diff = 0xFFFFU - sc->RxTime;
                    diff += now + 1;
                }
                if (diff >= sc->RxTimeout)
                {
                    if (sc->RxSize && sc->RxIndex != sc->RxSize)
                    { // timeout
                        sc->RxState = SeriConRxState_Error;
                    }
#if SERICON_MESSAGE_RESEND_SUPPORT
                    if (SeriConRxState_Resend2 == sc->RxState && sc->TxLastSize) {
                        sc->TxWorkBuffer[0] = 0xFF;
                        sc->TxWorkBuffer[1] = 0x55;
                        sc->TxIndex = 0;
                        sc->TxSize = sc->TxLastSize;
                    } else
#endif
                    if (SERICON_RX_STATE_BIT_ERROR & sc->RxState)
                    {
#if SERICON_MESSAGE_RESEND_SUPPORT
                        sc->TxWorkBuffer[0] = 0x00;
                        sc->TxWorkBuffer[1] = 0xAA;
                        sc->TxIndex = 0;
                        sc->TxSize = 2;
#endif
                        sc->RxErrorCount++;
                    }
                    sc->RxState = SeriConRxState_Normal;
                    sc->RxTimeout = 0;
                    sc->RxIndex = 0;
                    sc->RxSize = 0;
                }
                else
                {
                    sc->RxTimeout -= diff;
                }
                continue_rx = 0;
            }
            sc->RxTime = now;
        }
    }
}
