/*
 *  wsprotp.c
 *  wstest
 *
 *  Created by Fengping Bao <jamol@live.com> on 11/10/14.
 *  Copyright 2014. All rights reserved.
 *
 */
#include <stdlib.h>
#include "wsproto.h"

typedef struct frame_hdr_t {
    union{
        struct{
            uint8_t Opcode:4;
            uint8_t Rsv3:1;
            uint8_t Rsv2:1;
            uint8_t Rsv1:1;
            uint8_t Fin:1;
        }HByte;
        uint8_t uByte;
    }u1;
    union{
        struct{
            uint8_t PayloadLen:7;
            uint8_t Mask:1;
        }HByte;
        uint8_t uByte;
    }u2;
    union{
        uint16_t xpl16;
        uint64_t xpl64;
    }xpl;
    uint8_t maskey[4];
    uint32_t data_len;
}frame_hdr_t;

enum {
    FRAME_PARSE_STATE_HDR1,
    FRAME_PARSE_STATE_HDR2,
    FRAME_PARSE_STATE_HDREX,
    FRAME_PARSE_STATE_MASKEY,
    FRAME_PARSE_STATE_DATA,
    FRAME_PARSE_STATE_CLOSED,
    FRAME_PARSE_STATE_ERROR,
};

typedef struct ws_ctx_t {
    int status;
    frame_hdr_t frame_hdr;
    uint32_t recv_len;
    uint8_t* recv_buf;
}ws_ctx_t;

static int handle_data_mask(ws_ctx_t* ctx);

ws_ctx_t* ws_create()
{
    ws_ctx_t* ctx = (ws_ctx_t*)malloc(sizeof(ws_ctx_t));
    ctx->status = FRAME_PARSE_STATE_HDR1;
    ctx->recv_len = 0;
    ctx->recv_buf = NULL;
    return ctx;
}

void ws_destroy(ws_ctx_t *ctx)
{
    free(ctx);
}

void ws_free(void* buf)
{
    free(buf);
}

int ws_encode(ws_frame_type_t frame_type, const uint8_t *input, uint32_t input_len, uint8_t **output, uint32_t *output_len)
{
    uint8_t first_byte = 0x81;
    uint8_t second_byte = 0x00;
    uint8_t header_len = 2;
    if(frame_type == WS_FRAME_TYPE_BINARY)
    {
        first_byte = 0x82;
    }

    if(input_len <= 125)
    {//0 byte
        second_byte = (uint8_t)input_len;
    }
    else if(input_len <= 0xffff)
    {//2 bytes
        header_len += 2;
        second_byte = 126;
    }
    else
    {//8 bytes
        header_len += 8;
        second_byte = 127;
    }

    uint8_t* buf = (uint8_t*)malloc(header_len + input_len);
    buf[0] = first_byte;
    buf[1] = second_byte;
    if(126 == second_byte)
    {
        buf[2] = (unsigned char)(input_len >> 8);
        buf[3] = (unsigned char)input_len;
    }
    else if(127 == second_byte)
    {
        buf[2] = 0;
        buf[3] = 0;
        buf[4] = 0;
        buf[5] = 0;
        buf[6] = (unsigned char)(input_len >> 24);
        buf[7] = (unsigned char)(input_len >> 16);
        buf[8] = (unsigned char)(input_len >> 8);
        buf[9] = (unsigned char)input_len;
    }
    memcpy(buf + header_len, input, input_len);
    *output = buf;
    *output_len = header_len + input_len;

    return WS_ERROR_NOERR;
}

