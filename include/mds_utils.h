/**
 * Copyright (c) [2022] [pchom]
 * [MDS] is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 **/
#ifndef __MDS_UTILS_H__
#define __MDS_UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Utils ------------------------------------------------------------------- */
#define __ARGUMENT_PASSBY(x) x
#define __ARGUMENT_STR(x)    #x
#define MDS_ARGUMENT_STR(x)  __ARGUMENT_STR(x)

#define __ARGUMENT_CAT(x, ...)   x##__VA_ARGS__
#define MDS_ARGUMENT_CAT(x, ...) __ARGUMENT_CAT(x, __VA_ARGS__)

#define __DEBRACKET(...)                        __VA_ARGS__
#define __ARGUMENT2_DEBRACKET(ignore, val, ...) __DEBRACKET val

#define __ARGUMNET_NUMS(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,     \
                        _16, ARG, ...)                                                            \
    ARG

#define MDS_ARGUMENT_NUMS(...)                                                                    \
    __ARGUMNET_NUMS(0, ##__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define __ARGUMENT_GET_1(_0, ...)                                                               _0
#define __ARGUMENT_GET_2(_0, _1, ...)                                                           _1
#define __ARGUMENT_GET_3(_0, _1, _2, ...)                                                       _2
#define __ARGUMENT_GET_4(_0, _1, _2, _3, ...)                                                   _3
#define __ARGUMENT_GET_5(_0, _1, _2, _3, _4, ...)                                               _4
#define __ARGUMENT_GET_6(_0, _1, _2, _3, _4, _5, ...)                                           _5
#define __ARGUMENT_GET_7(_0, _1, _2, _3, _4, _5, _6, ...)                                       _6
#define __ARGUMENT_GET_8(_0, _1, _2, _3, _4, _5, _6, _7, ...)                                   _7
#define __ARGUMENT_GET_9(_0, _1, _2, _3, _4, _5, _6, _7, _8, ...)                               _8
#define __ARGUMENT_GET_10(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, ...)                          _9
#define __ARGUMENT_GET_11(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, ...)                     _10
#define __ARGUMENT_GET_12(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, ...)                _11
#define __ARGUMENT_GET_13(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, ...)           _12
#define __ARGUMENT_GET_14(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, ...)      _13
#define __ARGUMENT_GET_15(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, ...) _14
#define __ARGUMENT_GET_16(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15,   \
                          ...)                                                                    \
    _15

#define MDS_ARGUMENT_GET_N(N, ...) __ARGUMENT_GET_##N(__VA_ARGS__)

#define __ARGUMENT_FOREACH_0(_call, _sep, _arg, ...) _arg

#define __ARGUMENT_FOREACH_1(_call, _sep, _arg, _elem)                                            \
    _call(0, _elem, _arg) __DEBRACKET _sep __ARGUMENT_FOREACH_0(_call, _sep, _arg)

