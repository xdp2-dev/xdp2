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

#ifndef XDP2GEN_LLVM_PROTO_NEXT_PATTERN_H
#define XDP2GEN_LLVM_PROTO_NEXT_PATTERN_H

#include <xdp2gen/llvm/basic_block_iteration.h>
#include <xdp2gen/llvm/common_chain.h>

#include "llvm/ADT/APInt.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include <xdp2gen/program-options/log_handler.h>

namespace xdp2gen
{
namespace llvm
{

std::string comparison_op_to_string(::llvm::CmpInst::Predicate comp_op)
{
    switch (comp_op) {
    case ::llvm::ICmpInst::ICMP_NE:
        return { "not_equal" };
        break;

    case ::llvm::ICmpInst::ICMP_UGT:
        return { "ugt" };
        break;

    case ::llvm::ICmpInst::ICMP_UGE:
        return { "uge" };
        break;

    case ::llvm::ICmpInst::ICMP_ULT:
        return { "ult" };
        break;

    case ::llvm::ICmpInst::ICMP_ULE:
        return { "ule" };
        break;

    case ::llvm::ICmpInst::ICMP_SGT:
        return { "sgt" };
        break;

    case ::llvm::ICmpInst::ICMP_SGE:
        return { "sge" };
        break;

    case ::llvm::ICmpInst::ICMP_SLT:
        return { "slt" };
        break;

    case ::llvm::ICmpInst::ICMP_SLE:
        return { "sle" };
        break;

    case ::llvm::ICmpInst::ICMP_EQ:
    default:
        return { "equal" };
        break;
    }
}

::llvm::ICmpInst::Predicate invert_comparison_op(::llvm::ICmpInst::Predicate op)
{
    switch (op) {
    case ::llvm::ICmpInst::ICMP_NE:
        return ::llvm::ICmpInst::ICMP_EQ;
        break;

    case ::llvm::ICmpInst::ICMP_EQ:
    default:
        return ::llvm::ICmpInst::ICMP_NE;
        break;
    }
}

struct condition {
    using operand_type =
        std::variant<constant_value, packet_buffer_offset_masked_multiplied>;

    ::llvm::ICmpInst::Predicate comparison_op;

    operand_type lhs;

    operand_type rhs;

    constant_value default_fail{ 0, 0 };

    friend inline std::ostream &operator<<(std::ostream &os, condition l)
    {
        os << "[condition default_fail " << l.default_fail << " comparison_op "
           << comparison_op_to_string(l.comparison_op);

        if (std::holds_alternative<constant_value>(l.lhs))
            os << " lhs " << std::get<constant_value>(l.lhs);

        else if (std::holds_alternative<packet_buffer_offset_masked_multiplied>(
                     l.lhs))
            os << " lhs "
               << std::get<packet_buffer_offset_masked_multiplied>(l.lhs);

        if (std::holds_alternative<constant_value>(l.rhs))
            os << " rhs " << std::get<constant_value>(l.rhs);

        else if (std::holds_alternative<packet_buffer_offset_masked_multiplied>(
                     l.rhs))
            os << " rhs "
               << std::get<packet_buffer_offset_masked_multiplied>(l.rhs);

        return os << "]";
    }
};

struct swap {
    friend inline std::ostream &operator<<(std::ostream &os, swap s)
    {
        return (os << "[swap]");
    }
};

}

}

#endif
