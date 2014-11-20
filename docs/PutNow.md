###Put(D)
_MaidNode_ =>> |__MaidManager__ [Allow ? {Put.Sy, So} : PutFailure]  *->> |__DataManager__  [EXISTS(D) ? PutResponse : {So, PutToCache, PutResponse}] *->> |__PmidManager__ {So, (Sy)(PutResponse)} *-> |_PmidNode_ [Store ? NoOp : PutFailure]

--
#####MaidManager::PutFailure
__MaidManager__ *-> |_MaidNode_ 

--
#####DataManager::PutResponse
__DataManager__ *->> |__MaidManager__ {So Sy} *-> |_MaidNode_

--
#####PmidManager::PutResponse
__PmidManager__ *->> |__DataManager__ {AddPmid.Sy, [RemovePmid ? RemovePmid.Sy : NoOp]}

--
#####PmidNode::PutFailure
_PmidNode_ ->> |__PmidManager__ {So, Delete.Sy} *->> |__DataManager__ {[Retry ? (GetFromCache)(PutRequest.So) : NoOp], RemovePmid.Sy } 

