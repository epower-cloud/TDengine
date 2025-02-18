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

#include "tq.h"

static int32_t tqAddBlockDataToRsp(const SSDataBlock* pBlock, SMqDataRsp* pRsp, int32_t numOfCols) {
  int32_t dataStrLen = sizeof(SRetrieveTableRsp) + blockGetEncodeSize(pBlock);
  void*   buf = taosMemoryCalloc(1, dataStrLen);
  if (buf == NULL) return -1;

  SRetrieveTableRsp* pRetrieve = (SRetrieveTableRsp*)buf;
  pRetrieve->useconds = 0;
  pRetrieve->precision = TSDB_DEFAULT_PRECISION;
  pRetrieve->compressed = 0;
  pRetrieve->completed = 1;
  pRetrieve->numOfRows = htonl(pBlock->info.rows);

  // TODO enable compress
  int32_t actualLen = 0;
  blockEncode(pBlock, pRetrieve->data, &actualLen, numOfCols, false);
  actualLen += sizeof(SRetrieveTableRsp);
  ASSERT(actualLen <= dataStrLen);
  taosArrayPush(pRsp->blockDataLen, &actualLen);
  taosArrayPush(pRsp->blockData, &buf);
  return 0;
}

static int32_t tqAddBlockSchemaToRsp(const STqExecHandle* pExec, SMqDataRsp* pRsp) {
  SSchemaWrapper* pSW = tCloneSSchemaWrapper(pExec->pExecReader->pSchemaWrapper);
  if (pSW == NULL) {
    return -1;
  }
  taosArrayPush(pRsp->blockSchema, &pSW);
  return 0;
}

static int32_t tqAddTbNameToRsp(const STQ* pTq, int64_t uid, SMqDataRsp* pRsp) {
  SMetaReader mr = {0};
  metaReaderInit(&mr, pTq->pVnode->pMeta, 0);
  // TODO add reference to gurantee success
  if (metaGetTableEntryByUid(&mr, uid) < 0) {
    metaReaderClear(&mr);
    return -1;
  }
  char* tbName = strdup(mr.me.name);
  taosArrayPush(pRsp->blockTbName, &tbName);
  metaReaderClear(&mr);
  return 0;
}

int64_t tqScan(STQ* pTq, const STqHandle* pHandle, SMqDataRsp* pRsp, STqOffsetVal* pOffset) {
  const STqExecHandle* pExec = &pHandle->execHandle;
  qTaskInfo_t          task = pExec->execCol.task;

  if (qStreamPrepareScan(task, pOffset) < 0) {
    tqDebug("prepare scan failed, return");
    if (pOffset->type == TMQ_OFFSET__LOG) {
      pRsp->rspOffset = *pOffset;
      return 0;
    } else {
      tqOffsetResetToLog(pOffset, pHandle->snapshotVer);
      if (qStreamPrepareScan(task, pOffset) < 0) {
        tqDebug("prepare scan failed, return");
        pRsp->rspOffset = *pOffset;
        return 0;
      }
    }
  }

  int32_t rowCnt = 0;
  while (1) {
    SSDataBlock* pDataBlock = NULL;
    uint64_t     ts = 0;
    tqDebug("task start to execute");
    if (qExecTask(task, &pDataBlock, &ts) < 0) {
      ASSERT(0);
    }
    tqDebug("task execute end, get %p", pDataBlock);

    if (pDataBlock != NULL) {
      if (pRsp->withTbName) {
        if (pOffset->type == TMQ_OFFSET__LOG) {
          int64_t uid = pExec->pExecReader->msgIter.uid;
          if (tqAddTbNameToRsp(pTq, uid, pRsp) < 0) {
            continue;
          }
        } else {
          pRsp->withTbName = 0;
        }
      }
      tqAddBlockDataToRsp(pDataBlock, pRsp, pExec->numOfCols);
      pRsp->blockNum++;
      if (pOffset->type == TMQ_OFFSET__LOG) {
        continue;
      } else {
        rowCnt += pDataBlock->info.rows;
        if (rowCnt <= 4096) continue;
      }
    }

    if (pRsp->blockNum == 0 && pOffset->type == TMQ_OFFSET__SNAPSHOT_DATA) {
      tqDebug("vgId: %d, tsdb consume over, switch to wal, ver %" PRId64, TD_VID(pTq->pVnode),
              pHandle->snapshotVer + 1);
      tqOffsetResetToLog(pOffset, pHandle->snapshotVer);
      qStreamPrepareScan(task, pOffset);
      continue;
    }

    void* meta = qStreamExtractMetaMsg(task);
    if (meta != NULL) {
      // tq add meta to rsp
    }

    if (qStreamExtractOffset(task, &pRsp->rspOffset) < 0) {
      ASSERT(0);
    }

    ASSERT(pRsp->rspOffset.type != 0);

#if 0
    if (pRsp->reqOffset.type == TMQ_OFFSET__LOG) {
      if (pRsp->blockNum > 0) {
        ASSERT(pRsp->rspOffset.version > pRsp->reqOffset.version);
      } else {
        ASSERT(pRsp->rspOffset.version >= pRsp->reqOffset.version);
      }
    }
#endif

    tqDebug("task exec exited");
    break;
  }

  return 0;
}

