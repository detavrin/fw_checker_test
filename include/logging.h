/**
 * @file   logging.h
 * 
 * @brief  Заголовочный файл содержащий макроопределения, позволяющие осуществлять вывод 
 *         форматированных сообщений логов. 
 * 
 * Данный файл экспортирует свой функционал в единице трансляции путем включения его через 
 * директиву #include. Подходящим местом для включения будут файлы с расширениями .c/.cpp. 
 * Не подключайте данный файл в разделяемые заголовочные файлы во избежании засорения 
 * глобального пространства имен.
 * Логирование реализовано с использованием концепции модулей. Модуль - это некоторая независимая
 * программная единица, обладающая именем и набором функциональных особенностей реализующих какие-
 * либо задачи. Каждый модуль для возможности использования функций логирования должен определять 
 * ряд обязательных препроцессорных символов, среди которых: 
 * 
 * LOG_MODULE_IS_ENABLED - включает/выключает вывод сообщений для текущего модуля.
 * Возможные значения для данного символа: 1 - вывод сообщений включен, 0 - вывод сообщений выключен.
 *
 * LOG_MODULE_PRINTABLE_NAME - короткое строковое представление имени модуля, которое будет фигурировать в
 * составе всех лог-сообщений модуля. Может быть опущено (в этом случае имя модуля просто не выводится
 * вместе с остальными полями сообщения).
 *
 * LOG_MODULE_LOG_LEVEL - текущий уровень вывода лог-сообщений. Чем выше числовое значение присвоено этому
 * символу, тем более детальную информацию модуль будет выводить. Возможные значения для данного символа:
 *     0 (LOG_LEVEL_NONE) - вывод логов выключен,
 *     1 (LOG_LEVEL_ERR) - выводятся только сообщения об ошибках,
 *     2 (LOG_LEVEL_WRN) - выводятся сообщения об ошибках и предупреждения,
 *     3 (LOG_LEVEL_INF) - выводятся сообщения об ошибках, предупреждения и информационные сообщения,
 *     4 (LOG_LEVEL_INF) - выводятся все вышеперечисленные сообщения + отладочные данные.
 *
 *  Формат выводимых лог-сообщений:
 *  "[TS] | [TG] | LVL | [FN] | BODY | ENDL"   ,где
 *
 *  TS - опциональное поле вывода времени. По умолчанию вывод данного поля выключен. Чтобы включить
 *  вывод меток времени включающий модуль должен определить символ LOG_MODULE_IS_TIMESTAMP_ENABLED
 *  и установить для него значание 1. По умолчанию в качестве источника меток времени выступает
 *  системный миллисекундный таймер. Включающий модуль может переопределить источник системного времени
 *  и строку форматирования для этого значения, для этого предусмотрены 2 символа:
 *    LOG_MODULE_TIMESTAMP_CUSTOM_FUNC - пользовательская функция, которая возвращвет системное время,
 *    LOG_MODULE_TIMESTAMP_CUSTOM_FORMAT - строка форматирования вывода меток времени
 *
 *  Например имеется пользовательская функция, возвращающая строку текущего времени:
 *     const char* GetCustomTimestamp(void);
 *  В таком случае можно определить символы:
 *  #define LOG_MODULE_TIMESTAMP_CUSTOM_FUNC   GetCustomTimestamp
 *  #define LOG_MODULE_TIMESTAMP_CUSTOM_FORMAT "[%s]"
 *
 *  TG - опциональное поле, определяющее имя модуля
 *
 *  LVL - уровень лог-сообщения: ERR, WRN, INF, DBG
 *
 *  FN - опциональное поле, выводящее имя текущей функции и номер строки, на которой была вызвана
 *  функция логирования
 *
 *  BODY - содержимое лог-сообщения, определяется форматной строкой и преданными аргументами.
 *
 *  ENDL - завершающая последовательность символов вставляемая в конец каждого сообщения. Включающий
 *  модуль может переопределить символы по умолчанию (CR,LF), используя символ
 *  LOG_MODULE_ENDL_CUSTOM.
 */

#ifndef INC_LOGGING_H_
#define INC_LOGGING_H_

#include <stdio.h>

#include "core/util.h"

/**
 * @def   LOG_PRINTF_DEFAULT_FUNC
 *        Функция по умолчанию для вывода форматированных сообщений логов. Включающий модуль
 *        может переопределить данный define изменяя способ вывода сообщений. Переопределяемая
 *        функция должна иметь stdio::printf-like сигнатуру и осуществлять статическую проверку
 *        соответствия строки форматирования с переданными аргументами, используя, например,
 *        механизм атрибутов функций GNU C. Подробнее про атрибуты функций в GCC:
 *        https://gcc.gnu.org/onlinedocs/gcc-3.2/gcc/Function-Attributes.html
 */
#ifndef LOG_PRINTF_DEFAULT_FUNC
#define LOG_PRINTF_DEFAULT_FUNC printf
#endif

#define LOG_PRINTF       LOG_PRINTF_DEFAULT_FUNC

