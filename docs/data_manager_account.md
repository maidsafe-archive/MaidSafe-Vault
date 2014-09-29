#Data Manager account

##Naming Conventions

MaidClient  -> is now referred to as ```storage_client``` 
MaidManager -> is now referred to as ```storage_client_manager```
PmidNode  -> is now referred to as ```storage_node``` 
PmidManager -> is now referred to as ```storage_node_manager```

##Introduction

The ```data_manager``` is the persona that is responsible for data avalibility and integrity. To do so these personas must know where the data currenly resides and the state of that data. This state is twofold, is the ```storage_node``` on-line and is the data itself intact and secure. The former is arguably the main issue as the data itself is self-validating (i.e. to construct an immutable chunk itself validates the content). To ensure this, a ```data_manager``` mut know which nodes hold the data, whether they are currently on or off line. These are currently the most data heavy personas and under constant scrutiny to reduce the amount of data they hold.

##Motivation

The motiviating factor for re-factoring of this persona is efficiency, unlike the other personas the data manager has currently no easy win in terms of reducing the amount of information the persona holds. There is a very valid opportunity though via a better holding mechnaism (via sqlite) and also the length of the keys held. So there are two factors involved in this case, more efficient engine for managing the data and also the size of the data we are managing. 

##Overview
The hear of the ```data_manager``` will be a mini SQL database. Thsi will be managed via sqlite3. The table structure will be very simple.

ChunkName | ```storage_node``` |

Each chunkname will have associated ```storage_nodes``` and these nodes will have a status, on or off line. This status is held in the routing table of the data_manager already and need not be replicated in the database. 

On a churn event the SQL database should be searched for every chunk that old node had and joined with what chunks the new nodes has. Any chunk with less than 4 holders should now be stored to the node in the group with the least rank. If a record now conatins more than 8 nodes the 9th node shall be downranked and a delete message sent ot the ```storage_node_manager``` of that node. 

Records of nodes holding a chunk should be held in a manner that as a new node comes on line that it is put to the top of the list. 


##Implementation
