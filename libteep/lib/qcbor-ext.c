#include "qcbor-ext.h"

size_t QCBORDecode_Tell(QCBORDecodeContext *pCtx)
{
    return UsefulInputBuf_Tell(pCtx->InBuf);
}

typedef struct _QCBORItemWithOffset {
    size_t offset;
    QCBORItem item;
} QCBORItemWithOffset;

QCBORError QCBORDecode_GetNextWithOffset(QCBORDecodeContext *pCtx, QCBORItemWithOffset *pDecodedItem)
{
    pDecodedItem->offset = QCBORDecode_Tell(pCtx);
    return QCBORDecode_GetNext(pCtx, &pDecodedItem->item);
}
