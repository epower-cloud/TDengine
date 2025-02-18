/*
 * Copyright (c) 2019 TAOS Data, Inc. <jhtao@taosdata.com>
 *
 * This program is free software: you can use, redistribute, and/or modify
 * it under the terms of the GNU Affero General Public License, version 3
 * or later ("AGPL"), as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "tdbInt.h"
#include "tq.h"

int32_t tEncodeSTqHandle(SEncoder* pEncoder, const STqHandle* pHandle) {
  if (tStartEncode(pEncoder) < 0) return -1;
  if (tEncodeCStr(pEncoder, pHandle->subKey) < 0) return -1;
  if (tEncodeI64(pEncoder, pHandle->consumerId) < 0) return -1;
  if (tEncodeI64(pEncoder, pHandle->snapshotVer) < 0) return -1;
  if (tEncodeI32(pEncoder, pHandle->epoch) < 0) return -1;
  if (tEncodeI8(pEncoder, pHandle->execHandle.subType) < 0) return -1;
  if (pHandle->execHandle.subType == TOPIC_SUB_TYPE__COLUMN) {
    if (tEncodeCStr(pEncoder, pHandle->execHandle.execCol.qmsg) < 0) return -1;
  }
  tEndEncode(pEncoder);
  return pEncoder->pos;
}

int32_t tDecodeSTqHandle(SDecoder* pDecoder, STqHandle* pHandle) {
  if (tStartDecode(pDecoder) < 0) return -1;
  if (tDecodeCStrTo(pDecoder, pHandle->subKey) < 0) return -1;
  if (tDecodeI64(pDecoder, &pHandle->consumerId) < 0) return -1;
  if (tDecodeI64(pDecoder, &pHandle->snapshotVer) < 0) return -1;
  if (tDecodeI32(pDecoder, &pHandle->epoch) < 0) return -1;
  if (tDecodeI8(pDecoder, &pHandle->execHandle.subType) < 0) return -1;
  if (pHandle->execHandle.subType == TOPIC_SUB_TYPE__COLUMN) {
    if (tDecodeCStrAlloc(pDecoder, &pHandle->execHandle.execCol.qmsg) < 0) return -1;
  }
  tEndDecode(pDecoder);
  return 0;
}

int32_t tqMetaOpen(STQ* pTq) {
  if (tdbOpen(pTq->path, 16 * 1024, 1, &pTq->pMetaDB) < 0) {
    ASSERT(0);
    return -1;
  }

  if (tdbTbOpen("tq.db", -1, -1, NULL, pTq->pMetaDB, &pTq->pExecStore) < 0) {
    ASSERT(0);
    return -1;
  }

  if (tdbTbOpen("tq.check.db", -1, -1, NULL, pTq->pMetaDB, &pTq->pCheckStore) < 0) {
    ASSERT(0);
    return -1;
  }

  if (tqMetaRestoreHandle(pTq) < 0) {
    return -1;
  }

  if (tqMetaRestoreCheckInfo(pTq) < 0) {
    return -1;
  }

  return 0;
}

int32_t tqMetaClose(STQ* pTq) {
  if (pTq->pExecStore) {
    tdbTbClose(pTq->pExecStore);
  }
  if (pTq->pCheckStore) {
    tdbTbClose(pTq->pCheckStore);
  }
  tdbClose(pTq->pMetaDB);
  return 0;
}

int32_t tqMetaSaveCheckInfo(STQ* pTq, const char* key, const void* value, int32_t vLen) {
  TXN txn;
  if (tdbTxnOpen(&txn, 0, tdbDefaultMalloc, tdbDefaultFree, NULL, TDB_TXN_WRITE | TDB_TXN_READ_UNCOMMITTED) < 0) {
    return -1;
  }

  if (tdbBegin(pTq->pMetaDB, &txn) < 0) {
    return -1;
  }

  if (tdbTbUpsert(pTq->pExecStore, key, strlen(key), value, vLen, &txn) < 0) {
    return -1;
  }

  if (tdbCommit(pTq->pMetaDB, &txn) < 0) {
    return -1;
  }

  return 0;
}

int32_t tqMetaDeleteCheckInfo(STQ* pTq, const char* key) {
  TXN txn;

  if (tdbTxnOpen(&txn, 0, tdbDefaultMalloc, tdbDefaultFree, NULL, TDB_TXN_WRITE | TDB_TXN_READ_UNCOMMITTED) < 0) {
    ASSERT(0);
  }

  if (tdbBegin(pTq->pMetaDB, &txn) < 0) {
    ASSERT(0);
  }

  if (tdbTbDelete(pTq->pCheckStore, key, (int)strlen(key), &txn) < 0) {
    /*ASSERT(0);*/
  }

  if (tdbCommit(pTq->pMetaDB, &txn) < 0) {
    ASSERT(0);
  }

  return 0;
}

