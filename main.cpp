//
// Created by ivan on 29.04.18.
//

#include "reader.h"

#include <iostream>
#include <set>
#include <chrono>


using indexArrayType = vector<std::pair<int, int>>;
using namespace std;

int numOfStatesRFA;
int numOfStatesDFA;
int matrixSize;

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
}

void updateMatrix(const vector<edge> & DFAedges,
                  const adjacencyType & RFAedges,
                  char ** matrix)
{
    // matrix indices -> DFA state X RFA state
    for (auto & edge : DFAedges) {
        for (auto fromRFA = 0;  fromRFA < numOfStatesRFA; ++ fromRFA) {
            auto edgeRFA = RFAedges[fromRFA];
            auto iter = edgeRFA.find(edge.by);
            if (iter != edgeRFA.end()) {
                // have edge(s) by this nonterm in new edges of DFA and RFA
                // need to add info about edge(s) to matrix
                int rowIndex = edge.from * numOfStatesRFA + fromRFA;
                char * matrixRow = matrix[rowIndex];
                for (auto toRFA : iter->second) {
                    int colIndex = edge.to * numOfStatesRFA + toRFA;
                    matrixRow[colIndex] = 1;
                }
            }
        }
    }
}

void insertToFiniteAutomation(adjacencyType & finiteAutomation, int start, int final, string & by) {
    auto existIter = finiteAutomation[start].find(by);
    if (existIter != finiteAutomation[start].end()) {
        for (auto state : existIter->second) {
            if (state == final) {
                return; // edge exist already
            }
        }
        existIter->second.push_back(final);
    } else {
        finiteAutomation[start].insert(make_pair(by, vector<int>(1, final)));
    }
}

int countResult(const adjacencyType & DFA) {
    int res = 0;
    for (auto i = 0; i < numOfStatesDFA; ++ i) {
        for (auto edge : DFA[i]) {
            if (edge.first == "S") {
                res += edge.second.size();
            }
        }
    }
    return res;
}

int main() {

    adjacencyType RFA;
    startStatesType startStatesRFA;
    finalStatesType finalStatesRFA;
    readRFA("testRFA/Q1RFA.txt", RFA, startStatesRFA, finalStatesRFA, numOfStatesRFA);

    adjacencyType DFA;
    vector<edge> newEdges = vector<edge>(0);
    readDFA("data/wine.dot", DFA, numOfStatesDFA, newEdges);

    auto start_time = std::chrono::steady_clock::now();

    char ** matrix;
    matrixSize = numOfStatesDFA * numOfStatesRFA;
    matrix = new char * [matrixSize];
    for (auto i = 0; i < matrixSize; ++ i) {
        matrix[i] = new char[matrixSize];
        char * matrixRow = matrix[i];
        for (auto j = 0; j < matrixSize; ++ j) {
            matrixRow[j] = 0;
        }
    }

    // add edges to automation, if nonterminal start == end
    for (auto & iter : startStatesRFA) {
        int startGrm = iter.first;
        for (auto & by : iter.second) {
            auto finalsIter = finalStatesRFA.find(by);
            // if it is equal to end iterator => incorrect input
            for (auto finalGrm : finalsIter->second) {
                if (startGrm == finalGrm) {
                    for (auto i = 0; i < numOfStatesDFA; ++ i) {
                        insertToFiniteAutomation(DFA, i, i, by);
                    }
                }
            }
        }
    }

    bool needOneMoreStep = false;

    indexArrayType indexArray = indexArrayType(0);
    indexArray.reserve(static_cast<unsigned int>(numOfStatesDFA * numOfStatesRFA));

    for (int i = 0; i < numOfStatesDFA; ++ i) {
        for (int j = 0; j < numOfStatesRFA; ++ j) {
            indexArray.emplace_back(make_pair(i, j));
        }
    }

    do {
        needOneMoreStep = false;

        // stage 1: add info about new edges to matrix
        updateMatrix(newEdges, RFA, matrix);

        // stage 2: closure
        for (auto k = 0; k < matrixSize; ++ k) {
            for (auto i = 0; i < matrixSize; ++ i) {
                char * matrixRow = matrix[i];
                if (matrixRow[k]) {
                    char * matrixK = matrix[k];
                    for (auto j = 0; j < matrixSize; ++ j) {
                        if (matrixK[j]) {
                            matrixRow[j] = true;
                        }
                    }
                }
            }
        }

        // stage 3: add new edges for nonterminals to DFA after closure

        // can be improved by reordering of search
        // find starting states for edges -> get step in matrix -> so on

        // a lot of rechecking (mb it is good after closure)

        newEdges = vector<edge>(0);
        for (auto i = 0; i < matrixSize; ++ i) {
            char * matrixRow = matrix[i];
            int startDFA = indexArray[i].first;
            int startGrm = indexArray[i].second;
            auto nonTermStartIter = startStatesRFA.find(startGrm);
            if (nonTermStartIter != startStatesRFA.end()) {
                for (auto j = 0; j < matrixSize; ++ j) {
                    if (matrixRow[j]) {
                        int finalDFA = indexArray[j].first;
                        int finalGrm = indexArray[j].second;
                        for (auto & nonTerm : nonTermStartIter->second) {
                            auto nonTermFinalIter = finalStatesRFA.find(nonTerm);
                            if (nonTermFinalIter != finalStatesRFA.end()) {
                                for (auto finalStateForNonTerm : nonTermFinalIter->second) {
                                    if (finalStateForNonTerm == finalGrm) {
                                        newEdges.push_back(move(edge(startDFA, finalDFA, nonTerm)));
                                        insertToFiniteAutomation(DFA, startDFA, finalDFA, nonTerm);
                                    }
                                }
                            }
                        }
                    }
                }
            }

        }

    } while (needOneMoreStep);

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "Time: " << duration.count() << " msec\n";
    cout << "Result: " << countResult(DFA) << endl;
}