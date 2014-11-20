|Client =>> |MaidManager *->> |DataManager  [EXIST(D) ? PutResponse : (So, PutToCache, PutResponse)] *->> |PmidManager (So, Sync, PutResponse) *-> |PmidNode (Store ? NoOp : PutFailure)
