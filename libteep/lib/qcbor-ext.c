#include "qcbor-ext.h"

size_t QCBORDecode_Tell(QCBORDecodeContext *pCtx)
{
    return UsefulInputBuf_Tell(&pCtx->InBuf);
}

QCBORError QCBORDecode_GetNextWithOffset(QCBORDecodeContext *pCtx, QCBORItemWithOffset *pDecodedItem)
{
    pDecodedItem->offset = QCBORDecode_Tell(pCtx);
    return QCBORDecode_GetNext(pCtx, &pDecodedItem->item);
}

UsefulBufC QCBORDecode_Slice(QCBORDecodeContext *pCtx, size_t begin, size_t end)
{
    UsefulBufC src = pCtx->InBuf.UB;
    return UsefulBuf_Tail(UsefulBuf_Head(src, end), begin);
}
