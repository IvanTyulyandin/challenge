//
// Created by ivan on 29.04.18.
//

#include "reader.h"

#include <iostream>

/*
using edgeInfo = pair<int, string>;
using adjacencyType = vector<vector<edgeInfo>>;
using startStatesType = map<int, vector<string>>;
using finalStatesType = map<string, vector<int>>;
 */

namespace {
    void printAutomation(const adjacencyType & adjVec) {
        for (auto i = 0; i < adjVec.size(); ++ i) {
            for (auto & iter : adjVec[i]) {
                for (auto & iterVec : iter.second) {
                    cout << i << " " << iter.first << " " << iterVec << endl;
                }
            }
        }
    }

    template <typename FST, typename SND>
    void printStates(map<FST, vector<SND>> & statesInfo) {
        for (auto & it : statesInfo) {
            cout << it.first << ": ";
            for (auto & itMapped : it.second) {
                cout << itMapped << " ";
            }
            cout << endl;
        }
    };
}

int main() {
    adjacencyType RFA;
    startStatesType start;
    finalStatesType final;
    int numOfStatesRFA;
    readRFA("testRFA/Q1RFA.txt", RFA, start, final, numOfStatesRFA);

    adjacencyType DFA;
    int numOfStatesDFA;
    readDFA("data/atom-primitive.dot", DFA, numOfStatesDFA);

    printAutomation(DFA);
}