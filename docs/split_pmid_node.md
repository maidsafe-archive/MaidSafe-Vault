#PmidNode split to become client object

##Introduction

There is an attack on the system that is very hard to overcome. This is a colluding attack where people share their PMID address in return for cash or simply destruction. This attack seems impossible to overcome, where folk publish their PMID address and groups identified (collusion attack). The known address (pmid) will be fixed and open to this attack, but this will only be for data held at that node. As data is self validating then this attack becomes of very little value and can be considered temporary vandalism or denial of service on a chunk. There are several locations per chunk and this is a very expensive operation to deny access to a random chunk (if possible at all). 


##Naming Conventions

pmidNode -> is now referred to as ```storage_node``` 
PmidManager, DataManager, MaidManager, MpidManager are collectively know as ```manager_node```

##Motivation

The motivation here is twofold. To overcome any collusion atttack and also to seperate a client type persona from the manager type personas. This is a more natural approach as the ```storage_node``` is a single type (i.e. not a group as all managers are). These types have particular requirements and are managed types. It seems more natural they have their own routing object and manage that themselves. This way the manager nodes are 100% group message handlers, i.e. every message they recieve is syncronised to their group, agreed, acted on and then forwarded on. The ```storage_node``` on the other hand acts independant of any other nodes in a group and responds to requests from a manager group. It may respond to signed requests for any message recieved and signed by the ```storage_node's``` key.  

##Overview

The Manager personas will use a different routing object and will be forced to create temporary keys for the network, these will be altered by the close nodes and will therefore randomise the location of these manager groups. This temporary key will be deleted when this node goes off line. The close nodes will store the ```manager_node``` key on the network close to the keys ID. The close group will store this key in a ```manager_node``` container and remove the key when the node goes off line. 

##Implementation




