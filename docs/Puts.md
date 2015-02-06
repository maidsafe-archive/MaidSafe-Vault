_MaidNode_ =>> |__MaidManager__ [Allow ? So : PutFailure]  *->> |__DataManager__  [Exist(D) ? PutResponse : {(Store(D.name))(So.To(#D.name)), PutResponse}] *->> |_PmidNode_ [Store(D)]

_MaidNode_ =>> |__MaidManager__ [Allow ? So : PutFailure]  *->> |__DataManager__  [Exist(D) ? PutResponse : {(Store(D.name))({so.To(D.name), so.To(#D.name), so.To(##D.name), so.To(###D.name)}), PutResponse}] *->> |_PmidManager_ {!ClosestToTarget ? {StoreClosestAsHolder, so.Store(D)} *-> |_PmidNode_ Store(D)


< MaidNode::Put | MaidManager::HandlePut | PmidManagers{H^n(D)}::Store | PmidNode{H^n(D)}::Persist >

< MaidNode::Put<ImmutableData>(unsignedD) {
     D = SignWithMaid(unsignedD)
     routing.put<ImmutableData>(D) // }
 | MaidManager::HandlePut<ImmutableData>(D) {
     Allow ? ChargeCost(K*M*D.size()); return Flow [ PmidManager{H(D)}, Store(D) ] : return Error [ error::OutOfCredit ] }
 | PmidManager{H(D)}::Store<ImmutableData>(D) {
      if Exists( n <= K : H^n(D) == our_group_address ) then
         Register(D.name, n,  LookupMClosestPmidNodesInRTtoHashOfHash)
         return Flow [ [ PmidManager{H^2(D)}, Store(D) ],  // forward to next group
                              [ MaidManager{MaidNode}, ConfirmCost(cost)]  // confirm actual cost, discounted by already registered storage
			      [ PmidNode{LookupMClosestPmidNodesInRTtoHashOfHash(0)}, Persist(D)], …,  // persist in this group
			      [ PmidNode{LookupMClosestPmidNodesInRTtoHashOfHash(M)}, Persist(D)] ]      // on M PmidNodes
      else
         return Error [ error::EndOfDuplicationChain] 
      endif }
 | PmidManager{H^n(D)}::Store<ImmutableData>(D) { /* identical to above */}
      //  collapsed notation for chain of K redundant copies of PmidManager groups 
      //  < … | PmidManager{H(D)}::Store(D) | PmidManager{H^2(D)}::Store(D) | … | PmidManager{H^K(D)}::Store(D) | ForEach PmidNode::Persist >
 | PmidNode::Persist<ImmutableData>(D) {
      PersistInVault(D)
      on_error return Error [ error::FailedToStoreInVault ]
      return Flow [ ] } >

Additional flows, without implementation:

CONFIRM_COST
| PmidManager::Store(D) | MaidManager::ConfirmCost(cost_from_each_pmid_manager_group) | MaidNode::PutCompleted(total_cost) >

Highlighted error handle

PmidManager::HandleError<FailedToStoreInVault> (not really templated, but it’s 5am by now)

legend:
 <      scatter
 >      gather
 |       sentinel
 D     data
 H()   Hash512
 H^n() n-th Hash512
 ManagersGroup{Address};  {Address} omitted where evident, e.g. MaidManagers{MaidNode}
