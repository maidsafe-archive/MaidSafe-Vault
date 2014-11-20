###Terms and Conventions Used

* ->> : represents a single to group message (no callback)
* -> : represents a single to single message (no callback)
* =>> : represents a single to group message (callback)
* => : represents a single to single message (callback)
* *->> : represents a group to group message (no callback)
* *-> : represents a group to single message (no callback)
* *=>> :represents a group to group message (callback)
* *=> : represents a group to single message (callback)
* Action.Sy : denotes synchronising the Action
* | : represents [Ac, FW]
* [*Operation1*, *Operation2*, .. *OperationN*] : denotes sequential operations
* {*Operation1*, *Operation2*, .. *OperationN*} : denotes parallel operations
* (*Operation1*)(*Operation2*) .. (*OperationN*) : denotes sequential dependent operations, for instance, *Operation2* requires results from *Operation1* and produces results for *Operation3*
* condition ? A : B : C/C++ syntax
