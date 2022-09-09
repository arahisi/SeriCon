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

void SeriCon_init(SeriCon *sc,
                  size_t rxWorkSize, uint8_t *rxWorkBuffer, size_t txWorkSize, uint8_t *txWorkBuffer,
                  SeriConRxHandler *handler, const SeriConHAL *hal)
{
    sc->RxCRC = 0xFFFF;
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

static uint16_t SeriCon_getCRC16(uint8_t data, uint16_t crc16)
{
    // CRC16 CCITT
    static const uint16_t table[256] = {0x0000, 0x1189, 0x2312, 0x329B, 0x4624, 0x57AD, 0x6536, 0x74BF, 0x8C48, 0x9DC1, 0xAF5A, 0xBED3, 0xCA6C, 0xDBE5, 0xE97E, 0xF8F7, 0x1081, 0x0108, 0x3393, 0x221A,
                                        0x56A5, 0x472C, 0x75B7, 0x643E, 0x9CC9, 0x8D40, 0xBFDB, 0xAE52, 0xDAED, 0xCB64, 0xF9FF, 0xE876, 0x2102, 0x308B, 0x0210, 0x1399, 0x6726, 0x76AF, 0x4434, 0x55BD, 0xAD4A, 0xBCC3, 0x8E58,
                                        0x9FD1, 0xEB6E, 0xFAE7, 0xC87C, 0xD9F5, 0x3183, 0x200A, 0x1291, 0x0318, 0x77A7, 0x662E, 0x54B5, 0x453C, 0xBDCB, 0xAC42, 0x9ED9, 0x8F50, 0xFBEF, 0xEA66, 0xD8FD, 0xC974, 0x4204, 0x538D,
                                        0x6116, 0x709F, 0x0420, 0x15A9, 0x2732, 0x36BB, 0xCE4C, 0xDFC5, 0xED5E, 0xFCD7, 0x8868, 0x99E1, 0xAB7A, 0xBAF3, 0x5285, 0x430C, 0x7197, 0x601E, 0x14A1, 0x0528, 0x37B3, 0x263A, 0xDECD,
                                        0xCF44, 0xFDDF, 0xEC56, 0x98E9, 0x8960, 0xBBFB, 0xAA72, 0x6306, 0x728F, 0x4014, 0x519D, 0x2522, 0x34AB, 0x0630, 0x17B9, 0xEF4E, 0xFEC7, 0xCC5C, 0xDDD5, 0xA96A, 0xB8E3, 0x8A78, 0x9BF1,
                                        0x7387, 0x620E, 0x5095, 0x411C, 0x35A3, 0x242A, 0x16B1, 0x0738, 0xFFCF, 0xEE46, 0xDCDD, 0xCD54, 0xB9EB, 0xA862, 0x9AF9, 0x8B70, 0x8408, 0x9581, 0xA71A, 0xB693, 0xC22C, 0xD3A5, 0xE13E,
                                        0xF0B7, 0x0840, 0x19C9, 0x2B52, 0x3ADB, 0x4E64, 0x5FED, 0x6D76, 0x7CFF, 0x9489, 0x8500, 0xB79B, 0xA612, 0xD2AD, 0xC324, 0xF1BF, 0xE036, 0x18C1, 0x0948, 0x3BD3, 0x2A5A, 0x5EE5, 0x4F6C,
                                        0x7DF7, 0x6C7E, 0xA50A, 0xB483, 0x8618, 0x9791, 0xE32E, 0xF2A7, 0xC03C, 0xD1B5, 0x2942, 0x38CB, 0x0A50, 0x1BD9, 0x6F66, 0x7EEF, 0x4C74, 0x5DFD, 0xB58B, 0xA402, 0x9699, 0x8710, 0xF3AF,
                                        0xE226, 0xD0BD, 0xC134, 0x39C3, 0x284A, 0x1AD1, 0x0B58, 0x7FE7, 0x6E6E, 0x5CF5, 0x4D7C, 0xC60C, 0xD785, 0xE51E, 0xF497, 0x8028, 0x91A1, 0xA33A, 0xB2B3, 0x4A44, 0x5BCD, 0x6956, 0x78DF,
                                        0x0C60, 0x1DE9, 0x2F72, 0x3EFB, 0xD68D, 0xC704, 0xF59F, 0xE416, 0x90A9, 0x8120, 0xB3BB, 0xA232, 0x5AC5, 0x4B4C, 0x79D7, 0x685E, 0x1CE1, 0x0D68, 0x3FF3, 0x2E7A, 0xE70E, 0xF687, 0xC41C,
                                        0xD595, 0xA12A, 0xB0A3, 0x8238, 0x93B1, 0x6B46, 0x7ACF, 0x4854, 0x59DD, 0x2D62, 0x3CEB, 0x0E70, 0x1FF9, 0xF78F, 0xE606, 0xD49D, 0xC514, 0xB1AB, 0xA022, 0x92B9, 0x8330, 0x7BC7, 0x6A4E,
                                        0x58D5, 0x495C, 0x3DE3, 0x2C6A, 0x1EF1, 0x0F78};
    crc16 = table[(uint8_t)(crc16 & 0x00FF) ^ data] ^ (crc16 >> 8);
    return crc16;
}

int SeriCon_send(SeriCon *sc, size_t txSize, const uint8_t *txBuffer)
{
    uint16_t txCRC = 0xFFFF;
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
    int i;
    for (i = 0; i < (int)(txSize + 4); i++)
    {
        txCRC = SeriCon_getCRC16(sc->TxWorkBuffer[i], txCRC);
    }
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
                        sc->RxCRC = 0xFFFF;
                    }
                    sc->RxWorkBuffer[sc->RxIndex] = (uint8_t)c;
                    sc->RxCRC = SeriCon_getCRC16(sc->RxWorkBuffer[sc->RxIndex], sc->RxCRC);
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
                    if (SeriConRxState_Resend2 == sc->RxState && sc->TxLastSize)
                    {
                        sc->TxWorkBuffer[0] = 0xFF;
                        sc->TxWorkBuffer[1] = 0x55;
                        sc->TxIndex = 0;
                        sc->TxSize = sc->TxLastSize;
                    }
                    else
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
