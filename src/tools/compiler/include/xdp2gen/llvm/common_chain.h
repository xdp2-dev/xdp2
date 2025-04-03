/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
*
* Copyright (c) 2022 SiXDP2 Inc.
*
* Authors: Felipe Magno de Almeida <felipe@expertise.dev>
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
*/

#ifndef XDP2GEN_LLVM_COMMON_CHAIN_H
#define XDP2GEN_LLVM_COMMON_CHAIN_H

#include <boost/io/ios_state.hpp>

namespace xdp2gen
{
namespace llvm
{

struct load {
    std::size_t bit_size;

    friend inline std::ostream &operator<<(std::ostream &os, load l)
    {
        return os << "[load bit_size " << l.bit_size << "]";
    }
};

struct packet_buffer_load {
    std::size_t bit_size;

    bool endian_swap = false;

    friend inline std::ostream &operator<<(std::ostream &os,
                                           packet_buffer_load l)
    {
        auto end_swap_to_str = [](bool endian_swap) -> std::string const {
            return endian_swap ? std::string{ "true" } : std::string{ "false" };
        };

        return os << "[packet_buffer_load bit_size " << l.bit_size
                  << " endian_swap " << end_swap_to_str(l.endian_swap) << "]";
    }
};

struct packet_buffer_getelementptr {
    std::size_t bit_offset, bit_size;

    bool endian_swap = false;

    // |------- bit offset ----|  |bit size|
    // 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16
    // value = (packet{9-12} << shift_amount) && bit_mask

    friend inline std::ostream &operator<<(std::ostream &os,
                                           packet_buffer_getelementptr p)
    {
        auto end_swap_to_str = [](bool endian_swap) -> std::string const {
            return endian_swap ? std::string{ "true" } : std::string{ "false" };
        };

        return os << "[packet_buffer_getelementptr bit_offset " << p.bit_offset
                  << " bit_size " << p.bit_size << " endian_swap "
                  << end_swap_to_str(p.endian_swap) << "]";
    }
};

struct shift {
    int64_t right;

    friend inline std::ostream &operator<<(std::ostream &os, shift l)
    {
        return os << "[shift right " << l.right << "]";
    }
};

struct mask {
    int64_t value;
    std::size_t bit_size;

    friend inline std::ostream &operator<<(std::ostream &os, mask l)
    {
        const std::ostream::sentry ok(os);
        if (ok) {
            boost::io::ios_flags_saver ifs(os);
            os << std::hex;
            return os << "[mask value: 0x" << l.value << std::dec
                      << " bit_size: " << l.bit_size << "]";
        }
        return os;
    }
};

struct zext {
    std::size_t from_size, to_size;

    friend inline std::ostream &operator<<(std::ostream &os, zext l)
    {
        return os << "[zext from_size " << l.from_size << " to_size "
                  << l.to_size << "]";
    }
};

struct constant_value {
    int64_t value;
    size_t bit_size;

    friend inline std::ostream &operator<<(std::ostream &os, constant_value l)
    {
        return os << "[const value " << l.value << " bit_size " << l.bit_size
                  << "]";
    }
};

struct branch {
    bool is_conditional, is_evaluating_instruction;

    // int exit_default;

    friend inline std::ostream &operator<<(std::ostream &os, branch l)
    {
        return os << "[br "
                  << (l.is_conditional ? "conditional" : "unconditional")
                  << "]";
    }
};

struct packet_buffer_offset_masked_multiplied {
    std::size_t bit_offset, bit_size, bit_mask, multiplier;

    int64_t right_shift = 0;

    bool endian_swap = false;

    // |------- bit offset ----|  |bit size|
    // 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16
    // value = (packet{9-12} << shift_amount) && bit_mask

    friend inline std::ostream &
    operator<<(std::ostream &os, packet_buffer_offset_masked_multiplied p)
    {
        auto end_swap_to_str = [](bool endian_swap) -> std::string const {
            return endian_swap ? std::string{ "true" } : std::string{ "false" };
        };

        return os << "[packet_buffer_offset_masked_multiplied bit_offset "
                  << p.bit_offset << ", bit_size " << p.bit_size
                  << ", bit_mask " << p.bit_mask << ", multiplier "
                  << p.multiplier << " , endiand-swap "
                  << end_swap_to_str(p.endian_swap) << "]";
    }
};

}
}

#endif
