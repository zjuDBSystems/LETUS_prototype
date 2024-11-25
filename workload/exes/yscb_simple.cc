/**
 * A very simplified YCSB workload
 *  - only have one table, the table only have one field/column
*/

#include <fstream>
#include <iostream>
#include <string>

#include <json/json.h>

#include "DMMTrie.hpp"
#include "LSVPS.hpp"


using namespace std;

struct Task {
  std::vector<int> ops;
  std::vector<std::string> keys;
  std::vector<std::string> vals;
};

uint32_t zipf(uint32_t n) {
  double alpha = 1/(1 - theta);
  double eta = (1 - pow(2.0 / n, 1 - theta)) /
    (1 - zeta_2_theta / zetan);
  double u = (double)(rand() % 10000000) / 10000000;
  double uz = u*zetan;
  if(uz < 1) return 1;
  if(uz < 1 + pow(0.5,theta)) return 2;
  return 1 + (uint32_t)(n * pow(eta*u - eta + 1, alpha));
}

inline char RandomPrintChar() {
  return rand() % 94 + 33;
}


int main(int argc, char** argv) {

    // loading phase
    // test phase


}