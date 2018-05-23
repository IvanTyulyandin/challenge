//
// Created by ivan on 29.04.18.
//

#include "reader.h"

#include <iostream>
#include <set>
#include <chrono>
#include <future>
#include <immintrin.h>


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

bool insertToFiniteAutomation(adjacencyType & finiteAutomation, int start, int final, string & by) {
    auto existIter = finiteAutomation[start].find(by);
    if (existIter != finiteAutomation[start].end()) {
        return existIter->second.insert(final).second;
    }
    finiteAutomation[start].insert(make_pair(by, set<int>{final}));
    return true;
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

void getCurTime(decltype(chrono::steady_clock::now()) start_time, string && msg) {
    auto cur = chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(cur - start_time);
    cout << msg << endl;
    std::cout << "Time: " << duration.count() << " msec\n";
}

int main(int argc, char* argv[]) {

    adjacencyType RFA;
    startStatesType startStatesRFA;
    finalStatesType finalStatesRFA;
    readRFA(argv[1], RFA, startStatesRFA, finalStatesRFA, numOfStatesRFA);

    adjacencyType DFA;
    vector<edge> newEdges = vector<edge>(0);
    readDFA(argv[2], DFA, numOfStatesDFA, newEdges);

	matrixSize = numOfStatesDFA * numOfStatesRFA;

	int matrixSizeTo32 = matrixSize % 32 == 0 ? matrixSize : matrixSize + (32 - matrixSize % 32); // for AVX2
	auto matrix = new char * [matrixSize];
    for (auto i = 0; i < matrixSize; ++ i) {
        matrix[i] = new char[matrixSizeTo32];
        char * matrixRow = matrix[i];
        for (auto j = 0; j < matrixSizeTo32; ++ j) {
            matrixRow[j] = 0;
        }
    }

	auto start_time = std::chrono::steady_clock::now();

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


    auto THREADS = std::thread::hardware_concurrency() - 1;
    int step = matrixSize / (THREADS + 1);

    __m256i ik = _mm256_set1_epi8(1); // PogChamp

    auto parallelClosure = [&matrix, &ik, matrixSizeTo32](int startK, int howMuch) {
        int endK = startK + howMuch;
        for (auto k = startK; k < endK; ++ k) {
            for (auto i = 0; i < matrixSize; ++ i) {
                char * matrixRow = matrix[i];
                if (matrixRow[k]) {
					for (auto j = 0; j < matrixSizeTo32; j += 32) {
						__m256i ij = _mm256_loadu_si256((__m256i const *) &matrixRow[j]);
						__m256i kj = _mm256_loadu_si256((__m256i const *) &matrix[k][j]);

						_mm256_storeu_si256((__m256i *) &matrixRow[j], _mm256_or_si256(ij, _mm256_and_si256(ik, kj)));
					}
//					for (auto j = 0; j < matrixSize; ++ j) {
//						if (matrix[k][j]) {
//							matrixRow[j] = true;
//						}
//					}
                }
            }
        }
    };

	indexArrayType indexArray;
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

        //getCurTime(start_time, "1 stage done");


        vector<future<void>> futArr;
        futArr.reserve(THREADS);
        for (auto i = 0; i < THREADS; ++ i) {
            futArr.emplace_back(std::async(launch::async, parallelClosure, i * step, step));
        }

        parallelClosure(THREADS * step, matrixSize - THREADS * step);

        for (auto i = 0; i < THREADS; ++ i) {
            futArr[i].get();
        }

        // stage 2: closure
        //getCurTime(start_time, "2 stage done");

		//cout << countResult(DFA) << " before\n";
        newEdges = vector<edge>(0);
        for (auto i = 0; i < matrixSize; ++ i) {
            char * matrixRow = matrix[i];
            int startGrm = indexArray[i].second;
            auto nonTermStartIter = startStatesRFA.find(startGrm);
            if (nonTermStartIter != startStatesRFA.end()) {
                for (auto j = 0; j < matrixSize; ++ j) {
                    if (matrixRow[j]) {
                        int finalGrm = indexArray[j].second;
                        for (auto & nonTerm : nonTermStartIter->second) {
                            auto nonTermFinalIter = finalStatesRFA.find(nonTerm);
                            if (nonTermFinalIter != finalStatesRFA.end()) {
                                for (auto finalStateForNonTerm : nonTermFinalIter->second) {
                                    if (finalStateForNonTerm == finalGrm) {
                                        int startDFA = indexArray[i].first;
                                        int finalDFA = indexArray[j].first;
                                        if (insertToFiniteAutomation(DFA, startDFA, finalDFA, nonTerm)) {
											newEdges.emplace_back(move(edge(startDFA, finalDFA, nonTerm)));
											needOneMoreStep = true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

        }
        //getCurTime(start_time, "3 stage done");
		//cout << countResult(DFA) << " after\n";
    } while (needOneMoreStep);

    getCurTime(start_time, "");
    cout << countResult(DFA) << "\n";

    for (auto i = 0; i < matrixSize; ++ i) {
    	delete[](matrix[i]);
    }
    delete[](matrix);

}