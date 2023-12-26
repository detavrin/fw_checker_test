#ifndef INC_CORE_ASSERT_H_
#define INC_CORE_ASSERT_H_

#include "util.h"
#include "toolchain.h"

#ifndef NDEBUG
#define ASSERT_ON
#endif /* !NDEBUG */

EXTERN_C void assert_print(const char* fmt, ...);

#define __ASSERT_PRINT(fmt, ...)    assert_print(fmt, ##__VA_ARGS__)

#define __ASSERT_MSG_INFO(fmt, ...) __ASSERT_PRINT("\t" fmt "\n", ##__VA_ARGS__)

#define __ASSERT_LOC(test)                                  \
			__ASSERT_PRINT("ASSERTION FAIL [%s] @ %s:%d\n", \
			STRINGIFY(test),                                \
			__FILE__, __LINE__)

#ifdef ASSERT_ON

EXTERN_C void assert_post_action(const char*, unsigned int);

#define __ASSERT_POST_ACTION() assert_post_action(__FILE__, __LINE__)

#define __ASSERT_UNREACHABLE __builtin_unreachable()

#define ASSERT_NO_MSG(test)                                 \
	do {                                                    \
		if (!(test)) {                                      \
			__ASSERT_LOC(test);                             \
			__ASSERT_POST_ACTION();                         \
			__ASSERT_UNREACHABLE;                           \
		}                                                   \
	} while (false)

#define ASSERT(test, fmt, ...)                              \
	do {                                                    \
		if (!(test)) {                                      \
			__ASSERT_LOC(test);                             \
			__ASSERT_MSG_INFO(fmt, ##__VA_ARGS__);          \
			__ASSERT_POST_ACTION();                         \
			__ASSERT_UNREACHABLE;                           \
		}                                                   \
	} while (false)

#define ASSERT_EVAL(test, expr, fmt, ...)                   \
		ASSERT(test, fmt, ##__VA_ARGS__)

#define ASSERT_EVAL_NO_MSG(test, expr)                      \
		ASSERT_NO_MSG(test)

#else
#define ASSERT(test, fmt, ...) { }
#define ASSERT_EVAL(test, expr, fmt, ...)    do { if(!(test)) { expr; }} while(0)
#define ASSERT_EVAL_NO_MSG(test, expr)       ASSERT_EVAL(test, expr, "")
#define ASSERT_NO_MSG(test) { }
#define ASSERT_POST_ACTION() { }
#endif /* ASSERT_ON */

#define CHECK(test, expr) ASSERT_EVAL_NO_MSG(test, expr)

#endif /* !INC_CORE_ASSERT_H_ */
