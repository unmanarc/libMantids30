#ifndef STREAM_READER_H
#define STREAM_READER_H

#include <stdint.h>
#include <string.h>

#include <string>
#include <limits>
#include <stdexcept>

namespace Mantids { namespace Network { namespace Streams {


class StreamSocketReader
{
public:
    StreamSocketReader();
    virtual ~StreamSocketReader();


    template<typename T>
    T readU(bool *readOK=nullptr)
    {
        T r = 0;
        bool _readOK;

        switch(sizeof(T))
        {
        case 1:
            r = readU8(&_readOK);
            break;
        case 2:
            r = readU16(&_readOK);
            break;
        case 4:
            r = readU32(&_readOK);
            break;
        case 8:
            r = readU64(&_readOK);
            break;
        default:
            throw std::runtime_error("Code Error: reading block with invalid lenght variable size.");
            _readOK = false;
        }

        // Sets readOK
        if (readOK) *readOK = _readOK;

        if (!_readOK) readDeSync();

        return r;
    }

    /**
    * Read data block (lenght header->data)
    * @param data data to allocate the incoming bytes.
    * @param datalen maximum data length to be allocated, and returns the data received...
    * @param mustReceiveFullDataLen maximum data length to be received should match the incoming data, otherwise,return false (and you should close)
    * @return true if succeed.
    */
    template<typename T>
    bool readBlockEx(void * data,T * datalen, bool mustReceiveFullDataLen = false)
    {
        bool readOK;
        T len;

        // no data to be received:
        if (*datalen == 0) return true;

        len = readU<T>(&readOK);

        if (readOK)
        {
            if ((len != *datalen && mustReceiveFullDataLen) || len > *datalen)
            {
                readDeSync();
                return false;
            }           

            *datalen = len;

            if (!len) // Nothing to get here.
                return true;

            uint64_t r;
            return (readFull(data, len, &r) && r==len);
        }
        return false;
    }
    /**
        * Read and allocate a memory space data block
        * NOTE: Allocation occurs with new [], so delete it with delete []
        * @param datalen in: maximum data length supported, out: data retrieved. (don't go to extremes, check your system)
        * @return memory allocated with the retrieved data or nullptr if failed to allocate memory.
        */
    template<typename T>
    char *readBlockWAllocEx(T * datalen)
    {
        bool readOK;
        T expectedBytesCount;
        T virtualMax = std::numeric_limits<T>::max()-1;

        if (!datalen)
            datalen = &virtualMax;

        if (*datalen> std::numeric_limits<size_t>::max())
            throw std::runtime_error("Potential HOF. Aborting execution");

        expectedBytesCount = readU<T>(&readOK);
        if (readOK)
        {
            if (expectedBytesCount > *datalen)  // len received exceeded the max datalen permited.
            {
                *datalen = 0;
                readDeSync();
                return nullptr;
            }

            // download and resize (be careful, datalen limit is here to prevent you to receive
            char * odata = new char[expectedBytesCount+1];
            if (!odata)
            {
                readDeSync();
                return nullptr; // not enough memory.
            }

            memset(odata,0,expectedBytesCount+1);

            if (!expectedBytesCount)
            {
                // Empty string OK...
                *datalen = 0;
                return odata;
            }

            uint64_t r;
            bool ok = readFull(odata, expectedBytesCount,&r) && r==expectedBytesCount;
            if (!ok)
            {
                delete [] odata;
                *datalen = 0;
                readDeSync();
                return nullptr;
            }
            *datalen = expectedBytesCount;
            return odata;
        }


        *datalen = 0;
        return nullptr;
    }
    /**
        * Read and allocate a memory space data block of maximum of 4g bytes until a delimiter bytes (teotherical).
        * @param datalen in: maximum data length supported (should be min: 65536), out: data retrieved.
        * @param delim delimiter.
        * @param delimBytes delimiter size (max: 65535 bytes).
        * @return memory allocated with the retrieved data or nullptr if failed.
        */
    char *readBlock32WAllocAndDelim(unsigned int * datalen, const char *delim, uint16_t delimBytes);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Null terminated strings:

    template<typename T>
    std::string readStringEx(bool *readOK=nullptr , const T & maxLenght = std::numeric_limits<T>::max()-1)
    {
        T receivedBytes = maxLenght;

        if (maxLenght > std::numeric_limits<size_t>::max())
            throw std::runtime_error("Potential HOF. Aborting execution");

        if (readOK) *readOK = true;

        // readBlockWAllocEx will handle readDeSync();
        char * data = (char *)readBlockWAllocEx<T>(&receivedBytes);
        if (!data)
        {
            if (readOK) *readOK = false;
            return "";
        }

        if (!receivedBytes)
        {
            delete [] data;
            return "";
        }
        else
        {
            std::string v(data,receivedBytes);
            delete [] data;
            return v;
        }
    }


protected:
    virtual bool readFull(void * data, const uint64_t & datalen, uint64_t * bytesReceived = nullptr) = 0;
    virtual void readDeSync()=0;

private:
    int32_t read64KBlockDelim(char * block, const char* delim, const uint16_t & delimBytes, const uint32_t &blockNo);
    /**
        * Read unsigned char
        * @param readOK pointer to bool variable that will be filled with the result (success or fail).
        * @return char retrived.
        * */
    unsigned char readU8(bool *readOK=nullptr);
    /**
        * Read unsigned short (16bit)
        * @param readOK pointer to bool variable that will be filled with the result (success or fail).
        * @return char retrived.
        * */
    uint16_t readU16(bool *readOK=nullptr);
    /**
        * Read unsigned integer (32bit)
        * @param readOK pointer to bool variable that will be filled with the result (success or fail).
        * @return char retrived.
        * */
    uint32_t readU32(bool *readOK=nullptr);
    /**
        * Read unsigned integer (46bit)
        * @param readOK pointer to bool variable that will be filled with the result (success or fail).
        * @return char retrived.
        * */
    uint64_t readU64(bool *readOK=nullptr);
};

}}}

#endif // STREAM_READER_H
