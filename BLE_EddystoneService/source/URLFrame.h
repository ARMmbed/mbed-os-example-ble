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

/**
 * Class that encapsulates data that belongs to the Eddystone-URL frame. For
 * more information refer to https://github.com/google/eddystone/tree/master/eddystone-url.
 */
class URLFrame
{
public:
    /**
     * Construct a new instance of this class.
     */
    URLFrame(void);

    /**
     * Construct a new instance of this class.
     *
     * @param[in] urlDataIn
     *              A null terminated string representing a URL.
     */
    URLFrame(const char *urlDataIn);

    /**
     * Construct a new instance of this class.
     *
     * @param[in] urlDataIn
     *              An encoded URL.
     * @param[in] urlDataLengthIn
     *              The length (in bytes) of the encoded URL.
     */
    URLFrame(UrlData_t urlDataIn, uint8_t urlDataLengthIn);

    /**
     * Construct the raw bytes of the Eddystone-URL frame that will be directly
     * used in the advertising packets.
     *
     * @param[in] rawFrame
     *              Pointer to the location where the raw frame will be stored.
     * @param[in] advPowerLevel
     *              Power level value included withing the raw frame.
     */
    void constructURLFrame(uint8_t* rawFrame, int8_t advPowerLevel);

    /**
     * Get the size of the Eddystone-URL frame constructed with the
     * current state of the URLFrame object.
     *
     * @return The size in bytes of the Eddystone-URL frame.
     */
    size_t getRawFrameSize(void) const;

    /**
     * Get a pointer to the encoded URL data.
     *
     * @return A pointer to the encoded URL data.
     */
    uint8_t* getEncodedURLData(void);

    /**
     * Get the encoded URL data length.
     *
     * @return The length (in bytes) of the encoded URL data frame.
     */
    uint8_t getEncodedURLDataLength(void) const;

    /**
     * Set a new URL.
     *
     * @param[in] urlDataIn
     *              A null terminated string containing the new URL.
     */
    void setURLData(const char *urlDataIn);

    /**
     * Set an encoded URL.
     *
     * @param[in] urlEncodedDataIn
     *              A pointer to the encoded URL data.
     * @param[in] urlEncodedDataLengthIn
     *              The lenght of the encoded URL data pointed to by @p
     *              urlEncodedDataIn.
     */
    void setEncodedURLData(const uint8_t* urlEncodedDataIn, const uint8_t urlEncodedDataLengthIn);

private:
    /**
     * Helper function that encodes a URL null terminated string into the HTTP
     * URL Encoding required in Eddystone-URL frames. Refer to
     * https://github.com/google/eddystone/blob/master/eddystone-url/README.md#eddystone-url-http-url-encoding.
     *
     * @param[in] urlDataIn
     *              The null terminated string containing a URL to encode.
     */
    void encodeURL(const char *urlDataIn);

    /**
     *  The byte ID of an Eddystone-URL frame.
     */
    static const uint8_t FRAME_TYPE_URL     = 0x10;
    /**
     * The minimum size (in bytes) of an Eddystone-URL frame.
     */
    static const uint8_t FRAME_MIN_SIZE_URL = 2;

    /**
     * The length of the encoded URL.
     */
    uint8_t              urlDataLength;
    /**
     * The enconded URL data.
     */
    UrlData_t            urlData;

};

#endif /* __URLFRAME_H__ */
