/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (C) 2019 National Institute of Advanced Industrial Science
 *                           and Technology (AIST)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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
    uint8_t uNestLevel = pFirstItem->item.uNestingLevel;
    uint8_t uNextNestLevel = pFirstItem->item.uNextNestLevel;
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
