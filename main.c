//
//  main.c
//  wstest
//
//  Created by Fengping Bao <jamol@live.com> on 11/10/14.
//  Copyright (c) 2014 wme. All rights reserved.
//

#include <stdio.h>
#include "wsproto.h"

int main(int argc, const char * argv[]) {
    uint8_t data[128*1024];
    for (int i=0; i<sizeof(data); ++i) {
        data[i] = i;
    }
    uint8_t *en_data = NULL;
    uint32_t en_len = 0;
    ws_encode(WS_FRAME_TYPE_BINARY, data, sizeof(data), &en_data, &en_len);
    uint8_t *de_data = NULL;
    uint32_t de_len = 0;
    ws_proto_t ws_proto_de = ws_create();
    uint32_t handled_len = 0;
    ws_decode(ws_proto_de, en_data, en_len, &de_data, &de_len, &handled_len);
    ws_free(en_data);
    ws_free(de_data);
    ws_destroy(ws_proto_de);
    return 0;
}
