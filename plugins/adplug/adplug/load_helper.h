#ifndef ADPLUG_LOAD_HELPER_H
#define ADPLUG_LOAD_HELPER_H

#include <cstring>
#include <stdint.h>

inline uint16_t adplug_byteswap(const uint16_t val)
{
  return
    ((val & 0x00FF) << 8) |
    ((val & 0xFF00) >> 8);
}

inline uint32_t adplug_byteswap(const uint32_t val)
{
  return
    ((val & 0x000000FF) << 24) |
    ((val & 0x0000FF00) <<  8) |
    ((val & 0x00FF0000) >>  8) |
    ((val & 0xFF000000) >> 24);
}

// In many cases, we need to load a uint16_t/uint32_t from a (possibly)
// unaligned byte stream. In order to avoid undefined behavior, we have to use
// memcpy as the only portable way to perform type punning. See:
//   https://blog.regehr.org/archives/959
template <typename T>
static inline T load_unaligned_impl(const unsigned char* src, const bool big_endian)
{
  T result;
  std::memcpy(&result, src, sizeof(T));

#ifdef WORDS_BIGENDIAN
  // big-endian CHOST
  if (!big_endian)
#else
  // little-endian CHOST
  if (big_endian)
#endif
  {
    // have to do a byte-swap
    result = adplug_byteswap(result);
  }

  return result;
}

inline uint16_t u16_unaligned(const unsigned char* src, const bool big_endian = false)
{
  return load_unaligned_impl<uint16_t>(src, big_endian);
}

inline uint32_t u32_unaligned(const unsigned char* src, const bool big_endian = false)
{
  return load_unaligned_impl<uint32_t>(src, big_endian);
}

#endif  // ADPLUG_LOAD_HELPER_H
