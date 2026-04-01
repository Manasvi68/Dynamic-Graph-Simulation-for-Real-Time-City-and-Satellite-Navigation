#include "blockchain.h"
#include "json.hpp"
#include <iostream>
#include <fstream>
#include <ctime>

using namespace std;
using json = nlohmann::json;

// simple hash function — NOT cryptographically secure, but demonstrates the concept
// takes a string and produces a hexadecimal "fingerprint"
string Blockchain::simpleHashString(string input) const {
    unsigned long hash = 5381;  // starting value (djb2 algorithm by Dan Bernstein)

    for (int i = 0; i < input.length(); i++) {
        // hash = hash * 33 + character
        hash = ((hash << 5) + hash) + input[i];
    }

    // convert to hex string
    string hexResult = "";
    const char hexDigits[] = "0123456789abcdef";
    for (int i = 0; i < 16; i++) {
        hexResult += hexDigits[hash % 16];
        hash /= 16;
    }

    return hexResult;
}

// compute hash for a block by combining all its fields into one string
string Blockchain::computeBlockHash(int index, string data, string prevHash, string timestamp) const {
    string combined = to_string(index) + data + prevHash + timestamp;
    return simpleHashString(combined);
}

// the genesis block — the very first block in the chain
Block Blockchain::createGenesisBlock() {
    Block genesis;
    genesis.index = 0;
    genesis.data = "Genesis Block";
    genesis.previousHash = "0000000000000000";
    genesis.timestamp = "2025-01-01T00:00:00";
    genesis.hash = computeBlockHash(genesis.index, genesis.data,
                                     genesis.previousHash, genesis.timestamp);
    return genesis;
}

// constructor — automatically creates the genesis block
Blockchain::Blockchain() {
    chain.push_back(createGenesisBlock());
}

// add a new event to the blockchain
void Blockchain::addBlockFromEvent(NetworkEvent event) {
    Block newBlock;
    newBlock.index = chain.size();

    // serialize the event to JSON for storage
    json eventJson;
    eventJson["type"] = event.type;
    eventJson["from"] = event.fromNode;
    eventJson["to"] = event.toNode;
    eventJson["oldWeight"] = event.oldWeight;
    eventJson["newWeight"] = event.newWeight;
    eventJson["timestamp"] = event.timestamp;
    newBlock.data = eventJson.dump();

    // link to previous block
    newBlock.previousHash = chain.back().hash;

    // generate timestamp
    time_t now = time(0);
    newBlock.timestamp = ctime(&now);
    // remove trailing newline that ctime adds
    if (!newBlock.timestamp.empty() && newBlock.timestamp.back() == '\n') {
        newBlock.timestamp.pop_back();
    }

    // compute this block's hash
    newBlock.hash = computeBlockHash(newBlock.index, newBlock.data,
                                      newBlock.previousHash, newBlock.timestamp);

    chain.push_back(newBlock);
}