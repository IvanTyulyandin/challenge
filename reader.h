//
// Created by ivan on 29.04.18.
//

#pragma once

#include <vector>
#include <string>
#include <map>

using namespace std;

using adjacencyType = vector<map<string, vector<int>>>;
using startStatesType = map<int, vector<string>>;
using finalStatesType = map<string, vector<int>>;

struct edge {
    int from;
    int to;
    string by;

    edge() = default;

    edge(int fromState, int toState, string & bySymb)
            : from(fromState), to(toState), by(bySymb) {}
};

void readRFA(const string & fileName,
             adjacencyType & adjVec,
             startStatesType & startStates,
             finalStatesType & finalStates,
             int & numOfStates);

void readDFA(const string & fileName,
             adjacencyType & adjVec,
             int & numOfStates,
             vector<edge> & newEdges);