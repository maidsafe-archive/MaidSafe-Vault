#PMID (storage_node manager) account

##Naming Conventions

PmidNode  -> is now referred to as ```storage_node``` 
PmidManager -> is now referred to as ```storage_node_manager```

##Introduction

The ```storage_node``` is the network vault and is managed by the ```storage_node_manager``` group on connecting. The ```storage_node_manager``` group is the group the ```storage_node``` manager will connect to an this group will manage many things like data stored, safecoin farming attempt management amongst others. This group can also de-rank or remove a ``storage_node``` who has failed to perform as expected by the network.

##Motivation

To allow the network to expand at the fastest rate then as many nodes as possible need to be considered as routing nodes. At the moment this is not possible, due to the size of account information. The ```storage_node``` has been a huge amount of data, to allow recordign of on and off line nodes, required a mechanism to message data managers who may be responsible for the data.

This mechanism leaves the network open to an attack known as a traffic amplification attack, where a node could store a lot of info and then create chruin, by switching off and on quickly. It also means that the network account is too large for small devices to transfer in the event of churn.

##Overview

As ```storage_nodes``` are now selected in a mor deterministic manner (being selected from the close group), then these nodes are also connected to the ```data_managers```. As the ```data_managers``` are also connected to the ```storage_nodes``` the can tell these nodes are off line or not. The ```data_managers``` already know what chunks each ```storage_node```is responsible for (as it always did) and therefor there is no need for a ```storage_node_manager``` to hold anything more than ```data_stored``` and ```data_lost``` which can be two 64 bit integers. This reduces the account to a minimal value. 


##Implementation
