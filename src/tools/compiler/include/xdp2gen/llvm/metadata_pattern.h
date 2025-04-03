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

#ifndef XDP2GEN_LLVM_METADATA_PATTERN_H
#define XDP2GEN_LLVM_METADATA_PATTERN_H

#include <xdp2gen/llvm/basic_block_iteration.h>

#include <iostream>
#include <sstream>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/ModuleSlotTracker.h"
#include "llvm/IR/Module.h"

#include <xdp2gen/program-options/log_handler.h>

namespace xdp2gen
{
namespace llvm
{

enum counter_type {

    PRE_METADATA_INC,
    PRE_METADATA_RESET,
    POST_METADATA_INC,
    POST_METADATA_RESET

};

struct package_metadata_counter {
    std::size_t num_elems;

    counter_type cntr_type;

    std::size_t offset;

    std::size_t elem_size;

    std::string name;

    std::size_t counter_idx_size;

    std::size_t local_to_counter_offset;

    std::size_t field_size;

    std::size_t get_hash() const
    {
        std::hash<std::string> str_hash;
        std::hash<size_t> num_hash;

        return num_hash(this->elem_size) + num_hash(this->num_elems) +
               num_hash(this->cntr_type);
    };

    std::string get_hash_as_hex_string() const
    {
        std::size_t hash_val = this->get_hash();

        std::stringstream ss;

        ss << "0x" << std::hex << hash_val;

        return ss.str();
    }

    struct pmc_hash {
        std::size_t operator()(package_metadata_counter const &pmc) const
        {
            return pmc.get_hash();
        }

        bool operator()(package_metadata_counter const &pmc1,
                        package_metadata_counter const &pmc2) const
        {
            bool has_same_name = pmc1.name == pmc2.name;
            bool has_same_elem_size = pmc1.elem_size == pmc2.elem_size;
            bool has_same_num_elems = pmc1.num_elems == pmc2.num_elems;
            bool is_same_type_of_counter = pmc1.cntr_type == pmc2.cntr_type;

            return has_same_elem_size && has_same_num_elems &&
                   is_same_type_of_counter;
        }
    };

    std::string type_to_string() const
    {
        switch (this->cntr_type) {
        case counter_type::PRE_METADATA_INC:
            return std::string("pre_metadata_inc");
        case counter_type::POST_METADATA_INC:
            return std::string("post_metadata_inc");
        case counter_type::PRE_METADATA_RESET:
            return std::string("pre_metadata_reset");
        case counter_type::POST_METADATA_RESET:
            return std::string("post_metadata_reset");
        }

        return std::string("");
    }

    friend inline std::ostream &operator<<(std::ostream &os,
                                           package_metadata_counter p)
    {
        return os << "[package_metadata_counter " //bit_size " << p.bit_size
                  << " num_eles " << p.num_elems << " counter_type "
                  << p.type_to_string() << " offset " << p.offset
                  << " elem_size " << p.elem_size << " name " << p.name << "]";
    }
};

struct mask_shift {
    mask mask_val;

    shift shift_val;

    friend inline std::ostream &operator<<(std::ostream &os, mask_shift ms)
    {
        return os << "[mask_shift " << ms.mask_val << " " << ms.shift_val
                  << "]";
    }
};

struct metadata_write_constant {
    std::size_t value, bit_size, dst_bit_offset;

    bool is_frame = false;

    std::optional<package_metadata_counter> pckg_metadata_counter;

    std::optional<mask_shift> mask_shift_val;

    std::optional<std::size_t> different_dst_bit_size;

    friend inline std::ostream &operator<<(std::ostream &os,
                                           metadata_write_constant p)
    {
        std::stringstream oss;

        if (p.pckg_metadata_counter.has_value()) {
            oss << " " << p.pckg_metadata_counter.value();
        }

        return os << "[metadata_write_constant value " << p.value
                  << ", bit_dst_offset " << p.dst_bit_offset << ", bit_size "
                  << p.bit_size << oss.str() << "]";
    }
};

struct metadata_write_header_offset {
    std::size_t dst_bit_offset, bit_size, src_bit_offset;

    bool is_frame = false;

    std::optional<package_metadata_counter> pckg_metadata_counter;

    std::optional<mask_shift> mask_shift_val;

    friend inline std::ostream &operator<<(std::ostream &os,
                                           metadata_write_header_offset p)
    {
        std::stringstream oss;

        if (p.pckg_metadata_counter.has_value()) {
            oss << " " << p.pckg_metadata_counter.value();
        }

        return os << "[metadata_write_header_offset "
                  << " dst_bit_offset " << p.dst_bit_offset << ", bit_size "
                  << p.bit_size << ", src_bit_offset " << p.src_bit_offset
                  << oss.str() << "]";
    }
};

struct metadata_write_header_length {
    std::size_t dst_bit_offset, bit_size, src_bit_offset;

    bool is_frame = false;

    std::optional<package_metadata_counter> pckg_metadata_counter;

    std::optional<mask_shift> mask_shift_val;

    friend inline std::ostream &
    operator<<(std::ostream &os, metadata_write_header_length const &p)
    {
        std::stringstream oss;

        if (p.pckg_metadata_counter.has_value()) {
            oss << " " << p.pckg_metadata_counter.value();
        }

        return os << "[metadata_write_header_length dst_bit_offset "
                  << p.dst_bit_offset << ", bit_size " << p.bit_size
                  << ", src_bit_offset " << p.src_bit_offset << oss.str()
                  << "]";
    }
};

struct metadata_transfer {
    std::size_t src_bit_offset, dst_bit_offset, bit_size;

    bool is_frame = false;

    std::optional<package_metadata_counter> pckg_metadata_counter;

    std::optional<mask_shift> mask_shift_val;

    std::optional<std::size_t> different_dst_bit_size;

    std::optional<bool> endian_swap;

    friend inline std::ostream &operator<<(std::ostream &os,
                                           metadata_transfer p)
    {
        std::stringstream oss;

        if (p.pckg_metadata_counter.has_value()) {
            oss << " " << p.pckg_metadata_counter.value();
        }

        auto end_swap_to_str =
            [](std::optional<bool> endian_swap) -> std::string const {
            if (endian_swap)
                return endian_swap.value() ? std::string{ "true" } :
                                             std::string{ "false" };
            else
                return { "false" };
        };

        return os << "[metadata_transfer bit_src_offset " << p.src_bit_offset
                  << ", bit_dst_offset " << p.dst_bit_offset << ", bit_size "
                  << p.bit_size << ", endian_swap "
                  << end_swap_to_str(p.endian_swap) << oss.str() << "]";
    }
};

struct metadata_value_transfer {
    std::size_t src_bit_offset, dst_bit_offset, bit_size;

    std::string type;

    std::optional<package_metadata_counter> pckg_metadata_counter;

    std::optional<mask_shift> mask_shift_val;

    std::optional<std::size_t> different_dst_bit_size;

    friend inline std::ostream &operator<<(std::ostream &os,
                                           metadata_value_transfer p)
    {
        return os << "[metadata_value_transfer bit_src_offset "
                  << p.src_bit_offset << ", bit_dst_offset " << p.dst_bit_offset
                  << ", bit_size " << p.bit_size << ", type "
                  << p.type
                  // << oss.str()
                  << "]";
    }
};

}
}

#endif
