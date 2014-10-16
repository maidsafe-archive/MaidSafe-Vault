# PMID Creation

## Introduction 

In an XOR network, it's desirable to have uniform distribution of nodes.  However, using 100% random algorithm to define node placement will likely cause localised uneven distribution.

A node's placement will be defined by the name of its PMID key.

## Motivation

The ability to arbitrarily select a node's placement on the network is a security concern. It also makes establishing population difficult as well as potentially creating mass imbalance if these adresses are not uniformly distributed.

Futhermore, clustering of PMIDs will mean that groups of PMID managers will be managing dissimilar numbers of accounts.

So it would be desirable to try and avoid "bunching" of PMIDs, in other words, to try and force an even distribution by limiting the closeness of PMIDs to eachother.  This can also be viewed as restricting the number of leading bits in common between two PMIDs. This design is for the PMIDNode in particular as the manager nodes will take on a new identity on each network connect.

The same requirement would work well with regards to the MPIDs too, although for slightly different reasons.

The MPIDs define the location of the user's public ID (similar to traditional email address), so as with PMIDs it's desirable to distribute the workload of managing MPID accounts evenly between vaults.  However, less of an issue here is the security concern, since nodes don't join the network under the MPID identity.  The main motivation for limiting the number of leading common bits of MPIDs is rather to allow humans to easily enter what will be a random alpha-numeric string to unambiguously identify their MPID.

The original design for MPID name was the SHA512 of the chosen (human-readable) public name.  However, we want to avoid name-sqatting, and also allow multiple users to choose the same public name.

This means that the MPID can be named as per the other MaidSafe keys - as the hash of its entire contents - making it self-validating.  If we disallow more than a certain number of common leading bits between MPIDs, users only need to enter a few characters of the full ID to find the correct MPID.

## Overview

A new PMID and ANPMID will be created by the VaultManager (the daemon/service which is responsible for launching and stopping Vault processes), not the Vault itself.  However, they will be passed to the Vault to be put to the network - the VaultManager doesn't connect to the network at all.  First, the ANPMID will be Put to the network by the Vault.  Then the Vault will give the PMID to the network to be modified, after which it's passed back to the Vault  to be signed by the ANPMID.  Finally the Vault reconnects under the modified ID and Puts the PMID.

## Implementation

The structure of the PMID will be:

| Name - SHA512(1 and 2 and 3)                             |
| -------------------------------------------------------- |
| 1 - PMID Public Key                                      |
| 2 - Collection of `<PublicPmid::Name, Rnd #, Signature>` |
| 3 - Signature of (2 and 3) using ANPMID                  |

The steps to create and store the PMID are:

1. The VaultManager creates the initial ANPMID and PMID keys and passes these to the newly-started Vault via IPC.
2. The Vault joins the network with the ID of SHA512(unsigned PMID Public Key) - say "ABC".  It should keep a temporary container of all the PublicPmids of the nodes in its close group so that these can later be sent (step 8 below).
3. The Vault sends a Put(ANPMID) request.
4. The Vault sends a RegisterPmid to the group closest to "ABC" - say Group1.  The request contains the PMID Public Key only.
5. Each node in Group1 checks the SHA512 of the Public Key matches the Group ID the message was sent to, i.e. "ABC" (if not, fail).  Then they send a response containing: `<PublicPmid::Name, Rnd #, Signature>`.  The `PublicPmid::Name` is their own NodeID, the `Rnd #` is a random number they generate for this request, and the signature is of these 2 items concatenated with "ABC" (signing key is their PMID).
6. The original Vault gathers all responses and adds each `<PublicPmid::Name, Rnd #, Signature>` to the collection in the PMID.  Once all (enough?) responses are gathered, the PMID Public Key concatenated with the collection is signed using the ANPMID.  The PMID is now complete.
7. The Vault disconnects and reconnects using its new ID (the SHA512 of its PMID contents).
8. The Vault sends a Put(PMID) request (which should be signed by the PMID private key) to the DataManagers (say Group2) for this PMID.  This happens to be the group it's now connected to.  The request should also contain the PublicPmids of each of the members of Group1 (temporarily cached in step 2 above).
9. Each node in Group2 have to do validation (if the PMID parses without throwing, we assume the Name == Hash(Contents)):
    - Check the signature of the request
    - Check the collection of `PublicPmid::Name`s is a real close group to SHA512(unsigned PMID Public Key) (i.e. "ABC")
    - Check the signature of each entry in the collection
    - As for all keys being stored, check that the new PMID satisfies the max common leading bits check

In addition the rank shall be obtained (this is part of nodeinfo) and the ID only accepted if the rank of the vault brings the close group closer to the average of all the ranks of the routing table. i.e. if group's rank is higher than average the new node must have a lower rank and vice versa.

If any of the steps including or following step 3 fail, the Vault needs to send an IPC to the VaultManager or exit with a particular error code which tells the VaultManager to not just restart the Vault, but to issue new PMID/ANPMID keys.
