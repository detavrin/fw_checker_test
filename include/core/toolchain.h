/**
 * @file
 * @brief Macros to abstract toolchain specific capabilities
 *
 * This file contains various macros to abstract compiler capabilities that
 * utilize toolchain specific attributes and/or pragmas.
 */

#ifndef INC_CORE_TOOLCHAIN_H_
#define INC_CORE_TOOLCHAIN_H_

#include <stdbool.h>

/**
 * @def HAS_BUILTIN(x)
 * @brief Check if the compiler supports the built-in function \a x.
 *
 * This macro is for use with conditional compilation to enable code using a
 * builtin function that may not be available in every compiler.
 */
#ifdef __has_builtin
#define HAS_BUILTIN(x) __has_builtin(x)
#else
/*
 * The compiler doesn't provide the __has_builtin() macro, so instead we depend
 * on the toolchain-specific headers to define HAS_BUILTIN_x for the builtins
 * supported.
 */
#define HAS_BUILTIN(x) HAS_BUILTIN_##x
#endif

#ifndef __GNUC__
#error "Invalid/unknown toolchain configuration"
#endif

#define TOOLCHAIN_GCC_VERSION \
	((__GNUC__ * 10000) + (__GNUC_MINOR__ * 100) + __GNUC_PATCHLEVEL__)


#ifndef __ORDER_BIG_ENDIAN__
#define __ORDER_BIG_ENDIAN__            (1)
#endif

#ifndef __ORDER_LITTLE_ENDIAN__
#define __ORDER_LITTLE_ENDIAN__         (2)
#endif

#ifndef __BYTE_ORDER__
#if defined(__BIG_ENDIAN__) || defined(__ARMEB__)  || \
    defined(__THUMBEB__) || defined(__AARCH64EB__) || \
    defined(__MIPSEB__) || defined(__TC32EB__)

#define __BYTE_ORDER__                  __ORDER_BIG_ENDIAN__

#elif defined(__LITTLE_ENDIAN__) || defined(__ARMEL__) || \
      defined(__THUMBEL__) || defined(__AARCH64EL__)   || \
      defined(__MIPSEL__) || defined(__TC32EL__)

#define __BYTE_ORDER__                  __ORDER_LITTLE_ENDIAN__

#else
#error "__BYTE_ORDER__ is not defined and cannot be automatically resolved"
#endif
#endif

#undef BUILD_ASSERT /* clear out common version */
/* C++11 has static_assert built in */
#if defined(__cplusplus) && (__cplusplus >= 201103L)
#define BUILD_ASSERT(EXPR, MSG...) static_assert(EXPR, "" MSG)

/*
 * GCC 4.6 and higher have the C11 _Static_assert built in and its
 * output is easier to understand than the common BUILD_ASSERT macros.
 * Don't use this in C++98 mode though (which we can hit, as
 * static_assert() is not available)
 */
#elif !defined(__cplusplus) && \
	((__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)) ||	\
	 (__STDC_VERSION__) >= 201100)
#define BUILD_ASSERT(EXPR, MSG...) _Static_assert(EXPR, "" MSG)
#else
#define BUILD_ASSERT(EXPR, MSG...)
#endif

#ifdef __cplusplus
#define ZRESTRICT __restrict
#else
#define ZRESTRICT restrict
#endif

#define ALIAS_OF(of) __attribute__((alias(#of)))

#define FUNC_ALIAS(real_func, new_alias, return_type) \
	return_type new_alias() ALIAS_OF(real_func)

#define CODE_UNREACHABLE __builtin_unreachable()

#define FUNC_NORETURN    __attribute__((__noreturn__))

	/* Unaligned access */
#define UNALIGNED_GET(p)						\
__extension__ ({							    \
	struct  __attribute__((__packed__)) {		\
		__typeof__(*(p)) __v;					\
	} *__p = (__typeof__(__p)) (p);				\
	__p->__v;							        \
})


#if __GNUC__ >= 7

/* Version of UNALIGNED_PUT() which issues a compiler_barrier() after
 * the store. It is required to workaround an apparent optimization
 * bug in GCC for ARM Cortex-M3 and higher targets, when multiple
 * byte, half-word and word stores (strb, strh, str instructions),
 * which support unaligned access, can be coalesced into store double
 * (strd) instruction, which doesn't support unaligned access (the
 * compilers in question do this optimization ignoring __packed__
 * attribute).
 */
#define UNALIGNED_PUT(v, p)                                     \
do {                                                            \
	struct __attribute__((__packed__)) {                        \
		__typeof__(*p) __v;                                     \
	} *__p = (__typeof__(__p)) (p);                             \
	__p->__v = (v);                                             \
	compiler_barrier();                                         \
} while (false)

