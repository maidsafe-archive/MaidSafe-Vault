
# MaidSafe Language of the Network

### general considerations

Nodes and data both live in the same XOR space which is addressed with a 2^512 bit key; a Network-Addressable-Element (NAE).  A message flows from a NAE to a NAE.  An operation can be performed on a message flow by a manager group.

A message flow from start to end can be represented by

    < NAE_1 | manager | NAE_2 >

where the NAE can be a node (ie a vault or a client - Direct NAE) or a data element (Indirect NAE).  The manager group operating on the message flow will act forward `manager | NAE_2 >` under normal / successful conditions.  The manager group will act backwards `< NAE_1 | manager` upon error.

If no operation is needed on the message flow then this special case is represented by

    < NAE | NAE >

For a given message type `ACTION` the functions shall be named

    < A::ACTION | B::ActOnACTION | C::PerformACTION >

The function `HandleACTION` is reserved for the VaultFacade.  Alternatively `HandleACTION` functions in VaultFacade can be, similarly to RoutingNode, overloaded on message type.  Currently these message types are Connect\*, ConnectResponse\*, FindGroup\*, FindGroupResponse\*, GetData, GetDataResponse, PutData, PostMessage, where message types with \* are completed in routing and exempt from this naming convention.

For clarity a message is passed through RoutingNode and VaultFacade upto Persona according to following abstraction

    RoutingNode::MessageReceived {
      Parse; Filter; Cache; SendOn; Relay; Drop; Sentinel
      Switch RoutingNode::HandleMessage(MessageType /* */) {
        Completed in routing or
        VaultFacade::HandleACTION {
          Switch on Authority & Condition on DataType {
            Persona::ActOnACTION or  // Currently also named HandleACTION
            Persona::PerformACTION
          }
        }
      }
    }


The triplet structure `< A | B | C >` captures the general characteristic of every message flow.  The structure is event-driven  `A` corresponds to a physical 

Remaining conventions:

    D      data
    H()    Hash512
    H^n()  n-th Hash512
    Manager{Address};
           {Address} omitted where evident,
           e.g. MaidManagers{MaidNode}


### MAID PUT
all functions are templated on DataType

MAID PUT and MAID PUT CONFIRM

    < MaidNode::Put(D)
    | MaidManager::HandlePut(D)
    | DataManager::HandlePut(D)
    | PmidManager::HandlePut(D.name)
    | PmidNode::Persist(D) >

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
