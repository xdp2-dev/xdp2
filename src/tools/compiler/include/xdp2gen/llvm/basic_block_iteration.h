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

#ifndef XDP2GEN_LLVM_BASIC_BLOCK_ITERATION_H
#define XDP2GEN_LLVM_BASIC_BLOCK_ITERATION_H

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

#include <xdp2gen/program-options/log_handler.h>

namespace xdp2gen
{
namespace llvm
{

std::size_t get_integer_size(::llvm::Value const *value)
{
    std::size_t size = 0;

    plog::log(std::cout) << value->getType()->getTypeID() << std::endl;

    if (value->getType()->isIntegerTy(64))
        size = 64;
    else if (value->getType()->isIntegerTy(32))
        size = 32;
    else if (value->getType()->isIntegerTy(16))
        size = 16;
    else if (value->getType()->isIntegerTy(8))
        size = 8;
    else {
        value->print(::llvm::errs());
        plog::log(std::cerr) << std::endl;
        throw std::runtime_error("Unknown type size");
    }
    return size;
}

}
}

#endif
