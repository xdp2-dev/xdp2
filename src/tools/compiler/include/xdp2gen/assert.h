// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
 * Copyright (c) 2024 SiXDP2 Inc.
 *
 * Assertion utilities for xdp2-compiler.
 *
 * Thin wrappers around Boost.Assert for common patterns.
 *
 * Compile-time control:
 *   -DXDP2_ENABLE_ASSERTS=1   Enable all XDP2 assertions
 *
 * Default: assertions disabled (zero overhead).
 * For Nix debug/test builds, define XDP2_ENABLE_ASSERTS in the derivation.
 */

#ifndef XDP2GEN_ASSERT_H
#define XDP2GEN_ASSERT_H

#include <boost/assert.hpp>

/*
 * Null pointer check macros.
 *
 * XDP2_REQUIRE_NOT_NULL(ptr, context)
 *   - Returns ptr unchanged
 *   - When XDP2_ENABLE_ASSERTS: checks ptr != nullptr, aborts with message if null
 *   - When disabled: compiles to just (ptr) - zero overhead, no string in binary
 *
 * Usage:
 *   auto *decl = XDP2_REQUIRE_NOT_NULL(
 *       record_type->getDecl(),
 *       "RecordDecl from RecordType");
 */

#ifdef XDP2_ENABLE_ASSERTS

#define XDP2_REQUIRE_NOT_NULL(PTR, CONTEXT)				\
	(BOOST_ASSERT_MSG((PTR) != nullptr, CONTEXT), (PTR))

#else

#define XDP2_REQUIRE_NOT_NULL(PTR, CONTEXT) (PTR)

#endif // XDP2_ENABLE_ASSERTS

/*
 * Convenience macros for pre/postconditions.
 *
 * When XDP2_ENABLE_ASSERTS is defined:
 *   - Expands to BOOST_ASSERT_MSG
 *
 * When XDP2_ENABLE_ASSERTS is NOT defined:
 *   - Expands to nothing (zero overhead)
 *
 * Usage:
 *   XDP2_REQUIRE(ptr != nullptr, "ptr must be valid");
 *   XDP2_ENSURE(result > 0, "result must be positive");
 */

#ifdef XDP2_ENABLE_ASSERTS

#define XDP2_REQUIRE(CONDITION, MESSAGE)				\
	BOOST_ASSERT_MSG((CONDITION), "Precondition: " MESSAGE)

#define XDP2_ENSURE(CONDITION, MESSAGE)					\
	BOOST_ASSERT_MSG((CONDITION), "Postcondition: " MESSAGE)

#else

#define XDP2_REQUIRE(CONDITION, MESSAGE) ((void)0)
#define XDP2_ENSURE(CONDITION, MESSAGE)  ((void)0)

#endif // XDP2_ENABLE_ASSERTS

#endif // XDP2GEN_ASSERT_H
