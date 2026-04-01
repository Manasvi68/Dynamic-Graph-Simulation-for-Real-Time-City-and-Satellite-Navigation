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

bool Blockchain::isChainValid() const {
    // empty chain isn't "invalid" (though your constructor always creates genesis)
    if (chain.empty()) return true;

    // check genesis block separately
    const Block& genesis = chain[0];
    if (genesis.index != 0) {
        cout << "Genesis block index is not 0! Chain is corrupted." << endl;
        return false;
    }
    if (genesis.previousHash != "0000000000000000") {
        cout << "Genesis block previousHash is invalid! Chain is corrupted." << endl;
        return false;
    }

    string genesisCheck = computeBlockHash(genesis.index, genesis.data, genesis.previousHash, genesis.timestamp);
    if (genesis.hash != genesisCheck) {
        cout << "Genesis block hash mismatch! Chain is corrupted." << endl;
        return false;
    }

    // check every block after genesis
    for (int i = 1; i < (int)chain.size(); i++) {
        const Block& current = chain[i];
        const Block& previous = chain[i - 1];

        if (current.index != i) {
            cout << "Block " << i << " index mismatch! Chain is corrupted." << endl;
            return false;
        }

        // recalculate this block's hash from its data
        string recalculated = computeBlockHash(current.index, current.data, current.previousHash, current.timestamp);

        // does the stored hash match what we just calculated?
        if (current.hash != recalculated) {
            cout << "Block " << i << " hash mismatch! Data was tampered with." << endl;
            return false;
        }

        // does this block's previousHash match the actual previous block's hash?
        if (current.previousHash != previous.hash) {
            cout << "Block " << i << " previous hash doesn't match block " << (i - 1) << "!" << endl;
            return false;
        }
    }

    return true;  // all checks passed
}

void Blockchain::saveToJson(string filename) const {
    json j;
    j["chain"] = json::array();

    for (int i = 0; i < (int)chain.size(); i++) {
        const Block& b = chain[i];
        json blockJson;
        blockJson["index"] = b.index;
        blockJson["data"] = b.data;
        blockJson["previousHash"] = b.previousHash;
        blockJson["hash"] = b.hash;
        blockJson["timestamp"] = b.timestamp;
        j["chain"].push_back(blockJson);
    }

    ofstream outFile(filename);
    if (!outFile.is_open()) {
        cout << "Error: couldn't open " << filename << " for writing." << endl;
        return;
    }
    outFile << j.dump(2);
    outFile.close();
}

void Blockchain::loadFromJson(string filename) {
    ifstream inFile(filename);
    if (!inFile.is_open()) {
        cout << "Couldn't open " << filename << ", keeping existing chain." << endl;
        return;
    }

    json j;
    inFile >> j;
    inFile.close();

    if (!j.contains("chain") || !j["chain"].is_array()) {
        cout << "Invalid blockchain file format, keeping existing chain." << endl;
        return;
    }

    vector<Block> loaded;
    for (auto& blockObj : j["chain"]) {
        Block b;
        b.index = blockObj.value("index", 0);
        b.data = blockObj.value("data", "");
        b.previousHash = blockObj.value("previousHash", "");
        b.hash = blockObj.value("hash", "");
        b.timestamp = blockObj.value("timestamp", "");
        loaded.push_back(b);
    }

    if (loaded.empty()) {
        cout << "Loaded chain is empty, keeping existing chain." << endl;
        return;
    }

    chain = loaded;
}

const vector<Block>& Blockchain::getChain() const {
    return chain;
}