###Put(D)
_MaidNode_ =>> |__MaidManager__ [Allow ? So : PutFailure]  *->> |__DataManager__  [EXISTS(D) ? PutResponse : {So, PutToCache, PutResponse}] *->> |__PmidManager__ {So, Put.Sy, PutResponse} *-> |_PmidNode_ [Store ? NoOp : PutFailure]

--
#####MaidManager::PutFailure
__MaidManager__ *-> |_MaidNode_ 

--
#####DataManager::PutResponse
__DataManager__ *->> |__MaidManager__ [Put.Sy]

--
#####PmidManager::PutResponse
__PmidManager__ *->> |__DataManager__ {[Value.Pmids.Count > Threshold ? NoOp : Replicate(D)], [AddPmid.Sy]}

--
#####PmidNode::PutFailure
_PmidNode_ ->> |__PmidManager__ {So, Delete.Sy} *->> |__DataManager__ {[Value.Pmids.Count > Threshold ? NoOp : Replicate(D)], [Removepmid.Sy], [LyingPmidNode ? SendCorrectionToPmidManager : NoOp]} 