int32_t tqMetaRestoreCheckInfo(STQ* pTq) {
  TBC* pCur = NULL;
  if (tdbTbcOpen(pTq->pCheckStore, &pCur, NULL) < 0) {
    ASSERT(0);
    return -1;
  }

  void*    pKey = NULL;
  int      kLen = 0;
  void*    pVal = NULL;
  int      vLen = 0;
  SDecoder decoder;

  tdbTbcMoveToFirst(pCur);

  while (tdbTbcNext(pCur, &pKey, &kLen, &pVal, &vLen) == 0) {
    STqCheckInfo info;
    tDecoderInit(&decoder, (uint8_t*)pVal, vLen);
    if (tDecodeSTqCheckInfo(&decoder, &info) < 0) {
      terrno = TSDB_CODE_OUT_OF_MEMORY;
      return -1;
    }
    tDecoderClear(&decoder);
    if (taosHashPut(pTq->pCheckInfo, info.topic, strlen(info.topic), &info, sizeof(STqCheckInfo)) < 0) {
      terrno = TSDB_CODE_OUT_OF_MEMORY;
      return -1;
    }
  }
  tdbTbcClose(pCur);
  return 0;
}

int32_t tqMetaSaveHandle(STQ* pTq, const char* key, const STqHandle* pHandle) {
  int32_t code;
  int32_t vlen;
  tEncodeSize(tEncodeSTqHandle, pHandle, vlen, code);
  ASSERT(code == 0);

  tqDebug("tq save %s(%d) consumer %" PRId64 " vgId:%d", pHandle->subKey, strlen(pHandle->subKey), pHandle->consumerId,
          TD_VID(pTq->pVnode));

  void* buf = taosMemoryCalloc(1, vlen);
  if (buf == NULL) {
    ASSERT(0);
  }

  SEncoder encoder;
  tEncoderInit(&encoder, buf, vlen);

  if (tEncodeSTqHandle(&encoder, pHandle) < 0) {
    ASSERT(0);
  }

  TXN txn;

  if (tdbTxnOpen(&txn, 0, tdbDefaultMalloc, tdbDefaultFree, NULL, TDB_TXN_WRITE | TDB_TXN_READ_UNCOMMITTED) < 0) {
    ASSERT(0);
  }

  if (tdbBegin(pTq->pMetaDB, &txn) < 0) {
    ASSERT(0);
  }

  if (tdbTbUpsert(pTq->pExecStore, key, (int)strlen(key), buf, vlen, &txn) < 0) {
    ASSERT(0);
  }

  if (tdbCommit(pTq->pMetaDB, &txn) < 0) {
    ASSERT(0);
  }

  tEncoderClear(&encoder);
  taosMemoryFree(buf);
  return 0;
}

int32_t tqMetaDeleteHandle(STQ* pTq, const char* key) {
  TXN txn;

  if (tdbTxnOpen(&txn, 0, tdbDefaultMalloc, tdbDefaultFree, NULL, TDB_TXN_WRITE | TDB_TXN_READ_UNCOMMITTED) < 0) {
    ASSERT(0);
  }

  if (tdbBegin(pTq->pMetaDB, &txn) < 0) {
    ASSERT(0);
  }

  if (tdbTbDelete(pTq->pExecStore, key, (int)strlen(key), &txn) < 0) {
    /*ASSERT(0);*/
  }

  if (tdbCommit(pTq->pMetaDB, &txn) < 0) {
    ASSERT(0);
  }

  return 0;
}

int32_t tqMetaRestoreHandle(STQ* pTq) {
  TBC* pCur = NULL;
  if (tdbTbcOpen(pTq->pExecStore, &pCur, NULL) < 0) {
    ASSERT(0);
    return -1;
  }

  void*    pKey = NULL;
  int      kLen = 0;
  void*    pVal = NULL;
  int      vLen = 0;
  SDecoder decoder;

  tdbTbcMoveToFirst(pCur);

  while (tdbTbcNext(pCur, &pKey, &kLen, &pVal, &vLen) == 0) {
    STqHandle handle;
    tDecoderInit(&decoder, (uint8_t*)pVal, vLen);
    tDecodeSTqHandle(&decoder, &handle);

    handle.pRef = walOpenRef(pTq->pVnode->pWal);
    if (handle.pRef == NULL) {
      ASSERT(0);
      return -1;
    }
    walRefVer(handle.pRef, handle.snapshotVer);

    if (handle.execHandle.subType == TOPIC_SUB_TYPE__COLUMN) {
      SReadHandle reader = {
          .meta = pTq->pVnode->pMeta,
          .vnode = pTq->pVnode,
          .initTableReader = true,
          .initTqReader = true,
          .version = handle.snapshotVer,
      };

      handle.execHandle.execCol.task = qCreateQueueExecTaskInfo(
          handle.execHandle.execCol.qmsg, &reader, &handle.execHandle.numOfCols, &handle.execHandle.pSchemaWrapper);
      ASSERT(handle.execHandle.execCol.task);
      void* scanner = NULL;
      qExtractStreamScanner(handle.execHandle.execCol.task, &scanner);
      ASSERT(scanner);
      handle.execHandle.pExecReader = qExtractReaderFromStreamScanner(scanner);
      ASSERT(handle.execHandle.pExecReader);
    } else {
      handle.pWalReader = walOpenReader(pTq->pVnode->pWal, NULL);
      handle.execHandle.execDb.pFilterOutTbUid =
          taosHashInit(64, taosGetDefaultHashFunction(TSDB_DATA_TYPE_BIGINT), false, HASH_NO_LOCK);
    }
    tqDebug("tq restore %s consumer %" PRId64 " vgId:%d", handle.subKey, handle.consumerId, TD_VID(pTq->pVnode));
    taosHashPut(pTq->pHandle, pKey, kLen, &handle, sizeof(STqHandle));
  }

  tdbTbcClose(pCur);
  return 0;
}

