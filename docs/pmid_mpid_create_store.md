##Introduction 

In an XOR network, it's desirable to have uniform distribution of nodes.  However, using 100% random algorithm to define node placement will likely cause localised uneven distribution.

A node's placement will be defined by the name of its PMID key.

##Motivation

The ability to arbitrarily select a node's placement on the network is a security concern. It also makes establishing population difficult as well as potentially creating mass imbalance if these adresses are not uniformly distributed.

Futhermore, clustering of PMIDs will mean that groups of PMID managers will be managing dissimilar numbers of accounts.

So it would be desirable to try and avoid "bunching" of PMIDs, in other words, to try and force an even distribution by limiting the closeness of PMIDs to eachother.  This can also be viewed as restricting the number of leading bits in common between two PMIDs.

The same requirement would work well with regards to the MPIDs too, although for slightly different reasons.

The MPIDs define the location of the user's public ID (similar to traditional email address), so as with PMIDs it's desirable to distribute the workload of managing MPID accounts evenly between vaults.  However, less of an issue here is the security concern, since nodes don't join the network under the MPID identity.  The main motivation for limiting the number of leading common bits of MPIDs is rather to allow humans to easily enter what will be a random alpha-numeric string to unambiguously identify their MPID.

The original design for MPID name was the SHA512 of the chosen (human-readable) public name.  However, we want to avoid name-sqatting, and also allow multiple users to choose the same public name.

This means that the MPID can be named as per the other MaidSafe keys - as the hash of its entire contents - making it self-validating.  If we disallow more than a certain number of common leading bits between MPIDs, users only need to enter a few characters of the full ID to find the correct MPID.

##Overview

###Creating PMID

A new PMID will be created by the VaultManager (the daemon/service which is responsible for launching and stopping Vault processes), not the Vault itself.

1. The VaultManager will send a RegisterPmid to the group closest to the current PMID name (hash of [public key + signature]) - say Group1.
2. Group1 will then sync the request, modify the PMID to contain the XOR of the 4 group member's IDs, and send a RegisterPmid to the new group address (hash of [pubkey + sig + xor_value]) - say Group2.
3. Group1 reply with the modified PMID to the VaultManager.
4. Once the VaultManager has received all replies, it starts the new Vault, giving it the updated PMID.
5. The new Vault sends a signed PutPmid group request (will go to Group2).

Group2 have to validate at each stage.  When receiving the initial RegisterPmid from Group1, they must ensure that the XOR value in the PMID corresponds to the XOR of the 4 sender's IDs.  They must also ensure that the new PMID only exceeds the max common leading bits between any of the current group by 1.

Then when they receive the PutPmid from the new Vault, they must check that the sender's ID == the modified PMID name and that the request's signature validates using the public key of the PMID.

###Creating MPID


##Implementation
