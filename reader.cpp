//
// Created by ivan on 29.04.18.
//

#include "reader.h"

#include <iostream>
#include <fstream>
#include <regex>

namespace {

    ifstream openFile(const string & fileName) {
        ifstream fileStream;
        fileStream.open(fileName, fstream::in);
        if (fileStream.fail()) {
            cout << "Can't open file " << fileName << endl;
            exit(1);
        }
        return fileStream;
    }

    string skipUntilRegex(ifstream & fileStream, const regex & regExp) {
        string curString;
        do {
            getline(fileStream, curString);
        } while ( ! regex_search(curString, regExp));
        return curString;
    }

    vector<string> split(string & s, char delim) {
        vector<string> result;
        stringstream ss(s);
        string item;

        while (getline(ss, item, delim)) {
            if ( ! item.empty()) {
                result.push_back(item);
            }
        }
        return result;
    }

    void readEdges(ifstream & fileStream,
                   adjacencyType & adjVec,
                   unsigned int numOfStates,
                   string & curString, // curString contains rule
                   vector<edge> & newEdges)
    {
        adjVec = adjacencyType(numOfStates, map<string, set<int>>());

        //regex for X -> Y[label="Z"]
        regex automationRule ("([0-9]+)"
                              "( -> )"
                              "([0-9]+)"
                              "([^0-9]+\")"
                              "([^\"]*)"
                              "(\"])+");
        regex closingBracket ("[}]+");
        smatch res;
        do {
            if ( ! curString.empty()) {
                if ( ! regex_search(curString, closingBracket)) {
                    if (regex_search(curString, res, automationRule)) {
                        auto from = stoi(res.str(1));
                        auto to = stoi(res.str(3));
                        auto by = res.str(5);
                        newEdges.emplace_back(move(edge(from,to,by)));
                        auto iter = adjVec[from].find(by);
                        if (iter == adjVec[from].end()) {
                            adjVec[from].insert(make_pair(by, set<int>({to})));
                        } else {
                            iter->second.insert(to);
                        }
                    } else {
                        cout << "Can't parse " << curString
                             << " with regex ([0-9]+)( -> )([0-9]+)([^0-9]+\")[^\"]*)\"])+";
                        exit(1);
                    }
                } else {
                    break;
                }
            }
        } while (getline(fileStream, curString));
    }
}

void readRFA(const string & fileName,
             adjacencyType & adjVec,
             startStatesType & startStates,
             finalStatesType & finalStates,
             int & numOfStates)
{
    ifstream fileStream = openFile(fileName);

    // get number of states
    string curString = skipUntilRegex(fileStream, regex("[0-9]+"));
    numOfStates = static_cast<int>(split(curString, ';').size());

    // parse info about start and final states
    curString = skipUntilRegex(fileStream, regex("([0-9]+)([[])"));

    regex ruleRegex ("(->)");
    regex isStart ("(color=)");
    regex isFinal ("(shape=)");
    regex hasLabel ("(label=\")([^\"]+)");
    regex numberInStateDefRegex ("([0-9]+)");
    smatch res;
    string label;
    int numOfStateInDef;

    while ( ! regex_search(curString, res, ruleRegex)) {
        if ( ! curString.empty()) {
            if (regex_search(curString, res, numberInStateDefRegex)) {
                numOfStateInDef = stoi(res.str(1));
            } else {
                cout << "Incorrect format of state definition, expected X[label=\"Y\", ...], got "
                     << curString << endl;
                exit(1);
            }

            if (regex_search(curString, res, hasLabel)) {
                label = res.str(2);
                // this map state -> nonterminal
                if (regex_search(curString, res, isStart)) {
                    auto states = vector<string>(1, label);
                    auto emplaceResult = startStates.emplace(numOfStateInDef, states);
                    if ( ! emplaceResult.second) {
                        auto iter = emplaceResult.first;
                        (*iter).second.push_back(label);
                    }
                }

                // this map nonterminal -> state
                if (regex_search(curString, res, isFinal)) {
                    auto states = vector<int>(1, numOfStateInDef);
                    auto emplaceResult = finalStates.emplace(label, states);
                    if ( ! emplaceResult.second) {
                        auto iter = emplaceResult.first;
                        (*iter).second.push_back(numOfStateInDef);
                    }
                }
            } else {
                cout << "Expected label= in string " << curString << endl;
                exit(1);
            }
        }
        getline(fileStream, curString);
    }
    vector<edge> dummy = vector<edge>(0);
    readEdges(fileStream, adjVec, static_cast<unsigned int>(numOfStates), curString, dummy);
    fileStream.close();
}


void readDFA(const string & fileName,
             adjacencyType & adjVec,
             int & numOfStates,
             vector<edge> & newEdges)
{
    ifstream fileStream = openFile(fileName);

    // get number of states
    string curString = skipUntilRegex(fileStream, regex("[0-9]+"));
    numOfStates = static_cast<int>(split(curString, ';').size());
    getline(fileStream, curString);

    readEdges(fileStream, adjVec, static_cast<unsigned int>(numOfStates), curString, newEdges);
}
