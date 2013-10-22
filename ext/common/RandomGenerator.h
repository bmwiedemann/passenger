/*
 *  Phusion Passenger - https://www.phusionpassenger.com/
 *  Copyright (c) 2010-2013 Phusion
 *
 *  "Phusion Passenger" is a trademark of Hongli Lai & Ninh Bui.
 *
 *  See LICENSE file for license information.
 */
#ifndef _PASSENGER_RANDOM_GENERATOR_H_
#define _PASSENGER_RANDOM_GENERATOR_H_

#include <string>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <oxt/system_calls.hpp>

#include "StaticString.h"
#include "Exceptions.h"
#include "Utils/StrIntUtils.h"


/**
 * A random 
 */
namespace Passenger {

using namespace std;
using namespace boost;
using namespace oxt;

/**
 * A random data generator. Data is generated using /dev/urandom, and
 * is cryptographically secure. Unlike rand() and friends,
 * RandomGenerator does not require seeding.
 *
 * The reason why RandomGenerator isn't a singleton is because opening
 * /dev/urandom is *very* slow on Mac OS X and OpenBSD. Each object of
 * this class caches the /dev/urandom file handle.
 *
 * This class is thread-safe as long as there are no concurrent
 * calls to reopen() or close().
 */
class RandomGenerator: public boost::noncopyable {
private:
	FILE *handle;
	
public:
	RandomGenerator(bool open = true) {
		handle = NULL;
		if (open) {
			reopen();
		}
	}
	
	~RandomGenerator() {
		this_thread::disable_syscall_interruption dsi;
		close();
	}
	
	void reopen() {
		close();
		handle = syscalls::fopen("/dev/urandom", "r");
		if (handle == NULL) {
			throw FileSystemException("Cannot open /dev/urandom",
				errno, "/dev/urandom");
		}
	}
	
	void close() {
		if (handle != NULL) {
			syscalls::fclose(handle);
			handle = NULL;
		}
	}
	
	StaticString generateBytes(void *buf, unsigned int size) {
		size_t ret = syscalls::fread(buf, 1, size, handle);
		if (ret != size) {
			throw IOException("Cannot read sufficient data from /dev/urandom");
		}
		return StaticString((const char *) buf, size);
	}
	
	string generateByteString(unsigned int size) {
		string result(size, '\0');
		generateBytes(&result[0], size);
		return result;
	}
	
	string generateHexString(unsigned int size) {
		string buf(size, '\0');
		generateBytes(&buf[0], size);
		return toHex(buf);
	}
	
	/**
	 * Generates a random string of <em>size</em> bytes which is also
	 * valid ASCII. The result consists only of the characters A-Z,
	 * a-z and 0-9, and therefore the total number of possibilities
	 * given a size of N is 62**N. However not every character has an
	 * equal chance of being chosen: a-i have 5/256 chance of being
	 * chosen, while other characters have 4/256 chance of being chosen.
	 * Therefore, to match the entropy of a random binary string of
	 * size N, one should choose a <em>size</em> which yields slightly
	 * more possibilities than 2**N.
	 */
	string generateAsciiString(unsigned int size) {
		string result(size, '\0');
		generateAsciiString(&result[0], size);
		return result;
	}
	
	void generateAsciiString(char *_buf, unsigned int size) {
		static const char chars[] = {
			'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
			'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
			'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
			'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
			'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
		};
		unsigned char *buf = (unsigned char *) _buf;
		generateBytes(buf, size);
		for (unsigned int i = 0; i < size; i++) {
			buf[i] = chars[buf[i] % sizeof(chars)];
		}
	}
	
	int generateInt() {
		int ret;
		generateBytes(&ret, sizeof(ret));
		return ret;
	}
	
	unsigned int generateUint() {
		return (unsigned int) generateInt();
	}
};

typedef boost::shared_ptr<RandomGenerator> RandomGeneratorPtr;

} // namespace Passenger

#endif /* _PASSENGER_RANDOM_GENERATOR_H_ */
