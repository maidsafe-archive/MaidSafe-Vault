
# MaidSafe Language of the Network


The network allows

legend:

    <      scatter
    >      gather
    |      sentinel
    D      data
    H()    Hash512
    H^n()  n-th Hash512
    Manager{Address};
           {Address} omitted where evident,
           e.g. MaidManagers{MaidNode}


### MAID PUT
all functions are templated on DataType

MAID PUT and MAID PUT CONFIRM

    < MaidNode::Put(D) | MaidManager::HandlePut(D)
    | DataManager::InstantiateData (Maid, D)
    | PmidManager::Store(D.name) | PmidNode::Persist(D) >

    < PmidNode::Persist(D) | PmidManager:: ConfirmStoragePromise(D.name)
    | DataManager::PersistedInVault(D.name, Pmid.name)
    | MaidManager::AccountCost(Maid, D.name, cost)
    | MaidNode::ConfirmPut(D.name) >


Implementation:

    < MaidNode::Put(D) {
        client_routing.put(D) }
    | MaidManager::HandlePut(D) {
        Allow ? ReserveCost(K*D.size())
                return Flow [ DataManager{D.name}, InstantiateData(Maid, D) ]
              : return Error [ MaidNode, error::OutOfCredit ] }
    | DataManager{D.name}::InstantiateData(Maid, D) {
        ReplicationPmidNodes = SelectKClosestNodesTo(D.name)
        InstantiateRegister(Maid, D.name, D, ReplicationPmidNodes)
            // InstantiateRegister excluded from ChurnHandle ?
            // DM keeps the data in the InstantiateRegister
            // until it has at least two confirmations
            // from ReplicationPmidNodes, then it moves Data into LRUcache
            // This is dealt with in DM::PersistedInVault
        return Flow [ PmidManager{ReplicationPmidNodes}, Store(D) ] }
    | PmidManager{ReplicationPmidNode}::Store(D) {
            //  Only register reversed link upon successful storage
        IntegrityCheck(D) // eg D.name = H(D) for ImmutableData
        return Flow [ PmidNode, Persist(D) ]
    | PmidNode::Persist(D) {
        PersistInVault(D)
        on_error return Error [ PmidManager{PmidNode},
                                error::FailedToStoreInVault(D) ]
        return Flow [ PmidManager{PmidNode}, ConfirmStoragePromise(D.name) ] }
    >

    < PmidNode::Persist(D) {/* resuming from above */}
    | PmidManager{PmidNode}::ConfirmStoragePromise(D.name) {
        Register(PmidNode, D.name)
        return Flow [ DataManager{D.name}, PersistedInVault(D.name,
                                                            Pmid.name) ] }
    | DataManager{D.name}::PersistedInVault(D.name, Pmid.name) {
        RegisterPmid(D.name, D.size, Pmid.name)
            // size known from D stored in InstantiateRegister
        InstantiateRegister.RemovePmid(Pmid.name)
        RedundantCopies = PmidsRegistered(D.name).size
        if (RedundantCopies >= 2) {
          MoveToLRUcache(InstantiateRegister(D.name).getData())
        }
        return Flow [ MaidManager{Maid}, AccountCost(Maid,
                            D.name, RedundantCopies * D.size) ] }
    | MaidManager{Maid}::AccountCost(Maid, D.name, cost) {
        PayInSafeCoinUnpaidRemainder(UpdatedCost = cost)
        return Flow [ MaidNode, PutConfirm(D.name, cost) ] }
    | MaidNode::PutConfirm(D.name, cost) {
        Return to higher levels, persistance can be deduced from cost }
    >

### MAID GET

    < MaidNode::Get(D.name) {
        client_routing.get(D.name) }
    | DataManager{D.name}::HandleGet(D.name, ReplyToClient) {
        if (LRUcache.get(D.name)) {
          return Flow [ ReplyToClient, GetResponse(LRUcache.get(D.name)) ]
        }
        OnlinePmidNodes = Register.getOnlinePmidNodes(D.name)
        return Flow [ OnlinePmidNodes, PushForward(D.name, ReplyToClient) ] }
    | PmidNode{OnlinePmidNode}::PushForward(D.name, ReplyToClient) {
        return Flow [ ReplyToClient, GetResponse(Vault.get(D.name)) }
    >

### CHURN HANDLE
