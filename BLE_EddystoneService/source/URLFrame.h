/* mbed Microcontroller Library
 * Copyright (c) 2006-2015 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __URLFRAME_H__
#define __URLFRAME_H__

#include "EddystoneTypes.h"
#include <string.h>

class URLFrame
{
public:
    URLFrame(void);

    URLFrame(const char *urlDataIn);

    URLFrame(UrlData_t urlDataIn, uint8_t urlDataLength);

    void constructURLFrame(uint8_t* rawFrame, int8_t advPowerLevel);

    size_t getRawFrameSize(void) const;

    uint8_t* getEncodedURLData(void);

    uint8_t getEncodedURLDataLength(void) const;

    void setURLData(const char *urlDataIn);

    void setEncodedURLData(const uint8_t* urlEncodedDataIn, const uint8_t urlEncodedDataLengthIn);

private:
    void encodeURL(const char *urlDataIn);

    static const uint8_t FRAME_TYPE_URL     = 0x10;
    /* Even if the URL is 0 bytes we still need to include the type and txPower i.e. 2 bytes */
    static const uint8_t FRAME_MIN_SIZE_URL = 2;

    uint8_t              urlDataLength;
    UrlData_t            urlData;

};

#endif /* __URLFRAME_H__ */
