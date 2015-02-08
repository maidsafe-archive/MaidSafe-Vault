_MaidNode_ =>> |__MaidManager__ [Allow ? So : PutFailure] *->> |__DataManager__ [Exist(D) ? PutResponse : {(AddAccount(D.name, ClosestPnTo(D.name), 2ndClosestPnTo(D.name))])({So.To(ClosestTo(D.name)), So.To(2ndClosestTo(D.name))}), PutResponse}] *->> |__PmidManager__ {[LruCache.Add(D), AddEntry(PN1, PMG1, D.name), AddEntry(PN2, PMG2, D.name), So], PutResponse} *-> |_PmidNode_[!Store ? PutFailure]

#####PmidNode::PutFailure
_PmidNode_ ->> |__PmidManager__ [Delete(PNx, PMG, D.name), AddEntry(PN(new), PMG, D.name), So.Put(D /* get D from cache or network*/)] *-> | _PmidNode_[!Store ? PutFailure]
