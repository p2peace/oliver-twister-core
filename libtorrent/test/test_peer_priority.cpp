/*

Copyright (c) 2012, Arvid Norberg
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the distribution.
    * Neither the name of the author nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

*/

#include "libtorrent/policy.hpp"
#include "libtorrent/hasher.hpp"
#include "libtorrent/broadcast_socket.hpp" // for supports_ipv6()

#include "test.hpp"

using namespace libtorrent;

boost::uint32_t hash_buffer(char const* buf, int len)
{
	hasher h;
	h.update(buf, len);
	sha1_hash digest = h.final();
	boost::uint32_t ret;
	memcpy(&ret, &digest[0], 4);
	return ntohl(ret);
}

int test_main()
{

	// when the IP is the same, we hash the ports, sorted
	boost::uint32_t p = peer_priority(
		tcp::endpoint(address::from_string("230.12.123.3"), 0x4d2)
		, tcp::endpoint(address::from_string("230.12.123.3"), 0x12c));
	TEST_EQUAL(p, hash_buffer("\x01\x2c\x04\xd2", 4));

	// when we're in the same /24, we just hash the IPs
	p = peer_priority(
		tcp::endpoint(address::from_string("230.12.123.1"), 0x4d2)
		, tcp::endpoint(address::from_string("230.12.123.3"), 0x12c));
	TEST_EQUAL(p, hash_buffer("\xe6\x0c\x7b\x01\xe6\x0c\x7b\x03", 8));

	// when we're in the same /16, we just hash the IPs masked by
	// 0xffffff55
	p = peer_priority(
		tcp::endpoint(address::from_string("230.12.23.1"), 0x4d2)
		, tcp::endpoint(address::from_string("230.12.123.3"), 0x12c));
	TEST_EQUAL(p, hash_buffer("\xe6\x0c\x17\x01\xe6\x0c\x7b\x01", 8));

	// when we're in different /16, we just hash the IPs masked by
	// 0xffff5555
	p = peer_priority(
		tcp::endpoint(address::from_string("230.120.23.1"), 0x4d2)
		, tcp::endpoint(address::from_string("230.12.123.3"), 0x12c));
	TEST_EQUAL(p, hash_buffer("\xe6\x0c\x51\x01\xe6\x78\x15\x01", 8));

	if (supports_ipv6())
	{
		// IPv6 has a twice as wide mask, and we only care about the top 64 bits
		// when the IPs are the same, just hash the ports
		p = peer_priority(
			tcp::endpoint(address::from_string("ffff:ffff:ffff:ffff::1"), 0x4d2)
			, tcp::endpoint(address::from_string("ffff:ffff:ffff:ffff::1"), 0x12c));
		TEST_EQUAL(p, hash_buffer("\x01\x2c\x04\xd2", 4));
        
		// these IPs don't belong to the same /32, so apply the full mask
		// 0xffffffff55555555
		p = peer_priority(
			tcp::endpoint(address::from_string("ffff:ffff:ffff:ffff::1"), 0x4d2)
			, tcp::endpoint(address::from_string("ffff:0fff:ffff:ffff::1"), 0x12c));
		TEST_EQUAL(p, hash_buffer(
			"\xff\xff\x0f\xff\x55\x55\x55\x55\x00\x00\x00\x00\x00\x00\x00\x01"
			"\xff\xff\xff\xff\x55\x55\x55\x55\x00\x00\x00\x00\x00\x00\x00\x01", 32));
	}
	
	return 0;
}

