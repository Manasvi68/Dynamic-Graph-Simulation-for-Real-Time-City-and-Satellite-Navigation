#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <string>
#include <vector>

using namespace std;

// represents a single change to the network (like a transaction in Bitcoin)
struct NetworkEvent {
    string type;        // "CONGESTION", "ROAD_CLOSED", "ROAD_ADDED", "SAT_LINK_UP", "SAT_LINK_DOWN"
    string fromNode;    // which intersection
    string toNode;      // which intersection
    double oldWeight;   // previous travel time 
    double newWeight;   // new travel time 
    string timestamp;   // when this happened
};

// a single block in the chain — holds one event plus hash links
struct Block {
    int index;              // position in the chain
    string data;            // JSON string of the NetworkEvent
    string previousHash;    // hash of the block before this one
    string hash;            // this block's own hash
    string timestamp;       // when block was created
};

class Blockchain {
private:
    vector<Block> chain;    // the actual blockchain

    // our simple hash function
    string simpleHashString(string input) const;

    // computes the hash for a block based on its contents
    string computeBlockHash(int index, string data, string prevHash, string timestamp) const;

    // creates the very first block 
    Block createGenesisBlock();

public:
    Blockchain();

    // add a new event to the blockchain
    void addBlockFromEvent(NetworkEvent event);

    // verify the entire chain hasn't been tampered with
    bool isChainValid() const;

    // save/load the blockchain as JSON
    void saveToJson(string filename) const;
    void loadFromJson(string filename);

    // get all blocks (for server API)
    const vector<Block>& getChain() const;
};

#endif