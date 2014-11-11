/*
 *  wsprotp.h
 *  wstest
 *
 *  Created by Fengping Bao <jamol@live.com> on 11/10/14.
 *  Copyright 2014. All rights reserved.
 *
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