#define __ARGUMENT_FOREACH_2(_call, _sep, _arg, _elem, ...)                                       \
    _call(1, _elem, _arg) __DEBRACKET _sep __ARGUMENT_FOREACH_1(_call, _sep, _arg, ##__VA_ARGS__)

#define __ARGUMENT_FOREACH_3(_call, _sep, _arg, _elem, ...)                                       \
    _call(2, _elem, _arg) __DEBRACKET _sep __ARGUMENT_FOREACH_2(_call, _sep, _arg, ##__VA_ARGS__)

#define __ARGUMENT_FOREACH_4(_call, _sep, _arg, _elem, ...)                                       \
    _call(3, _elem, _arg) __DEBRACKET _sep __ARGUMENT_FOREACH_3(_call, _sep, _arg, ##__VA_ARGS__)

#define __ARGUMENT_FOREACH_5(_call, _sep, _arg, _elem, ...)                                       \
    _call(4, _elem, _arg) __DEBRACKET _sep __ARGUMENT_FOREACH_4(_call, _sep, _arg, ##__VA_ARGS__)

#define __ARGUMENT_FOREACH_6(_call, _sep, _arg, _elem, ...)                                       \
    _call(5, _elem, _arg) __DEBRACKET _sep __ARGUMENT_FOREACH_5(_call, _sep, _arg, ##__VA_ARGS__)

#define __ARGUMENT_FOREACH_7(_call, _sep, _arg, _elem, ...)                                       \
    _call(6, _elem, _arg) __DEBRACKET _sep __ARGUMENT_FOREACH_6(_call, _sep, _arg, ##__VA_ARGS__)

#define __ARGUMENT_FOREACH_8(_call, _sep, _arg, _elem, ...)                                       \
    _call(7, _elem, _arg) __DEBRACKET _sep __ARGUMENT_FOREACH_7(_call, _sep, _arg, ##__VA_ARGS__)

#define __ARGUMENT_FOREACH_9(_call, _sep, _arg, _elem, ...)                                       \
    _call(8, _elem, _arg) __DEBRACKET _sep __ARGUMENT_FOREACH_8(_call, _sep, _arg, ##__VA_ARGS__)

#define __ARGUMENT_FOREACH_10(_call, _sep, _arg, _elem, ...)                                      \
    _call(9, _elem, _arg) __DEBRACKET _sep __ARGUMENT_FOREACH_9(_call, _sep, _arg, ##__VA_ARGS__)

#define __ARGUMENT_FOREACH_11(_call, _sep, _arg, _elem, ...)                                      \
    _call(10, _elem, _arg) __DEBRACKET _sep __ARGUMENT_FOREACH_10(_call, _sep, _arg, ##__VA_ARGS__)

#define __ARGUMENT_FOREACH_12(_call, _sep, _arg, _elem, ...)                                      \
    _call(11, _elem, _arg) __DEBRACKET _sep __ARGUMENT_FOREACH_11(_call, _sep, _arg, ##__VA_ARGS__)

#define __ARGUMENT_FOREACH_13(_call, _sep, _arg, _elem, ...)                                      \
    _call(12, _elem, _arg) __DEBRACKET _sep __ARGUMENT_FOREACH_12(_call, _sep, _arg, ##__VA_ARGS__)

#define __ARGUMENT_FOREACH_14(_call, _sep, _arg, _elem, ...)                                      \
    _call(13, _elem, _arg) __DEBRACKET _sep __ARGUMENT_FOREACH_13(_call, _sep, _arg, ##__VA_ARGS__)

#define __ARGUMENT_FOREACH_15(_call, _sep, _arg, _elem, ...)                                      \
    _call(14, _elem, _arg) __DEBRACKET _sep __ARGUMENT_FOREACH_14(_call, _sep, _arg, ##__VA_ARGS__)

#define MDS_ARGUMENT_FOREACH_N(_call, _sep, _arg, ...)                                            \
    MDS_ARGUMENT_CAT(__ARGUMENT_FOREACH_, MDS_ARGUMENT_NUMS(__VA_ARGS__))(_call, _sep, _arg,      \
                                                                          __VA_ARGS__)

#define __CONDITION_CODE(_args, _if_code, _else_code)                                             \
    __ARGUMENT2_DEBRACKET(_args _if_code, _else_code)

// if (defined(_flag) && (_flag != 0)) { _if_code_1 } else { _else_code }
#define __COND_CODE_1(_flag, _if_1_code, _else_code)                                              \
    __CONDITION_CODE(__COND_CODE_RSV1_##_flag, _if_1_code, _else_code)
#define __COND_CODE_RSV1_1                             __COND_CODE_RSV1_,
#define MDS_COND_CODE_1(_flag, _if_1_code, _else_code) __COND_CODE_1(_flag, _if_1_code, _else_code)

// if (defined(_flag) && (_flag == 0)) { _if_code_0 } else { _else_code }
#define __COND_CODE_0(_flag, _if_0_code, _else_code)                                              \
    __CONDITION_CODE(__COND_CODE_RSV0_##_flag, _if_0_code, _else_code)
#define __COND_CODE_RSV0_0                             __COND_CODE_RSV0_,
#define MDS_COND_CODE_0(_flag, _if_0_code, _else_code) __COND_CODE_0(_flag, _if_0_code, _else_code)

#ifdef __cplusplus
}
#endif

#endif /* __MDS_UTILS_H__ */
