###Terms and conventions

* ->> : represents a single to group message (no callback)
* -> : represents a single to single message (no callback)
* =>> : represents a single to group message (callback)
* => : represents a single to single message (callback)
* *->> : represents a group to group message (no callback)
* *-> : represents a group to single message (no callback)
* *=>> :represents a group to group message (callback)
* *=> : represents a group to single message (callback)
* So - Send On, this function which sends the message on to the next persona is implicitly mentioned in all above notations. They can be explicitly mentioned, when it matters
* Action.Sy : denotes synchronising the Action
* Ac - Accumulate, This function accumulates messages from previous 
* Fw - Firewall, this function ensures duplicate messages are prevented from progressing and answers such messages with the calculated response (such as success:error:synchronising etc. and may include a message containing data)
* | : represents [Ac, FW]
* [*Operation1*, *Operation2*, .. *OperationN*] : denotes sequential operations
* {*Operation1*, *Operation2*, .. *OperationN*} : denotes parallel operations
* (*Operation1*)(*Operation2*) .. (*OperationN*) : denotes sequential dependent operations, for instance, *Operation2* requires results from *Operation1* and produces results for *Operation3*
* condition ? A : B : C/C++ syntax
* condition ? A : represents _condition ? A : NoOp_
* ! denotes NOT
* __Bold__ represents Indirect Network Addressable Entities INAE
* _Italic_ represents Direct Network Addressable Entities DNAE
