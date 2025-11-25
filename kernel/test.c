#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

struct MemError {
} G_MEM_ERROR[] = {};

#define MEM_ERROR_DEFINE()

#define RESULT_TYPE(TypeOk, TypeErr)                                                              \
    struct {                                                                                      \
        TypeOk ok;                                                                                \
        TypeErr err;                                                                              \
    }

#define RESULT_ERR(erval) {.err = erval}
#define RESULT_OK(okval)  {.ok = okval, .err = NULL}

#define RESULT_UNWRAP_ERR(res)         ((res).err)
#define RESULT_UNWRAP_OK(res)          (((res).err) ? (abort(), (res).ok) : (res).ok)
#define RESULT_UNWRAP_OR(res, default) (((res).err) ? (default) : ((res).ok))

#define OPTION_TYPE(TypeVal) RESULT_TYPE(TypeVal, const char *const)

#define OPTION_NONE()      RESULT_ERR("None")
#define OPTION_SOME(value) RESULT_OK(value)

#define OPTION_UNWRAP(opt)             RESULT_UNWRAP_OK(opt)
#define OPTION_UNWRAP_OR(opt, default) RESULT_UNWRAP_OR(opt, default)

typedef OPTION_TYPE(int32_t) OptionI32;
typedef RESULT_TYPE(int32_t, const char *const) ResultI32;

OptionI32 test(void)
{
    OptionI32 opt = OPTION_NONE();

    return (opt);
}

ResultI32 testRes(int x)
{
    uint8_t array[10];
    ResultI32 res = RESULT_ERR("Error");


    if (x > 0) {
        array[x] = 0;
    }

    return (res);
}

int _read_r()
{}


int main(void)
{
    printf("%d\n", OPTION_UNWRAP(test()));
}
