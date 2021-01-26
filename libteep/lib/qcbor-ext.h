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

UsefulBufC QCBORDecode_Slice(QCBORDecodeContext *pCtx, size_t begin, size_t end);


#ifdef __cplusplus
}
#endif
