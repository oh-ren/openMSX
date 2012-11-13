// $Id$

// UTF8-CPP   http://utfcpp.sourceforge.net/
//   Slightly simplified (and reformatted) to fit openMSX coding style.

// Copyright 2006 Nemanja Trifunovic

/*
Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/


#ifndef UTF8_CORE_HH
#define UTF8_CORE_HH

#include <iterator>

namespace utf8 {

// The typedefs for 8-bit, 16-bit and 32-bit unsigned integers
// You may need to change them to match your system.
// These typedefs have the same names as ones from cstdint, or boost/cstdint
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

// Helper code - not intended to be directly called by the library users.
// May be changed at any time
namespace internal {

// Unicode constants
// Leading (high) surrogates: 0xd800 - 0xdbff
// Trailing (low) surrogates: 0xdc00 - 0xdfff
const uint16_t LEAD_SURROGATE_MIN  = 0xd800u;
const uint16_t LEAD_SURROGATE_MAX  = 0xdbffu;
const uint16_t TRAIL_SURROGATE_MIN = 0xdc00u;
const uint16_t TRAIL_SURROGATE_MAX = 0xdfffu;
const uint16_t LEAD_OFFSET         = LEAD_SURROGATE_MIN - (0x10000 >> 10);
const uint32_t SURROGATE_OFFSET    = 0x10000u - (LEAD_SURROGATE_MIN << 10) - TRAIL_SURROGATE_MIN;

// Maximum valid value for a Unicode code point
const uint32_t CODE_POINT_MAX      = 0x0010ffffu;

inline bool is_trail(uint8_t oc)
{
	return (oc >> 6) == 0x2;
}

inline bool is_surrogate(uint16_t cp)
{
	return (cp >= LEAD_SURROGATE_MIN) && (cp <= TRAIL_SURROGATE_MAX);
}

inline bool is_code_point_valid(uint32_t cp)
{
	return (cp <= CODE_POINT_MAX) && !is_surrogate(cp) &&
	       (cp != 0xfffe) && (cp != 0xffff);
}

inline unsigned sequence_length(uint8_t lead)
{
	if (lead < 0x80) {
		return 1;
	} else if ((lead >> 5) == 0x06) {
		return 2;
	} else if ((lead >> 4) == 0x0e) {
		return 3;
	} else if ((lead >> 3) == 0x1e) {
		return 4;
	} else {
		return 0;
	}
}

enum utf_error {
	OK,
	NOT_ENOUGH_ROOM,
	INVALID_LEAD,
	INCOMPLETE_SEQUENCE,
	OVERLONG_SEQUENCE,
	INVALID_CODE_POINT
};

template <typename octet_iterator>
utf_error validate_next(octet_iterator& it, octet_iterator end,
                        uint32_t* code_point)
{
	uint32_t cp = *it;
	// Check the lead octet
	int length = sequence_length(*it);

	// "Shortcut" for ASCII characters
	if (length == 1) {
		if (end - it <= 0) {
			return NOT_ENOUGH_ROOM;
		}
		if (code_point) {
			*code_point = cp;
		}
		++it;
		return OK;
	}

	// Do we have enough memory?
	if (std::distance(it, end) < length) {
		return NOT_ENOUGH_ROOM;
	}

	// Check trail octets and calculate the code point
	switch (length) {
	case 0:
		return INVALID_LEAD;
		break;
	case 2:
		if (is_trail(*(++it))) {
			cp = ((cp << 6) & 0x7ff) + ((*it) & 0x3f);
		} else {
			--it;
			return INCOMPLETE_SEQUENCE;
		}
		break;
	case 3:
		if (is_trail(*(++it))) {
			cp = ((cp << 12) & 0xffff) + ((*it << 6) & 0xfff);
			if (is_trail(*(++it))) {
				cp += (*it) & 0x3f;
			} else {
				std::advance(it, -2);
				return INCOMPLETE_SEQUENCE;
			}
		} else {
			--it;
			return INCOMPLETE_SEQUENCE;
		}
		break;
	case 4:
		if (is_trail(*(++it))) {
			cp = ((cp << 18) & 0x1fffff) + ((*it << 12) & 0x3ffff);
			if (is_trail(*(++it))) {
				cp += (*it << 6) & 0xfff;
				if (is_trail(*(++it))) {
					cp += (*it) & 0x3f;
				} else {
					std::advance(it, -3);
					return INCOMPLETE_SEQUENCE;
				}
			} else {
				std::advance(it, -2);
				return INCOMPLETE_SEQUENCE;
			}
		} else {
			--it;
			return INCOMPLETE_SEQUENCE;
		}
		break;
	}
	// Is the code point valid?
	if (!is_code_point_valid(cp)) {
		for (int i = 0; i < length - 1; ++i) {
			--it;
		}
		return INVALID_CODE_POINT;
	}

	if (code_point) {
		*code_point = cp;
	}
	if (cp < 0x80) {
		if (length != 1) {
			std::advance(it, -(length-1));
			return OVERLONG_SEQUENCE;
		}
	} else if (cp < 0x800) {
		if (length != 2) {
			std::advance(it, -(length-1));
			return OVERLONG_SEQUENCE;
		}
	} else if (cp < 0x10000) {
		if (length != 3) {
			std::advance(it, -(length-1));
			return OVERLONG_SEQUENCE;
		}
	}

	++it;
	return OK;
}

template <typename octet_iterator>
inline utf_error validate_next(octet_iterator& it, octet_iterator end) {
	return validate_next(it, end, nullptr);
}

} // namespace internal

/// The library API - functions intended to be called by the users

// Byte order mark
const uint8_t bom[] = { 0xef, 0xbb, 0xbf };

template <typename octet_iterator>
octet_iterator find_invalid(octet_iterator start, octet_iterator end)
{
	auto result = start;
	while (result != end) {
		internal::utf_error err_code = internal::validate_next(result, end);
		if (err_code != internal::OK) {
			return result;
		}
	}
	return result;
}

template <typename octet_iterator>
inline bool is_valid(octet_iterator start, octet_iterator end)
{
	return find_invalid(start, end) == end;
}

template <typename octet_iterator>
inline bool is_bom(octet_iterator it)
{
	return ((*it++ == bom[0]) &&
	        (*it++ == bom[1]) &&
	        (*it   == bom[2]));
}

template<typename octet_iterator>
inline octet_iterator sync_forward(octet_iterator it)
{
	while (internal::is_trail(*it)) ++it;
	return it;
}

template<typename octet_iterator>
inline octet_iterator sync_backward(octet_iterator it)
{
	while (internal::is_trail(*it)) --it;
	return it;
}

} // namespace utf8

#endif
