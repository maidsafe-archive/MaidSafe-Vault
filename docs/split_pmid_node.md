#PmidNode split to become client object

##Introduction

There is an attack on the system that is very hard to overcome. This is a colluding attack where people share their PMID address in return for cash or simply destruction. This attack seems impossible to overcome, where folk publish their PMID address and groups identified (collusion attack). The known address (pmid) will be fixed and open to this attack, but this will only be for data held at that node. As data is self validating then this attack becomes of very little value and can be considered temporary vandalism or denial of service on a chunk. There are several locations per chunk and this is a very expensive operation to deny access to a random chunk (if at all). 

The Manager personas will use a different routing obnject and will be forced to create temporary keys for the network, these will be altered on request by the close nodes and will therefor randomise the location of these manager groups. 

##Naming Conventions

pmidNode -> is now referred to as network_worker_node 

##Motivation

The motivation here is twofold. To overcome any collusion atttack adn also to seperate a client type persona from the manager type personas. This is a more natural approach as the network_worker_node is a single type (i.e. not a group as all managers are). These types have particular requirements and are managed types. It seems more natural they have their own routing object and manage that themselves. This way the manager nodes are 100% group message handlers, i.e. every message they recieve is syncronised to their group, agreed, acted on and then forwarded on. The network_worker_node on the other hand acts independet of any other nodes in a group and responds to requests from a manager group.  

##Overview
 

##Assumptions

##Implementation