#if 0
int32_t tqScanSnapshot(STQ* pTq, const STqExecHandle* pExec, SMqDataRsp* pRsp, STqOffsetVal offset, int32_t workerId) {
  ASSERT(pExec->subType == TOPIC_SUB_TYPE__COLUMN);
  qTaskInfo_t task = pExec->execCol.task[workerId];

  if (qStreamPrepareTsdbScan(task, offset.uid, offset.ts) < 0) {
    ASSERT(0);
  }

  int32_t rowCnt = 0;
  while (1) {
    SSDataBlock* pDataBlock = NULL;
    uint64_t     ts = 0;
    if (qExecTask(task, &pDataBlock, &ts) < 0) {
      ASSERT(0);
    }
    if (pDataBlock == NULL) break;

    ASSERT(pDataBlock->info.rows != 0);
    ASSERT(taosArrayGetSize(pDataBlock->pDataBlock) != 0);

    tqAddBlockDataToRsp(pDataBlock, pRsp);

    if (pRsp->withTbName) {
      pRsp->withTbName = 0;
#if 0
      int64_t uid;
      int64_t ts;
      if (qGetStreamScanStatus(task, &uid, &ts) < 0) {
        ASSERT(0);
      }
      tqAddTbNameToRsp(pTq, uid, pRsp);
#endif
    }
    pRsp->blockNum++;

    rowCnt += pDataBlock->info.rows;
    if (rowCnt >= 4096) break;
  }
  int64_t uid;
  int64_t ts;
  if (qGetStreamScanStatus(task, &uid, &ts) < 0) {
    ASSERT(0);
  }
  tqOffsetResetToData(&pRsp->rspOffset, uid, ts);

  return 0;
}
#endif

int32_t tqLogScanExec(STQ* pTq, STqExecHandle* pExec, SSubmitReq* pReq, SMqDataRsp* pRsp) {
  ASSERT(pExec->subType != TOPIC_SUB_TYPE__COLUMN);

  if (pExec->subType == TOPIC_SUB_TYPE__TABLE) {
    pRsp->withSchema = 1;
    STqReader* pReader = pExec->pExecReader;
    tqReaderSetDataMsg(pReader, pReq, 0);
    while (tqNextDataBlock(pReader)) {
      SSDataBlock block = {0};
      if (tqRetrieveDataBlock(&block, pReader) < 0) {
        if (terrno == TSDB_CODE_TQ_TABLE_SCHEMA_NOT_FOUND) continue;
      }
      if (pRsp->withTbName) {
        int64_t uid = pExec->pExecReader->msgIter.uid;
        if (tqAddTbNameToRsp(pTq, uid, pRsp) < 0) {
          blockDataFreeRes(&block);
          continue;
        }
      }
      tqAddBlockDataToRsp(&block, pRsp, taosArrayGetSize(block.pDataBlock));
      blockDataFreeRes(&block);
      tqAddBlockSchemaToRsp(pExec, pRsp);
      pRsp->blockNum++;
    }
  } else if (pExec->subType == TOPIC_SUB_TYPE__DB) {
    pRsp->withSchema = 1;
    STqReader* pReader = pExec->pExecReader;
    tqReaderSetDataMsg(pReader, pReq, 0);
    while (tqNextDataBlockFilterOut(pReader, pExec->execDb.pFilterOutTbUid)) {
      SSDataBlock block = {0};
      if (tqRetrieveDataBlock(&block, pReader) < 0) {
        if (terrno == TSDB_CODE_TQ_TABLE_SCHEMA_NOT_FOUND) continue;
      }
      if (pRsp->withTbName) {
        int64_t uid = pExec->pExecReader->msgIter.uid;
        if (tqAddTbNameToRsp(pTq, uid, pRsp) < 0) {
          blockDataFreeRes(&block);
          continue;
        }
      }
      tqAddBlockDataToRsp(&block, pRsp, taosArrayGetSize(block.pDataBlock));
      blockDataFreeRes(&block);
      tqAddBlockSchemaToRsp(pExec, pRsp);
      pRsp->blockNum++;
    }
  }

  if (pRsp->blockNum == 0) {
    return -1;
  }

  return 0;
}