/* Если определена кастомная завершающая последовательность символов */
#ifdef LOG_MODULE_ENDL_CUSTOM
#define ENDL             LOG_MODULE_ENDL_CUSTOM
#else
#define ENDL             "\r\n"
#endif /* LOG_MODULE_ENDL_CUSTOM */


/* Имя модуля было задано - выводить его всякий раз как строку */
#ifdef LOG_MODULE_PRINTABLE_NAME
#define TGP              LOG_MODULE_PRINTABLE_NAME
#define FTGP             " %12s "
#else
#define LOG_MODULE_PRINTABLE_NAME ""
#define TGP
#define FTGP
#endif /* !LOG_MODULE_PRINTABLE_NAME */

/* Включение вывода текущего системного времени */
#if LOG_MODULE_IS_TIMESTAMP_ENABLED

#define TS  HAL_GetTick()
#define FTS "%10lu |"

/* Если предоставлена кастомная функция получения времени */
#ifdef LOG_MODULE_TIMESTAMP_CUSTOM_FUNC
#undef  TS
#define TS LOG_MODULE_TIMESTAMP_CUSTOM_FUNC
#endif /* LOG_MODULE_TIMESTAMP_CUSTOM_FUNC */

/* Если предоставлена кастомная строка форматирования текущего времени */
#ifdef LOG_MODULE_TIMESTAMP_CUSTOM_FORMAT
#undef  FTS
#define FTS LOG_MODULE_TIMESTAMP_CUSTOM_FORMAT
#endif /* LOG_MODULE_TIMESTAMP_CUSTOM_FORMAT */

#else
#define TS  ""
#define FTS "%s           |"
#endif /* LOG_MODULE_IS_TIMESTAMP_ENABLED */

/* Включение вывода вызывающей функции и номера линни в файле */
#if LOG_MODULE_IS_FUNC_NAME_ENABLED
#define FN  __func__, __LINE__
#define FFN "  %s(%d)"
#define HDRE ": "
#else
#define FN  ""
#define FFN "%s"
#define HDRE " "

#endif /* LOG_MODULE_IS_FUNC_NAME_ENABLED */

#undef  LOG_DBG
#undef  LOG_INF
#undef  LOG_WRN
#undef  LOG_ERROR
#undef  LOG_HEX_ARRAY_DBG
#undef  LOG_HEX_ARRAY_INF
#undef  LOG_HEX_ARRAY_WRN
#undef  LOG_HEX_ARRAY_ERR

#define LOG_DBG(...)      (void)0
#define LOG_INF(...)      (void)0
#define LOG_WRN(...)      (void)0
#define LOG_ERROR(...)    (void)0
#define LOG_DBG_IF(...)   (void)0
#define LOG_INF_IF(...)   (void)0
#define LOG_WRN_IF(...)   (void)0
#define LOG_ERROR_IF(...) (void)0

#define LOG_HEX_ARRAY_DBG(...)   (void)0
#define LOG_HEX_ARRAY_INF(...)   (void)0
#define LOG_HEX_ARRAY_WRN(...)   (void)0
#define LOG_HEX_ARRAY_ERROR(...) (void)0

#if LOG_MODULE_IS_ENABLED

#ifndef LOG_MODULE_LOG_LEVEL
#define LOG_MODULE_LOG_LEVEL 0U
#endif /* !LOG_MODULE_LOG_LEVEL */

