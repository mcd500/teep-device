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

UsefulBufC QCBORDecode_SubObjectFrom(QCBORDecodeContext *pCtx, const QCBORItemWithOffset *pFirstItem)
{
    uint8_t uNestLevel = pFirstItem->uNestLevel;
    uint8_t uNextNestLevel = pFirstItem->uNextNestLevel;
    while (uNestLevel != uNextNestLevel) {
        QCBORItem Item;
        QCBORError err = QCBORDecode_GetNext(pCtx, &Item);
        if (err != QCBOR_SUCCESS) {
            return NULLUsefulBufC;
        } else {
            uNextNestLevel = Item.uNextNestLevel;
        }
    }
    return QCBORDecode_Slice(pCtx, pFirstItem->offset, QCBORDecode_Tell(pCtx));
}

UsefulBufC QCBORDecode_NextSubObject(QCBORDecodeContext *pCtx, QCBORItemWithOffset *pDecodedFirstItem)
{
    QCBORError err = QCBORDecode_GetNextWithOffset(pCtx, pDecodedFirstItem);
    if (err == QCBOR_SUCCESS) {
        return QCBORDecode_SubObjectFrom(pCtx, pDecodedFirstItem);
    } else {
        return NULLUsefulBufC;
    }
}
