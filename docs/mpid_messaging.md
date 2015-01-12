MPID Messaging system
=========

Design document 1.0

Introduction
============

In MaidSafe the ability for secured messaging is obvious and may take many forms, mail like, IM like etc. This document outlines the system componenets and design for general communications infrastructure and security. 

Motivation
==========

The motivation for a messaging system is an obvious one, but a wider motivation is, as always in MaidSafe, a messaging system that is secure and private without the possibility of snooping tracking or being abused in any way. A large abuse of modern day digital communications is the ability for immoral entities such as spammers and the less than ethical marketing companies to flood the Internet with levels of unsolicited email to a level of circa 90%. This is a waste of good bandwidth and also a nuisance of significant proportions.

A MaidSafe messaging system will attempt to eradicate unwanted and intrusive communications. This would seem counter intuative to a network that purports to be more efficient, cheaper and faster than today's mechanisms. The flip side of this is the network's ability to work autonomously and follow pre-programmed rules. With such rules then the network can take care of people's inboxes and outboxes for incoming and outgoing mail respectively. 

This design outlines a mechanism where the cost of messaging is with the sender as this would seem more natural. To achieve this, the sender will maintain messages in the network outbox until they are retrived by the recipient. If the email is unwanted the recipient simply does not retrieve the message. The sender will quickly fill their own outbox with undelivered mail and be forced to clean this up, themselves.

This paradigm shift will mean that the obligation to un-subscribe from mailing lists etc. is now with the owner of these lists. If people are not picking up mail, it is because they do not want it. So the sender has to do a better job. It is assumed this shift of responsibilities will lead to a better managed bandwidth solution and considerably less stress on the network and the users of the network.

Overview
========

A good way to look at the solution is that, rather than charging for unwanted mail with cash, the network charges with a limited resource and then prevents further abuse. In another apsect this is regulation of entities by the users of the system affected by that entity. Rather than build a ranking system to prevent bad behaviour, this proposal is actually the affected people acting independently. This protects the minorites who may suffer from system wide rules laid down by any designer of such rules. 

Network OutBox
--------------

This is a simple data structure for now and will be a ```std::map``` ordered by the hash of the serialised and encrypted ```MpidMessage```  and with a user defined object to represent the message (value). The map will be named with the ID of the MPID it represents (owner). The data structure for the value will be 

```c++
struct MpidMessage {
  PublicMpid::Name sender;
  PublicMpid::Name recipient;
  std::string message_head, message_body;
  Identity id, parent_id;
};

```

It needs to be highlighted that each above MpidMessage only targets one recipient. When a sender sending a message to multiple recipients, multiple MpidMessages will be created in the ```OutBox``` . This is to ensure spammers will run out of limited resource quickly, so the network doesn't have to suffer from abused usage.

Network Inbox
-------------

The network inbox is an even simpler structure and will be again named with the MpidName of the owner. This can be represented via a ```std::vector<MpidAlert>```

```c++
struct MpidAlert {
  Identity message_id;
  PublicMpid::Name sender;
};
```

Message Flow
------------
```
        MpidManagers (A)                           MpidManagers (B)
           /  *                                    * \
Mpid (A) -> - *                                    * - <-Mpid (B)
           \  *                                    * /

```
1. The user at Mpid(A) sends MpidMessage to MpidManager(A) signed with the recipient included
2. The MpidManagers(A) sync this message and perform the action() which sends the MpidAlert to MpidManagers(B) [the ```MpidAlert::message_id``` at this stage is simply the hash of the MpidMessage.
3. MpidManager(B) either store the MpidAlert or send immediately to the Mpid(B) user if they are off-line or on-line respectively.
4. Mpid(B) then sends a ```Signed(retrieve_message)``` to the MpidManager(B) group who send this on to the MpidManagers(A). This message is of the form ```retrieve_message(MpidAlert, MpidPacket)[signed by Mpid(B)]``` 
5. MpidManagers(A) then sync() the MpidAlert and confirm this is from the MpidManager(B) group, do a signature check and then perform action() which sends the message to MpidManagers(B) and remove the message.
6. MpidManager(B) then sync() the message to confirm it was delivered from MpidManagers(A) and send the message to Mpid(B), or store for later retieval if the node has gone off-line. 
7. When Mpid(A) decided to remove the MpidMessage from the OutBox, when the message hasn't got retrived by Mpid(B). The MpidManagers(A) group needs not only remove the correspondent MpidMessage from their OutBox of Mpid(A), but also send a notification to the group of MpidManagers(B) so they can remove the correspodent MpidAlert from their InBox of Mpid(B).
8. When Mpid(B) send a request to fetch message head only, it will directly goes to MpidManagers(A) and get response. This will not trigger the removal of such message in MpidManagers(A).

MPID Messaging Client
--------------
The messaging client, as described as Mpid(X) in the above section, can be named as nfs_mpid_client. It shall provide following key functionalities :

1. Send Message (Put from sender)
2. Retrieve Full Message (Get from receiver)
3. Retrieve Message Head only (Get from receiver)
4. Remove sent Message (Delete from sender)
5. Accept Message Alert (when ```PUSH``` model used) and/or Retrive Message Alert (when ```PULL``` model used)

When ```PUSH``` model is used, nfs_mpid_client is expected to have it's own routing object (not sharing with maid_nfs). So it can connect to network directly allowing the MpidManagers around it to tell the connection status directly.

Such seperate routing object is not required when ```PULL``` model is used. It may also have the benefit of saving the battery life on mobile device as the client app doesn't need to keeps nfs_mpid_client running all the time.

Future Works
============

This proposal implements a container as a std::map, it is assumed this will fall over to become a Structured Data Version ([SDV](https://github.com/maidsafe/MaidSafe-Common/blob/next/include/maidsafe/common/data_types/structured_data_versions.h)) when/if SDV is able to insert/delete single elements in a branch (possibly doubly linked list type). This is considered premature optimisation at this stage of development and requires measuring of the performance/size hit on adding two pointers per node. 
