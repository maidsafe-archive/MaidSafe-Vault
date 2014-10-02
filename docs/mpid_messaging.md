MPID Messaging system
=========

Design document 1.0

Introduction
============

In MaidSafe the ability for secured messaging is obvious and may take many forms, mail like, IM like etc. This document outlines the system componenets and design for general communications infrastructure and security. The assumption is that all personal communications are only handled by an Structured Data Version ([SDV]) type container. This is described in Future Work section. 

Motivation
==========

The motivation for a messaing system is an obvious one, but a wider motivation is as always in MaidSafe a messaging system that is secure and private withouth the possibiliyt of snooping trakcing or being abused in any way. A large abuse of modern day digital communications is the ability for imorral entities such as spammers and the less than ethical marketing companies to flood the Internet with levels of unsolicitted email to a level of circa 90%. This is a waste of good bandwidth and also a nusiance of significant proportions. 

A MaidSafe messaging system will attempt to eradicate unwanted and intrusive communications. This would seem counter intuative to a network that proports to be more efficient, chearper and faster than todays older mechanisms. The flip side of this is the networks ability to work autonomously and follow pre programmed rules. With such rules then the network can take care of peoples inbox and outbox for incoming and outgoing mail respectively. 

This design outlines a mechanism where the cost of messaging is with the sender as this would seem more natural. To achieve this is sender will maintain messages in the network outbox until they are retrived by the recipient. If the email is unwanted the recipient simply does not retrieve the message. The sender will quickly fill their own outbox with undelivered mail and be forced to clean this up, themselves. 

This paradigm shift will mean that the obligation to un-subscribe form mailing lists etc. is now with the owner of these lists, if people are not picking up mail, it is because they do not want it. So the sender has to do a better job. It is assumed this shift on responsibilities will lead to a better managed bandwidth solution and considerably less stress on the network and th eusers of the network.

Overview
=========

A good way to look at the solution is that, rather than charging for unwanted mail with cash, the network charges with a limited resource and then prevents further abuse. In another apsect this is regulation of entities by the users of the system affected by that entity. Rather than build a ranking system to prevent bad behaviour, this proposal is actually the affected people acting independently. This protects the minorites who may suffer from system wide rules laid down by any designer of such rules. 

Network OutBox
--------------

This is a simple data structure for now and will be a ```std::map``` ordered by the hash of the serialised and encrypted ```MpidMessage```  and with a user defined object to represent the message (value). The map will be named with the ID of the MPID it represents (owner). The data structure for the value will be 

```c++
struct MpidMessage {
PmidName Sender;
std::vector<<PmidName>> recipients;
std::string message;
std::string body;
Identity id;
Identity parent_id;
};

```

Network Inbox
-------------

The network inbox is an even simpler structure and will be again named with the PmidName of the owner. This can be represented via a ```std::vector<MpidAlert>```

```c++
struct MpidAlert {
Identity message_id;
Identity sender;
};
```