#else

#define UNALIGNED_PUT(v, p)                                     \
do {                                                            \
	struct __attribute__((__packed__)) {                        \
		__typeof__(*p) __v;                                     \
	} *__p = (__typeof__(__p)) (p);                             \
	__p->__v = (v);                                             \
} while (false)

#endif

/* Double indirection to ensure section names are expanded before
 * stringification
 */
#define __GENERIC_SECTION(segment) __attribute__((section(STRINGIFY(segment))))
#define Z_GENERIC_SECTION(segment) __GENERIC_SECTION(segment)

#define __GENERIC_DOT_SECTION(segment) \
	__attribute__((section("." STRINGIFY(segment))))
#define Z_GENERIC_DOT_SECTION(segment) __GENERIC_DOT_SECTION(segment)

#define ___in_section(a, b, c) \
	__attribute__((section("." Z_STRINGIFY(a)			\
				"." Z_STRINGIFY(b)			\
				"." Z_STRINGIFY(c))))
#define __in_section(a, b, c) ___in_section(a, b, c)

#define __in_section_unique(seg) ___in_section(seg, __FILE__, __COUNTER__)

#define __in_section_unique_named(seg, name) \
	___in_section(seg, __FILE__, name)

#ifndef __fallthrough
#if __GNUC__ >= 7
#define __fallthrough        __attribute__((fallthrough))
#else
#define __fallthrough
#endif	/* __GNUC__ >= 7 */
#endif

#ifndef __packed
#define __packed        __attribute__((__packed__))
#endif

#ifndef __aligned
#define __aligned(x)	__attribute__((__aligned__(x)))
#endif

#ifndef FORCEINLINE
#if defined(__GNUC__) || defined(__clang__)
#define FORCEINLINE inline __attribute__((always_inline))
#else
#define FORCEINLINE
#endif /* __GNUC__ || __clang__ */
#endif

#define __may_alias     __attribute__((__may_alias__))

#define __printf_like(f, a)   __attribute__((format (printf, f, a)))

#define __used		    __attribute__((__used__))
#define __unused	    __attribute__((__unused__))
#define __maybe_unused	__attribute__((__unused__))

#ifndef __deprecated
#define __deprecated	__attribute__((deprecated))
#endif

#ifndef __attribute_const__
#define __attribute_const__ __attribute__((__const__))
#endif

#ifndef __must_check
#define __must_check __attribute__((warn_unused_result))
#endif

#define ARG_UNUSED(x) (void)(x)

#define likely(x)   (__builtin_expect((bool)!!(x), true) != 0L)
#define unlikely(x) (__builtin_expect((bool)!!(x), false) != 0L)
#define POPCOUNT(x) __builtin_popcount(x)

#ifndef __no_optimization
#define __no_optimization __attribute__((optimize("-O0")))
#endif

#ifndef __weak
#define __weak __attribute__((__weak__))
#endif

/* Builtins with availability that depend on the compiler version. */
#if __GNUC__ >= 5
#define HAS_BUILTIN___builtin_add_overflow 1
#define HAS_BUILTIN___builtin_sub_overflow 1
#define HAS_BUILTIN___builtin_mul_overflow 1
#define HAS_BUILTIN___builtin_div_overflow 1
#endif
#if __GNUC__ >= 4
#define HAS_BUILTIN___builtin_clz          1
#define HAS_BUILTIN___builtin_clzl         1
#define HAS_BUILTIN___builtin_clzll        1
#define HAS_BUILTIN___builtin_ctz          1
#define HAS_BUILTIN___builtin_ctzl         1
#define HAS_BUILTIN___builtin_ctzll        1
#endif

/*
 * Be *very* careful with these. You cannot filter out __DEPRECATED_MACRO with
 * -wno-deprecated, which has implications for -Werror.
 */

/*
 * Expands to nothing and generates a warning. Used like
 *
 *   #define FOO __WARN("Please use BAR instead") ...
 *
 * The warning points to the location where the macro is expanded.
 */
#define __WARN(msg) __WARN1(GCC warning msg)
#define __WARN1(s) _Pragma(#s)

/* Generic message */
#ifndef __DEPRECATED_MACRO
#define __DEPRECATED_MACRO __WARN("Macro is deprecated")
#endif

#define compiler_barrier() do {             \
	__asm__ __volatile__ ("" ::: "memory"); \
} while (false)

/** @brief Return larger value of two provided expressions.
 *
 * Macro ensures that expressions are evaluated only once.
 *
 * @note Macro has limited usage compared to the standard macro as it cannot be
 *	 used:
 *	 - to generate constant integer, e.g. __aligned(Z_MAX(4,5))
 *	 - static variable, e.g. array like static uint8_t array[Z_MAX(...)];
 */
