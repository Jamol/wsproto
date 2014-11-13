/* Copyright (c) 2014, Fengping Bao <jamol@live.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef __WSPROTO_H__
#define __WSPROTO_H__
#include <stdint.h>

typedef struct ws_ctx_t* ws_proto_t;

enum{
    WS_ERROR_NOERR,
    WS_ERROR_NEED_MORE_DATA,
    WS_ERROR_INVALID_FRAME,
    WS_ERROR_INVALID_LENGTH,
    WS_ERROR_CLOSED
};
typedef enum{
    WS_FRAME_TYPE_TEXT  = 1,
    WS_FRAME_TYPE_BINARY
}ws_frame_type_t;

ws_proto_t ws_create();
void ws_destroy(ws_proto_t ws);
void ws_free(void* buf);
int ws_encode(ws_frame_type_t frame_type, const uint8_t *input, uint32_t input_len, uint8_t **output, uint32_t *output_len);
int ws_decode(ws_proto_t ws, const uint8_t *input, uint32_t input_len, uint8_t **output, uint32_t *output_len, uint32_t *handled_len);

#endif
