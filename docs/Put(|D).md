###Put<Data>
|MaidNode =>> |__MaidManager__ [PutResponse]  *->> |__DataManager__  [EXIST(D) ? PutResponse : {So, PutToCache, PutResponse}] *->> |__PmidManager__ ((So, Sync, PutResponse)) *-> |PmidNode [Store ? NoOp : PutFailure]

--
#####MaidManager::PutResponse
|__MaidManager__ *-> |MaidNode 

--
#####DataManager::PutResponse
|__DataManager__ *->> |__MaidManager__ [Sync]

--
#####PmidManager::PutResponse
|__PmidManager__ *->> |__DataManager__ [Sync(Pmid.Size > (group_size + 1) / 2 ? NoOp : SendPutResponseWithCachedData)]

--
#####PmidNode::PutFailure
|PmidNode ->> |__PmidManager__ {So, Sync} *->> |__DataManager__ {[LyingPmidNode ? DoubleDerank : Derank)(Sync)], [LyingPmidNode ? SendCorrectionToPmidManager : NoOp)]} 

