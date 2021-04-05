#pragma once

#include <qcbor/qcbor.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * tell current offset of QCBORDecodeContext
 */
size_t QCBORDecode_Tell(QCBORDecodeContext *pCtx);

/*
 * get sub-UsefulBuf of specified range [begin, end)
 */
UsefulBufC QCBORDecode_Slice(QCBORDecodeContext *pCtx, size_t begin, size_t end);

typedef struct _QCBORItemWithOffset {
    size_t offset;
    QCBORItem item;
} QCBORItemWithOffset;

QCBORError QCBORDecode_GetNextWithOffset(QCBORDecodeContext *pCtx, QCBORItemWithOffset *pDecodedItem);

UsefulBufC QCBORDecode_Slice(QCBORDecodeContext *pCtx, size_t begin, size_t end);

UsefulBufC QCBORDecode_SubObjectFrom(QCBORDecodeContext *pCtx, const QCBORItemWithOffset *pFirstItem);

UsefulBufC QCBORDecode_NextSubObject(QCBORDecodeContext *pCtx, QCBORItemWithOffset *pDecodedFirstItem);

#ifdef __cplusplus
}
#endif
