#pragma once

#include <qcbor/qcbor.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t QCBORDecode_Tell(QCBORDecodeContext *pCtx);

typedef struct _QCBORItemWithOffset {
    size_t offset;
    QCBORItem item;
} QCBORItemWithOffset;

QCBORError QCBORDecode_GetNextWithOffset(QCBORDecodeContext *pCtx, QCBORItemWithOffset *pDecodedItem);

#ifdef __cplusplus
}
#endif
