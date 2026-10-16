/*
 * test_config.c — Unit Tests fuer den JSON-Parser in config.c
 *
 * Testet:
 *   - json_parse_int mit Integer-Overflow-Check
 *   - json_parse_string mit Backslash-Escape-Behandlung
 *
 * Die JSON-Parser-Funktionen sind static in config.c. Um sie testen
 * zu koennen, ohne die gesamten ESP-IDF-Abhaengigkeiten einzubinden,
 * werden die Funktionen hier als Inline-Kopie mitgeprueft.
 *
 * Die Tests verwenden das Unity-Test-Framework und laufen auf dem
 * Host-PC mit GCC.
 */

#include "unity.h"

#include <string.h>
#include <limits.h>

/* ============================================================
 * Kopie der JSON-Parser-Funktionen aus config.c.
 * Diese werden hier direkt getestet, da sie im Original static sind.
 * Bei Aenderungen in config.c muessen diese Funktionen synchron
 * gehalten werden.
 * ============================================================ */

static const char *json_find_key(const char *json, const char *key)
{
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *p = strstr(json, pattern);
    if (!p) return NULL;
    p += strlen(pattern);
    while (*p && (*p == ' ' || *p == '\t' || *p == ':')) p++;
    return p;
}

static int json_parse_int(const char *json, const char *key, int *out)
{
    const char *p = json_find_key(json, key);
    if (!p) return -1;
    int sign = 1;
    if (*p == '-') { sign = -1; p++; }
    if (*p < '0' || *p > '9') return -1;
    int val = 0;
    while (*p >= '0' && *p <= '9') {
        if (val > (INT_MAX - (*p - '0')) / 10) {
            return -1;
        }
        val = val * 10 + (*p - '0');
        p++;
    }
    *out = val * sign;
    return 0;
}

static int json_parse_string(const char *json, const char *key,
                              char *out, size_t max_len)
{
    const char *p = json_find_key(json, key);
    if (!p || *p != '"') return -1;
    p++;
    size_t i = 0;
    while (*p && *p != '"' && i < max_len - 1) {
        if (*p == '\\') {
            p++;
            if (!*p) return -1;
            switch (*p) {
            case 'n':  out[i++] = '\n'; break;
            case 't':  out[i++] = '\t'; break;
            case 'r':  out[i++] = '\r'; break;
            case '"':  out[i++] = '"';  break;
            case '\\': out[i++] = '\\'; break;
            case '/':  out[i++] = '/';  break;
            case 'b':  out[i++] = '\b'; break;
            case 'f':  out[i++] = '\f'; break;
            default:   out[i++] = *p;   break;
            }
            p++;
        } else {
            out[i++] = *p++;
        }
    }
    out[i] = '\0';
    return 0;
}

/* ============================================================
 * Tests: json_parse_int
 * ============================================================ */

void test_json_parse_int_normal(void)
{
    int val = 0;
    int ret = json_parse_int("{\"brightness\": 80}", "brightness", &val);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL(80, val);
}

void test_json_parse_int_negative(void)
{
    int val = 0;
    int ret = json_parse_int("{\"offset\": -42}", "offset", &val);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL(-42, val);
}

void test_json_parse_int_overflow(void)
{
    /* 21474836478 = INT_MAX (2147483647) + '8' digit -> overflow */
    int val = 999;
    int ret = json_parse_int("{\"val\": 21474836478}", "val", &val);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(999, val);
}

void test_json_parse_int_max(void)
{
    char json[64];
    snprintf(json, sizeof(json), "{\"val\": %d}", INT_MAX);

    int val = 0;
    int ret = json_parse_int(json, "val", &val);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL(INT_MAX, val);
}

void test_json_parse_int_max_plus_one(void)
{
    /* INT_MAX + 1 = 2147483648 */
    int val = 0;
    int ret = json_parse_int("{\"val\": 2147483648}", "val", &val);
    TEST_ASSERT_EQUAL(-1, ret);
}

void test_json_parse_int_missing_key(void)
{
    int val = 42;
    int ret = json_parse_int("{\"other\": 5}", "missing", &val);
    TEST_ASSERT_EQUAL(-1, ret);
    TEST_ASSERT_EQUAL(42, val);
}

void test_json_parse_int_zero(void)
{
    int val = 999;
    int ret = json_parse_int("{\"val\": 0}", "val", &val);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL(0, val);
}

/* ============================================================
 * Tests: json_parse_string
 * ============================================================ */

void test_json_parse_string_escapes(void)
{
    char out[64];

    /* \n -> newline */
    int ret = json_parse_string("{\"name\": \"line1\\nline2\"}", "name", out, sizeof(out));
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_STRING("line1\nline2", out);

    /* \\ -> backslash */
    ret = json_parse_string("{\"path\": \"C:\\\\dir\"}", "path", out, sizeof(out));
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_STRING("C:\\dir", out);

    /* \" -> quote */
    ret = json_parse_string("{\"q\": \"say \\\"hi\\\"\"}", "q", out, sizeof(out));
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_STRING("say \"hi\"", out);

    /* \t -> tab */
    ret = json_parse_string("{\"t\": \"a\\tb\"}", "t", out, sizeof(out));
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_STRING("a\tb", out);

    /* \r -> carriage return */
    ret = json_parse_string("{\"r\": \"a\\rb\"}", "r", out, sizeof(out));
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_STRING("a\rb", out);
}

void test_json_parse_string_plain(void)
{
    char out[64];
    int ret = json_parse_string("{\"name\": \"BC-Transalp\"}", "name", out, sizeof(out));
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_STRING("BC-Transalp", out);
}

void test_json_parse_string_empty(void)
{
    char out[64];
    int ret = json_parse_string("{\"name\": \"\"}", "name", out, sizeof(out));
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_STRING("", out);
}

void test_json_parse_string_missing_key(void)
{
    char out[64];
    int ret = json_parse_string("{\"other\": \"x\"}", "missing", out, sizeof(out));
    TEST_ASSERT_EQUAL(-1, ret);
}

void test_json_parse_string_truncate(void)
{
    char out[4];
    int ret = json_parse_string("{\"name\": \"hello\"}", "name", out, sizeof(out));
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_STRING("hel", out);
}

/* ============================================================
 * Test Runner
 * ============================================================ */
void setUp(void) {}
void tearDown(void) {}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_json_parse_int_normal);
    RUN_TEST(test_json_parse_int_negative);
    RUN_TEST(test_json_parse_int_overflow);
    RUN_TEST(test_json_parse_int_max);
    RUN_TEST(test_json_parse_int_max_plus_one);
    RUN_TEST(test_json_parse_int_missing_key);
    RUN_TEST(test_json_parse_int_zero);
    RUN_TEST(test_json_parse_string_escapes);
    RUN_TEST(test_json_parse_string_plain);
    RUN_TEST(test_json_parse_string_empty);
    RUN_TEST(test_json_parse_string_missing_key);
    RUN_TEST(test_json_parse_string_truncate);

    return UNITY_END();
}
