/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2025 Tom Herbert
 *
 * Authors: Felipe Magno de Almeida <felipe@expertise.dev>
 *          João Paulo Taylor Ienczak Zanette <joao.tiz@expertise.dev>
 *          Lucas Cavalcante de Sousa <lucas@expertise.dev>
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

#ifndef XDP2GEN_INCLUDE_XDP2GEN_X3_TOKEN_GRAMMAR_H
#define XDP2GEN_INCLUDE_XDP2GEN_X3_TOKEN_GRAMMAR_H

#include <boost/spirit/home/x3.hpp>

#include <boost/spirit/home/x3/core/parser.hpp>
#include <boost/spirit/home/x3/core/skip_over.hpp>
#include <boost/spirit/home/x3/operator/sequence.hpp>

namespace xdp2gen
{
namespace x3
{

namespace x3 = boost::spirit::x3;

template <typename P> void
assign_from_token(boost::wave::cpplexer::lex_token<P> &to,
		  boost::wave::cpplexer::lex_token<P> const &from)
{
	to = from;
}

template <typename P, typename String> void
assign_from_token(String &to, boost::wave::cpplexer::lex_token<P> const &from)
{
	auto str = from.get_value();

	to = { str.begin(), str.end() };
}

struct token_char : x3::parser<token_char> {
	constexpr token_char (char c) : c{ c } {}
	static bool const handles_container = false;
	static bool const has_attribute = false;
	typedef x3::unused_type attribute_type;
	char c;

	template <typename Iterator, typename Context,
		  typename Attribute_> bool
	parse(Iterator &first, Iterator const &last, Context const &context,
	      x3::unused_type, Attribute_ &attr) const
	{
		x3::skip_over(first, last, context);
		if (first != last) {
			auto str = first->get_value();

			if (str.size() && c == str[0]) {
				++first;
				return true;
			}
		}
		return false;
	}
};

struct token_identifier : x3::parser<token_identifier> {
	static bool const handles_container = false;
	static bool const has_attribute = true;
	typedef std::string attribute_type;

	template <typename Iterator, typename Context, typename Attribute_> bool
	parse(Iterator &first, Iterator const &last,
		   Context const &context, x3::unused_type,
		   Attribute_ &attr) const
	{
		x3::skip_over(first, last, context);
		if (first != last) {
			if (*first == boost::wave::T_IDENTIFIER) {
				xdp2gen::x3::assign_from_token(attr, *first);
				++first;
				return true;
			}
		}
		return false;
	}
};

struct any_token : x3::parser<any_token> {
	static bool const handles_container = false;
	static bool const has_attribute = true;
	typedef std::string attribute_type;

	template <typename Iterator, typename Context, typename Attribute_> bool
	parse(Iterator &first, Iterator const &last, Context const &context,
	      x3::unused_type, Attribute_ &attr) const
	{
		x3::skip_over(first, last, context);
		if (first != last) {
			xdp2gen::x3::assign_from_token(attr, *first);
			++first;
			return true;
		} else {
			return false;
		}
	}
};

struct token_any_space : x3::parser<token_any_space> {
	static bool const handles_container = false;
	static bool const has_attribute = false;
	typedef x3::unused_type attribute_type;

	template <typename Iterator, typename Context, typename Attribute_> bool
	parse(Iterator &first, Iterator const &last, Context const &context,
	      x3::unused_type, Attribute_ &attr) const
	{
		x3::skip_over(first, last, context);
		if (first != last) {
			boost::wave::token_id id = *first;
			if ((id & boost::wave::TokenTypeMask) ==
					boost::wave::WhiteSpaceTokenType) {
				++first;
				return true;
			}
		}
		return false;
	}
};

constexpr token_char operator"" _t(char c) { return { c }; }

namespace _impl
{
[[maybe_unused]] constexpr const any_token token = {};
[[maybe_unused]] constexpr const token_identifier identifier = {};
[[maybe_unused]] constexpr const token_any_space spaces = {};
}

using _impl::identifier;
using _impl::spaces;
using _impl::token;

} // namespace x3
} // namespace xdp2gen

#endif