#if LOG_MODULE_LOG_LEVEL >=  1U
#undef  LOG_ERROR
#undef  LOG_ERROR_IF
#undef  LOG_HEX_ARRAY_ERROR
#define LOG_ERROR(format, ...)    LOG_PRINTF(FTS FTGP "(ERR):" FFN HDRE format ENDL, TS, TGP, FN, ##__VA_ARGS__)
#define LOG_ERROR_IF(cond, format, ...) \
	                            do {if((cond)) LOG_ERROR(format, ##__VA_ARGS__);} while(0)

/**
 * @def   LOG_HEX_ARRAY_ERROR
 * @brief Вывод массива байт уровня ERR.
 *
 * @param pre  Форматная строка выводимая до строки массива.
 * @param buf  Указатель на первый элемент массива последовательности байт.
 * @param sz   Размер массива в байтах.
 * @param ...  Аргументы форматной строки.
 */
#define LOG_HEX_ARRAY_ERROR(pre, buf, sz, ...)                   \
        do {                                                   \
            LOG_ERROR("HEX ARRAY(%u): " pre, sz, ##__VA_ARGS__); \
            log_hexdump_buffer((buf), (sz));                   \
        } while(0);

#endif /* LOG_LEVEL_ERR */

#if LOG_MODULE_LOG_LEVEL >=  2U
#undef  LOG_WRN
#undef  LOG_WRN_IF
#undef  LOG_HEX_ARRAY_WRN
#define LOG_WRN(format, ...)    LOG_PRINTF(FTS FTGP "(WRN):" FFN HDRE format ENDL, TS, TGP, FN, ##__VA_ARGS__)
#define LOG_WRN_IF(cond, format, ...) \
	                            do {if((cond)) LOG_WRN(format, ##__VA_ARGS__);} while(0)

/**
 * @def   LOG_HEX_ARRAY_WRN
 * @brief Вывод массива байт уровня WRN.
 *
 * @param pre  Форматная строка выводимая до строки массива.
 * @param buf  Указатель на первый элемент массива последовательности байт.
 * @param sz   Размер массива в байтах.
 * @param ...  Аргументы форматной строки.
 */
#define LOG_HEX_ARRAY_WRN(pre, buf, sz, ...)                   \
        do {                                                   \
            LOG_INF("HEX ARRAY(%u): " pre, sz, ##__VA_ARGS__); \
            log_hexdump_buffer((buf), (sz));                   \
        } while(0);

#endif /* LOG_LEVEL_WRN */

#if LOG_MODULE_LOG_LEVEL >=  3U
#undef  LOG_INF
#undef  LOG_INF_IF
#undef  LOG_HEX_ARRAY_INF
#define LOG_INF(format, ...)    LOG_PRINTF(FTS FTGP "(INF):" FFN HDRE format ENDL, TS, TGP, FN, ##__VA_ARGS__)
#define LOG_INF_IF(cond, format, ...) \
	                            do {if((cond)) LOG_INF(format, ##__VA_ARGS__);} while(0)

/**
 * @def   LOG_HEX_ARRAY_INF
 * @brief Вывод массива байт уровня INF.
 *
 * @param pre  Форматная строка выводимая до строки массива.
 * @param buf  Указатель на первый элемент массива последовательности байт.
 * @param sz   Размер массива в байтах.
 * @param ...  Аргументы форматной строки.
 */
#define LOG_HEX_ARRAY_INF(pre, buf, sz, ...)                   \
        do {                                                   \
            LOG_INF("HEX ARRAY(%u): " pre, sz, ##__VA_ARGS__); \
            log_hexdump_buffer((buf), (sz));                   \
        } while(0);

#endif /* LOG_LEVEL_INF */

#if LOG_MODULE_LOG_LEVEL >=  4U
#undef  LOG_DBG
#undef  LOG_DBG_IF
#undef  LOG_HEX_ARRAY_DBG
#define LOG_DBG(format, ...)    LOG_PRINTF(FTS FTGP "(DBG):" FFN HDRE format ENDL, TS, TGP, FN, ##__VA_ARGS__)
#define LOG_DBG_IF(cond, format, ...) \
	                            do {if((cond)) LOG_DBG(format, ##__VA_ARGS__);} while(0)

/**
 * @def   LOG_HEX_ARRAY_DBG
 * @brief Вывод массива байт уровня DBG.
 *
 * @param pre  Форматная строка выводимая до строки массива.
 * @param buf  Указатель на первый элемент массива последовательности байт.
 * @param sz   Размер массива в байтах.
 * @param ...  Аргументы форматной строки.
 */
#define LOG_HEX_ARRAY_DBG(pre, buf, sz, ...)                   \
        do {                                                   \
            LOG_DBG("HEX ARRAY(%u): " pre, sz, ##__VA_ARGS__); \
            log_hexdump_buffer((buf), (sz));                   \
        } while(0);

#endif /* LOG_LEVEL_DBG */

#endif /* LOG_MODULE_IS_ENABLED */

/**
 * @def   LOG_HEX_ARRAY
 * @brief Вывод массива байт уровня DBG.
 *
 * @param pre  Форматная строка выводимая до строки массива.
 * @param buf  Указатель на первый элемент массива последовательности байт.
 * @param sz   Размер массива в байтах.
 * @param ...  Аргументы форматной строки.
 */
#define LOG_HEX_ARRAY(pre, buf, sz, ...) LOG_HEX_ARRAY_DBG(pre, buf, sz, ##__VA_ARGS__)

/**
 * @def   LOG_ERR_CODE
 * @brief Вывод сообщения об ошибке, с уточнением кода и значения.
 *
 * @param code  Целочисленный код ошибки.
 * @param code  Целочисленное значение ошибки.
 */
#define LOG_ERR_CODE(code, value) LOG_ERROR("Code: %d, Value: %d", code, value)

/**
 *  @def   LOG_VAL
 *  @brief Отправить на сервер лог с кодом и целочисленным значением
 *
 *  @param code  Код лога (см. socketProtocol::LogTypes)
 *  @param value Значение лога (см. socketProtocol::LogCodes)
 */
#define LOG_VAL(code, value) do {                                                                       \
            LOG_DBG("Code: %d, Value: %d", code, value);                                                \
            log_send(code, LOG_MODULE_PRINTABLE_NAME, __FUNCTION__, __LINE__, value); } while(0) \

/**
 *  @brief  Вывести на экран побайтовый дамп буфера в шестнадцатеричной форме.
 *
 *  @param buffer  Начало области памяти для которого выводится дамп.
 *  @param size    Размер буфера в байтах.
 */
void log_hexdump_buffer(const unsigned char* buffer, size_t size);

#endif /* INC_LOGGING_H_ */