#define UB1	ctx->frame_hdr.u1.HByte
#define UB2	ctx->frame_hdr.u2.HByte
#define WS_MAX_FRAME_DATA_LENGTH	10*1024*1024
int ws_decode(ws_ctx_t *ctx, const uint8_t *input, uint32_t input_len, uint8_t **output, uint32_t *output_len, uint32_t *handled_len)
{
    uint32_t *pcur_pos = handled_len;
    *pcur_pos = 0;
    while((*pcur_pos) < input_len)
    {
        switch(ctx->status)
        {
        case FRAME_PARSE_STATE_HDR1:
            ctx->frame_hdr.u1.uByte = input[(*pcur_pos)++];
            if(UB1.Rsv1 != 0 || UB1.Rsv2 != 0 || UB1.Rsv3 != 0) {
                ctx->status = FRAME_PARSE_STATE_ERROR;
                return WS_ERROR_INVALID_FRAME;
            }
            ctx->status = FRAME_PARSE_STATE_HDR2;
            break;
        case FRAME_PARSE_STATE_HDR2:
            ctx->frame_hdr.u2.uByte = input[(*pcur_pos)++];
            ctx->frame_hdr.xpl.xpl64 = 0;
            ctx->recv_len = 0;
            ctx->status = FRAME_PARSE_STATE_HDREX;
            break;
        case FRAME_PARSE_STATE_HDREX:
            if(126 == UB2.PayloadLen)
            {
                uint32_t expect_len = 2;
                if(input_len-(*pcur_pos)+ctx->recv_len >= expect_len)
                {
                    for (; ctx->recv_len<expect_len; ++(*pcur_pos), ++ctx->recv_len) {
                        ctx->frame_hdr.xpl.xpl16 |= input[(*pcur_pos)] << ((expect_len-ctx->recv_len-1) << 3);
                    }
                    ctx->recv_len = 0;
                    if(ctx->frame_hdr.xpl.xpl16 < 126)
                    {// invalid ex payload length
                        ctx->status = FRAME_PARSE_STATE_ERROR;
                        return WS_ERROR_INVALID_LENGTH;
                    }
                    ctx->frame_hdr.data_len = ctx->frame_hdr.xpl.xpl16;
                    ctx->status = FRAME_PARSE_STATE_MASKEY;
                }
                else
                {
                    ctx->frame_hdr.xpl.xpl16 |= input[(*pcur_pos)++]<<8;
                    ++ctx->recv_len;
                    return WS_ERROR_NEED_MORE_DATA;
                }
            }
            else if(127 == UB2.PayloadLen)
            {
                uint32_t expect_len = 8;
                if(input_len-(*pcur_pos)+ctx->recv_len >= expect_len)
                {
                    for (; ctx->recv_len<expect_len; ++(*pcur_pos), ++ctx->recv_len) {
                        ctx->frame_hdr.xpl.xpl64 |= input[(*pcur_pos)] << ((expect_len-ctx->recv_len-1) << 3);
                    }
                    ctx->recv_len = 0;
                    if((ctx->frame_hdr.xpl.xpl64>>63) != 0)
                    {// invalid ex payload length
                        ctx->status = FRAME_PARSE_STATE_ERROR;
                        return WS_ERROR_INVALID_LENGTH;
                    }
                    ctx->frame_hdr.data_len = (uint32_t)ctx->frame_hdr.xpl.xpl64;
                    if(ctx->frame_hdr.data_len > WS_MAX_FRAME_DATA_LENGTH)
                    {// invalid ex payload length
                        ctx->status = FRAME_PARSE_STATE_ERROR;
                        return WS_ERROR_INVALID_LENGTH;
                    }
                    ctx->status = FRAME_PARSE_STATE_MASKEY;
                }
                else
                {
                    for (; (*pcur_pos)<input_len; ++(*pcur_pos), ++ctx->recv_len) {
                        ctx->frame_hdr.xpl.xpl64 |= input[(*pcur_pos)] << ((expect_len-ctx->recv_len-1) << 3);
                    }
                    return WS_ERROR_NEED_MORE_DATA;
                }
            }
            else
            {
                ctx->frame_hdr.data_len = UB2.PayloadLen;
                ctx->status = FRAME_PARSE_STATE_MASKEY;
            }
            break;
        case FRAME_PARSE_STATE_MASKEY:
            if(UB2.Mask)
            {
                uint32_t expect_len = 4;
                if(input_len-(*pcur_pos)+ctx->recv_len >= expect_len)
                {
                    memcpy(ctx->frame_hdr.maskey+ctx->recv_len, input+(*pcur_pos), expect_len-ctx->recv_len);
                    (*pcur_pos) += expect_len-ctx->recv_len;
                    ctx->recv_len = 0;
                    ctx->recv_buf = NULL;
                }
                else
                {
                    memcpy(ctx->frame_hdr.maskey+ctx->recv_len, input+(*pcur_pos), input_len-(*pcur_pos));
                    ctx->recv_len += input_len-(*pcur_pos);
                    (*pcur_pos) = input_len;
                    return WS_ERROR_NEED_MORE_DATA;
                }
            }
            ctx->status = FRAME_PARSE_STATE_DATA;
            break;
        case FRAME_PARSE_STATE_DATA:
            if(0x8 == UB1.Opcode)
            {
                // connection closed
                ctx->status = FRAME_PARSE_STATE_CLOSED;
                return WS_ERROR_CLOSED;
            }
            if(NULL == ctx->recv_buf) {
                ctx->recv_buf = malloc(ctx->frame_hdr.data_len + 1);
            }
            if(input_len-(*pcur_pos)+ctx->recv_len >= ctx->frame_hdr.data_len)
            {
                memcpy(ctx->recv_buf+ctx->recv_len, input+(*pcur_pos), ctx->frame_hdr.data_len-ctx->recv_len);
                (*pcur_pos) += ctx->frame_hdr.data_len-ctx->recv_len;
                ctx->recv_len = ctx->frame_hdr.data_len;
                handle_data_mask(ctx);
                ctx->recv_buf[ctx->recv_len] = '\0';
                *output = ctx->recv_buf;
                *output_len = ctx->recv_len;
                
                // prepare for next frame
                memset(&ctx->frame_hdr, 0, sizeof(ctx->frame_hdr));
                ctx->status = FRAME_PARSE_STATE_HDR1;
                ctx->recv_len = 0;
                ctx->recv_buf = NULL;
                return WS_ERROR_NOERR;
            }
            else
            {
                memcpy(ctx->recv_buf+ctx->recv_len, input+(*pcur_pos), input_len-(*pcur_pos));
                ctx->recv_len += input_len-(*pcur_pos);
                (*pcur_pos) = input_len;
                return WS_ERROR_NEED_MORE_DATA;
            }
            break;
        default:
            return WS_ERROR_INVALID_FRAME;
        }
    }
    return FRAME_PARSE_STATE_HDR1==ctx->status?WS_ERROR_NOERR:WS_ERROR_NEED_MORE_DATA;
}

static int handle_data_mask(ws_ctx_t* ctx)
{
    if(0 == UB2.Mask) return WS_ERROR_NOERR;
    if(NULL == ctx->recv_buf || 0 == ctx->recv_len) return WS_ERROR_INVALID_FRAME;

    for(uint32_t i=0; i < ctx->recv_len; ++i)
    {
        ctx->recv_buf[i] = ctx->recv_buf[i] ^ ctx->frame_hdr.maskey[i%4];
    }

    return WS_ERROR_NOERR;
}

