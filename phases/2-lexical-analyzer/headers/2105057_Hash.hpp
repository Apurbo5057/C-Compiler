// Guard against multiple declarations
#ifndef HASH_CLASS_HEADER
#define HASH_CLASS_HEADER

#include <iostream>
#include <string>
#include <cstdint>

using namespace std;

// Hash class provides different hash functions
class Hash
{
public:
    // SDBM hash function provided by instructor
    // static uint64_t SDBMHash(string str, uint64_t num_buckets)
    // {
    //     uint64_t hash = 0;
    //     uint64_t len = str.length();
    //     for (uint64_t i = 0; i < len; i++)
    //     {
    //         hash = ((str[i]) + (hash << 6) + (hash << 16) - hash) % num_buckets;
    //     }
    //     return hash % num_buckets;
    // }

    /*
    sdf
    
    */

    static uint64_t SDBMHash(string str, uint64_t num_buckets)
    {
        unsigned int hash = 0;
        for (char ch : str)
        {
            int c = static_cast<unsigned char>(ch);
            hash = c + (hash << 6) + (hash << 16) - hash;
        }
        return static_cast<uint64_t>(hash) % num_buckets;
    }



    // DJB2 hash function, attributed to Daniel J. Bernstein
    // Source: http://www.cse.yorku.ca/~oz/hash.html
    static uint64_t DJB2Hash(string str, uint64_t num_buckets)
    {
        uint64_t hash = 5381;
        for (char c : str)
        {
            hash = ((hash << 5) + hash) + static_cast<uint8_t>(c); // hash * 33 + c
            hash %= num_buckets;
        }
        return hash;
    }

    // FNV-1a hash function
    // Source: http://www.isthe.com/chongo/tech/comp/fnv/
    static uint64_t FNV1AHash(string str, uint64_t num_buckets)
    {
        uint64_t hash = 14695981039346656037ULL; // FNV offset basis for 64-bit
        uint64_t prime = 1099511628211ULL;        // FNV prime for 64-bit
        for (char c : str)
        {
            hash ^= static_cast<uint8_t>(c);
            hash *= prime;
            hash %= num_buckets;
        }
        return hash;
    }

    // Simple sum hash for comparison
    static uint64_t SumHash(string str, uint64_t num_buckets)
    {
        uint64_t hash = 0;
        for (char c : str)
        {
            hash += static_cast<uint8_t>(c);
            hash %= num_buckets;
        }
        return hash;
    }
};

#endif


