#Persona Account Transfer

To offer a reliable information, on each churn managing personas must update relevant nodes with appropraite account information. Following presents information required to be transferred on account transfer by managing personas.


##Storage client manager
Storage client manager keeps records about the storage clients it is responsible for. The information kept represent the amount of network space the storage client is entitled to.

| Storage client id | Available space | Stored space |
| ------------------| --------------- | ------------ |


##Data manager
Data manages holds account information about chunks it is responsible for. The account information for a chunk represent the chunk name and the id and status of nodes storing the chunk.

| ChunkName | `storage nodes info`s |
| --------- | --------------------- |

storage node info has id and the status (online / offline) for the node storing the chunck.


##Storage node manager

Storage node manager holds account information about `storage nodes` it is responsible for. The account information for a storage node represent the id of the storage node along with store success/failure statistics related to that storage node.

| Storage node id | Claimed available size | Stored total size | Stored count | Lost total size | Lost count |
| ----------------| ---------------------- | ----------------- | ------------ | --------------- | ---------- |
