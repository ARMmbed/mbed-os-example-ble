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

#include "URLFrame.h"

URLFrame::URLFrame(void)
{
    urlDataLength = 0;
    memset(urlData, 0, sizeof(UrlData_t));
}

URLFrame::URLFrame(const char *urlDataIn)
{
    encodeURL(urlDataIn);
}

URLFrame::URLFrame(UrlData_t urlDataIn, uint8_t urlDataLengthIn)
{
    urlDataLength = (urlDataLengthIn > URL_DATA_MAX) ? URL_DATA_MAX : urlDataLengthIn;
    memcpy(urlData, urlDataIn, urlDataLength);
}

void URLFrame::constructURLFrame(uint8_t* rawFrame, int8_t advPowerLevel)
{
    size_t index = 0;
    rawFrame[index++] = EDDYSTONE_UUID[0];            // 16-bit Eddystone UUID
    rawFrame[index++] = EDDYSTONE_UUID[1];
    rawFrame[index++] = FRAME_TYPE_URL;               // 1B  Type
    rawFrame[index++] = advPowerLevel;                // 1B  Power @ 0meter
    memcpy(rawFrame + index, urlData, urlDataLength); // Encoded URL
}

size_t URLFrame::getRawFrameSize(void) const
{
    return urlDataLength + FRAME_MIN_SIZE_URL + EDDYSTONE_UUID_SIZE;
}

uint8_t* URLFrame::getEncodedURLData(void)
{
    return urlData;
}

uint8_t URLFrame::getEncodedURLDataLength(void) const
{
    return urlDataLength;
}

void URLFrame::setURLData(const char *urlDataIn)
{
    encodeURL(urlDataIn);
}

void URLFrame::setEncodedURLData(const uint8_t* urlEncodedDataIn, const uint8_t urlEncodedDataLengthIn)
{
    urlDataLength = urlEncodedDataLengthIn;
    memcpy(urlData, urlEncodedDataIn, urlEncodedDataLengthIn);
}

void URLFrame::encodeURL(const char *urlDataIn)
{
    const char  *prefixes[] = {
        "http://www.",
        "https://www.",
        "http://",
        "https://",
    };
    const size_t NUM_PREFIXES = sizeof(prefixes) / sizeof(char *);
    const char  *suffixes[]   = {
        ".com/",
        ".org/",
        ".edu/",
        ".net/",
        ".info/",
        ".biz/",
        ".gov/",
        ".com",
        ".org",
        ".edu",
        ".net",
        ".info",
        ".biz",
        ".gov"
    };
    const size_t NUM_SUFFIXES = sizeof(suffixes) / sizeof(char *);

    urlDataLength = 0;
    memset(urlData, 0, sizeof(UrlData_t));

    if ((urlDataIn == NULL) || (strlen(urlDataIn) == 0)) {
        return;
    }

    /*
     * handle prefix
     */
    for (size_t i = 0; i < NUM_PREFIXES; i++) {
        size_t prefixLen = strlen(prefixes[i]);
        if (strncmp(urlDataIn, prefixes[i], prefixLen) == 0) {
            urlData[urlDataLength++]  = i;
            urlDataIn                      += prefixLen;
            break;
        }
    }

    /*
     * handle suffixes
     */
    while (*urlDataIn && (urlDataLength < URL_DATA_MAX)) {
        /* check for suffix match */
        size_t i;
        for (i = 0; i < NUM_SUFFIXES; i++) {
            size_t suffixLen = strlen(suffixes[i]);
            if (strncmp(urlDataIn, suffixes[i], suffixLen) == 0) {
                urlData[urlDataLength++]  = i;
                urlDataIn                      += suffixLen;
                break; /* from the for loop for checking against suffixes */
            }
        }
        /* This is the default case where we've got an ordinary character which doesn't match a suffix. */
        if (i == NUM_SUFFIXES) {
            urlData[urlDataLength++] = *urlDataIn;
            ++urlDataIn;
        }
    }
}