#define Z_MAX(a, b) ({                                 \
		/* random suffix to avoid naming conflict */   \
		__typeof__(a) _value_a_ = (a);                 \
		__typeof__(b) _value_b_ = (b);                 \
		_value_a_ > _value_b_ ? _value_a_ : _value_b_; \
	})

/** @brief Return smaller value of two provided expressions.
 *
 * Macro ensures that expressions are evaluated only once. See @ref Z_MAX for
 * macro limitations.
 */
#define Z_MIN(a, b) ({ \
		/* random suffix to avoid naming conflict */   \
		__typeof__(a) _value_a_ = (a);                 \
		__typeof__(b) _value_b_ = (b);                 \
		_value_a_ < _value_b_ ? _value_a_ : _value_b_; \
	})

/** @brief Return a value clamped to a given range.
 *
 * Macro ensures that expressions are evaluated only once. See @ref Z_MAX for
 * macro limitations.
 */
#define Z_CLAMP(val, low, high) ({                                     \
		/* random suffix to avoid naming conflict */                   \
		__typeof__(val) _value_val_ = (val);                           \
		__typeof__(low) _value_low_ = (low);                           \
		__typeof__(high) _value_high_ = (high);                        \
		(_value_val_ < _value_low_)  ? _value_low_ :                   \
		(_value_val_ > _value_high_) ? _value_high_ :                  \
					       _value_val_;                                \
	})

/**
 * @brief Calculate power of two ceiling for some nonzero value
 *
 * @param x Nonzero unsigned long value
 * @return X rounded up to the next power of two
 */
#define Z_POW2_CEIL(x) \
	((x) <= 2UL ? (x) : (1UL << (8 * sizeof(long) - __builtin_clzl((x) - 1))))

/**
 * @brief Check whether or not a value is a power of 2
 *
 * @param x The value to check
 * @return true if x is a power of 2, false otherwise
 */
#define Z_IS_POW2(x) (((x) != 0) && (((x) & ((x)-1)) == 0))

/**
 * @brief Function attribute to disable stack protector.
 *
 * @note Only supported for GCC >= 11.0.0 or Clang >= 7.
 */
#if (TOOLCHAIN_GCC_VERSION >= 110000) || (TOOLCHAIN_CLANG_VERSION >= 70000)
#define FUNC_NO_STACK_PROTECTOR __attribute__((no_stack_protector))
#else
#define FUNC_NO_STACK_PROTECTOR
#endif

/* Abstract use of extern keyword for compatibility between C and C++ */
#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif

#define Z_STRINGIFY(x) #x
#define STRINGIFY(s) Z_STRINGIFY(s)

/* concatenate the values of the arguments into one */
#define _DO_CONCAT(x, y) x ## y
#define _CONCAT(x, y) _DO_CONCAT(x, y)

/*
 * This is meant to be used in conjunction with __in_section() and similar
 * where scattered structure instances are concatenated together by the linker
 * and walked by the code at run time just like a contiguous array of such
 * structures.
 *
 * Assemblers and linkers may insert alignment padding by default whose
 * size is larger than the natural alignment for those structures when
 * gathering various section segments together, messing up the array walk.
 * To prevent this, we need to provide an explicit alignment not to rely
 * on the default that might just work by luck.
 *
 * Alignment statements in  linker scripts are not sufficient as
 * the assembler may add padding by itself to each segment when switching
 * between sections within the same file even if it merges many such segments
 * into a single section in the end.
 */
#define Z_DECL_ALIGN(type) __aligned(__alignof(type)) type

/* Check if a pointer is aligned for against a specific byte boundary  */
#define IS_PTR_ALIGNED_BYTES(ptr, bytes) ((((uintptr_t)ptr) % bytes) == 0)

/* Check if a pointer is aligned enough for a particular data type. */
#define IS_PTR_ALIGNED(ptr, type) IS_PTR_ALIGNED_BYTES(ptr, __alignof(type))

/** @brief Tag a symbol (e.g. function) to be kept in the binary even though it is not used.
 *
 * It prevents symbol from being removed by the linker garbage collector. It
 * is achieved by adding a pointer to that symbol to the kept memory section.
 *
 * @param symbol Symbol to keep.
 */
#define LINKER_KEEP(symbol)                        \
	static const void * const symbol##_ptr  __used \
	__attribute__((__section__(".symbol_to_keep"))) = (void *)&symbol

#endif /* !INC_CORE_TOOLCHAIN_H_ */
