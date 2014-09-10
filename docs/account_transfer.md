#Account Transfer

##Introduction

The purpose of account transfer is to allow the network nodes to maintain the distributed state of the network. This state is held primarily as a key value store for various personas. As
nodes go on and off line the network reconfigures around it and all state held in the group around the node has to be reconfigured. The current network implementation maintains 4 accurate
copies of all state per address (address being a node or data element). To accurately manage 4 copies a network range for the 16 close nodes must be considered due to uni-directional
closeness. This means that to ensure we have the 4 closest nodes we much consider at least 16 nodes surrounding any address. 

##Naming Conventions

Proximity Group - the 16 nodes close to an address
Close Group - the 4 nodes containing accurate information on any address.


##Motivation

The motivation behind this design is to provide an efficient mechanism to transfer accounts data between nodes, whilst reusing much of the existing design patterns as possible. As the values
for all data for the various personas are potentially different then there is a lean towards forcing account data values to be synchronise-able. This really means the account value data should be consistent and not prone to any consensus. An example of bad values would be measurements that may vary across keys. These varying values must me at the very least minimised.or better removed. The current proposal is to allow account transfer to be as fast as possible and with minimum network traffic cost. 

##Overview

Account transfer resembles network sync with a noticeable exception that there is at least one node less to consider. By definition a node has altered, by either disappearing or indeed
possibly joining. So worst case scenario there will be a maximum of three other nodes with the info we require. An account transfer packet be delivered to a node from another node in this
proximity group of 16 at any time based on any churn at all in the proximity group.

The routing library will present an interface that allows upper layers to query any changes to the proximity group. This is defined in   

##Implementation

The implementation of the account transfer is presented in pseudo code below

```
struct Record {
  Identity key;
  NonEmptyString value;
};


std::vector<Nodes> proximity_group;
std::vector<Record> all_account_info;
std::pair<Node, Node>; first = new node second = old node 
```

