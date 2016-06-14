#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <queue>
#include <set>
#include <sstream>
#include <fstream>
#include "readSmt.h"
#include "readSmt.tab.h"
#include <map>

extern FILE *yyin;

using namespace std;

map<string, Node *> nodeMap;
set<string> sameMultibitReturnValueOperators;
set<string> diffMultibitReturnValueOperators;
set<string> singleBitReturnValueOperators;
set<string> operatorsAffectingRightSlices;

static bool logFlag = false;
static ofstream logFile;

/* [begin] - Node function definitions */

/* constructors */
Node::Node(string n, string o, string dt) {
    name = n;
    oper = o;
    datatype = dt;
}

Node::Node(string n, string dt) {
    name = n;
    datatype = dt;
}

Node::Node(string n, string dt, int w) {
    name = n;
    datatype = dt;
    width = w;
    bitSlices.insert(0);
    bitSlices.insert(w);
}

Node::Node(string n, string dt, int w, set<Node *> p, set<int> bs) {
    name = n;
    datatype = dt;
    width = w;
    preds = p;
    bitSlices = bs;
}

/* getters */
string Node::getName() {
    return name;
}

string Node::getOper() {
    return oper;
}

string Node::getDatatype() {
    return datatype;
}

int Node::getWidth() {
    return width;
}

vector<Node *>& Node::getSuccs() {
    return succs;
}

set<Node *>& Node::getPreds() {
    return preds;
}

set<int>& Node::getBitSlices() {
    return bitSlices;
}

int Node::getLb() {
    return lb;
}

int Node::getUb() {
    return ub;
}

/* setters */
void Node::setName(string n) {
    name = n;
}

void Node::setOper(string o) {
    oper = o;
}

void Node::setDatatype(string dt) {
    datatype = dt;
}

void Node::setWidth(int w) {
    width = w;
}

void Node::setLb(int l) {
    lb = l;
}

void Node::setUb(int u) {
    ub = u;
}

/* functions */
void Node::addSucc(Node *node) {
    succs.push_back(node);
}

void Node::addPred(Node *node) {
    preds.insert(node);
}

bool Node::addBitSlice(int index) {
    return bitSlices.insert(index).second;
}

int Node::getRightSliceWidth() {
    set<int>::iterator it = getBitSlices().begin();
    return *(++it) - *(getBitSlices().begin());
}

string Node::toString() {
    stringstream ss;
    ss << "[Name: " << name;
    ss << ", Operator: " << oper;
    ss << ", Datatype: " << datatype;
    ss << ", Width: " << width;

    ss << ", bitSlices: (";
    for (set<int>::iterator it = bitSlices.begin(); it != bitSlices.end(); ++it)
        ss << " " << *it;
    ss << ")";

    ss << ", Succs: " << succs.size() << "(";
    for (vector<Node *>::iterator it = succs.begin(); it != succs.end(); ++it)
        ss << " " << (*it)->getName();
    ss << ")";

    ss << ", Preds: " << preds.size() << "(";
    for (set<Node *>::iterator it = preds.begin(); it != preds.end(); ++it)
        ss << " " << (*it)->getName();
    ss << ")]";

    return ss.str();
}

/* [end] - Node function definitions */

/* [begin] - Auxillary functions */

void init() {
    sameMultibitReturnValueOperators.insert("bvadd");
    sameMultibitReturnValueOperators.insert("bvsub");
    sameMultibitReturnValueOperators.insert("bvor");
    sameMultibitReturnValueOperators.insert("bvand");
    sameMultibitReturnValueOperators.insert("bvxor");
    sameMultibitReturnValueOperators.insert("bvnot");
    sameMultibitReturnValueOperators.insert("ite");

    diffMultibitReturnValueOperators.insert("extract");
    diffMultibitReturnValueOperators.insert("concat");

    singleBitReturnValueOperators.insert("and");
    singleBitReturnValueOperators.insert("or");
    singleBitReturnValueOperators.insert("xor");
    singleBitReturnValueOperators.insert("=");
    singleBitReturnValueOperators.insert("not");
    singleBitReturnValueOperators.insert("bvuge");
    singleBitReturnValueOperators.insert("bvule");
}

void printMap(map<string, Node *>& nMap, ostream& out) {
    map<string, Node *>::iterator it;

    out << "-------------------[begin] Map--------------------" << endl;
    out << "Size of Map: " << nMap.size() << endl;
    for (it = nMap.begin(); it != nMap.end(); ++it) {
        out << it->first << ": " << (it->second)->toString() << endl;
    }
    out << "--------------------[end] Map---------------------" << endl;
}

void addSuccsAndPreds(Node *parent, vector<Node *> *succs) {
    vector<Node *>::iterator it;
    for (it = succs->begin(); it != succs->end(); ++it) {
        parent->addSucc(*it);
        (*it)->addPred(parent);
    }
}

void getExtractNodes(Node *root, queue<Node *>& bitSliceQueue) {
    queue<Node *> bfsQueue;
    set<Node *> visited;

    bfsQueue.push(root);
    visited.insert(root);

    while (!bfsQueue.empty()) {
        Node *node = bfsQueue.front();
        bfsQueue.pop();
        if (node->getOper() == "extract")
            bitSliceQueue.push(node);

        for (vector<Node *>::iterator it = node->getSuccs().begin(); it != node->getSuccs().end(); ++it) {
            if (visited.find(*it) == visited.end()) {
                visited.insert(*it);
                bfsQueue.push(*it);
            }
        }
    }
}

void bitSliceAndPropagate(map<string, Node *>& nMap) {

    Node *root = nMap["__root__"];

    queue<Node *> bitSliceQueue;
    getExtractNodes(root, bitSliceQueue); // pushes the 'extract' nodes in bitSliceQueue

    while (!bitSliceQueue.empty()) {
        Node *node = bitSliceQueue.front();
        bitSliceQueue.pop();
        propagateBitSlice(node, bitSliceQueue);
    }
}

void propagateBitSlice(Node *node, queue<Node *>& bitSliceQueue) {

    if (node->getOper() == "extract" || node->getOper() == "concat"
            || (sameMultibitReturnValueOperators.find(node->getOper()) != sameMultibitReturnValueOperators.end())
            || (singleBitReturnValueOperators.find(node->getOper()) != singleBitReturnValueOperators.end())
            ) {
        propagateBitSliceToSuccs(node, bitSliceQueue);
        propagateBitSliceToSiblings(node, bitSliceQueue);
    } else if (node->getWidth() > 1 && (node->getDatatype() == "constant" || (node->getOper() == "" && node->getDatatype() == "bitvector"))) {
        propagateBitSliceToSiblings(node, bitSliceQueue);
    } else {
        if (logFlag)
            logFile << "[" << __FILE__ << ":" << __LINE__ << "] Unhandled Operator: " << node->getOper() << " for node: " << node->getName() << endl;
    }
}

void propagateBitSliceToSuccs(Node *node, queue<Node *>& bitSliceQueue) {
    for (vector<Node *>::iterator it = node->getSuccs().begin(); it != node->getSuccs().end(); ++it) {
        if (node->getOper() == "concat") {

            int prevWidth = 0;

            for (vector<Node *>::reverse_iterator it = node->getSuccs().rbegin(); it != node->getSuccs().rend(); ++it) {

                Node *succ = *it;
                bool isNewElem = false;

                for (set<int>::iterator sliceIt = node->getBitSlices().begin(); sliceIt != node->getBitSlices().end(); ++sliceIt) {
                    int childSliceIndex = *sliceIt - prevWidth;
                    if (childSliceIndex > 0 && childSliceIndex <= succ->getWidth()) {
                        if (childSliceIndex == 1 && succ->getWidth() > 1) {
                            if (logFlag)
                                logFile << "[Non shrinking node] Node: " << succ->getName() << ", Reason: propagated from parent node: " << node->getName() << endl;
                        }
                        isNewElem = succ->addBitSlice(childSliceIndex) || isNewElem;
                    }
                }

                if (isNewElem)
                    bitSliceQueue.push(succ);

                prevWidth += succ->getWidth();
            }
        } else {
            if (updateBitSliceOfNode(node, *it))
                bitSliceQueue.push(*it);
        }
    }
}

void propagateBitSliceToSiblings(Node *node, queue<Node *>& bitSliceQueue) {
    for (set<Node *>::iterator it = node->getPreds().begin(); it != node->getPreds().end(); ++it) {
        if (singleBitReturnValueOperators.find((*it)->getOper()) != singleBitReturnValueOperators.end()) {
            for (vector<Node *>::iterator succ = (*it)->getSuccs().begin(); succ != (*it)->getSuccs().end(); ++succ) {
                if (updateBitSliceOfNode(node, *succ))
                    bitSliceQueue.push(*succ);
            }
        }
    }
}

bool updateBitSliceOfNode(Node *src, Node *dest) {
    if (src->getDatatype() == "bool" || src->getWidth() == 1 || dest->getDatatype() == "bool" || dest->getWidth() == 1) {
        return false;
    } else if (src->getOper() == "extract") {

        bool isNewElem = false;
        for (set<int>::iterator it = src->getBitSlices().begin(); it != src->getBitSlices().end(); ++it) {
            int childSliceIndex = *it + src->getLb();
            assert(childSliceIndex <= dest->getWidth());

            if (*it == 1 && dest->getWidth() > 1) {
                if (logFlag)
                    logFile << "[Non shrinking node] Node: " << dest->getName() << ", Reason: propagated from node: " << src->getName() << endl;
            }

            isNewElem = dest->addBitSlice(childSliceIndex) || isNewElem;
        }

        return isNewElem;
    } else if (src->getBitSlices() == dest->getBitSlices()) {
        return false;
    } else {
        for (set<int>::iterator it = src->getBitSlices().begin(); it != src->getBitSlices().end(); ++it) {
            assert(*it <= dest->getWidth());

            if (*it == 1 && dest->getWidth() > 1) {
                if (logFlag)
                    logFile << "[Non shrinking node] Node: " << dest->getName() << ", Reason: propagated from node: " << src->getName() << endl;
            }
            dest->addBitSlice(*it);
        }

        return true;
    }
}

bool shrinkSmtGraph(map<string, Node *>& nMap, int shrinkToBitWidth) {
    vector<Node *> leafNodes;
    getLeafNodes(nMap, leafNodes); // puts the leaf nodes in 'leafNodes'
    Node *leafWithSmallestRightSlice = getLeafWithSmallestRightMostSlice(leafNodes, shrinkToBitWidth);

    if (leafWithSmallestRightSlice == NULL) {// nothing to shrink
        if (logFlag) {
            logFile << "[Shrink Error] Unable to shrink - no node has right slice width > width to shrink." << endl;
        }
        return false;
    }

    int shrinkWidth = leafWithSmallestRightSlice->getRightSliceWidth() - shrinkToBitWidth;

    // shrink right slices to 'shrinkWidth' amount for all leaf nodes
    if (!shrinkLeaves(leafNodes, shrinkToBitWidth, shrinkWidth))
        return false;

    map<string, int> alreadyResizedNodes;
    map<string, string> replacedNodesMap;
    return shrinkNode(nMap["__root__"], alreadyResizedNodes, replacedNodesMap, shrinkWidth);
}

bool shrinkLeaves(vector<Node *>& leafNodes, int shrinkToBitWidth, int shrinkWidth) {
    for (vector<Node *>::iterator it = leafNodes.begin(); it != leafNodes.end(); ++it) {
        if ((*it)->getWidth() > shrinkToBitWidth) {
            if ((*it)->getDatatype() == "constant") {
                int length = (*it)->getName().length();
                string rightBits = (*it)->getName().substr(length - (*it)->getRightSliceWidth());

                bool hasZeros = rightBits.find('0') != string::npos;
                bool hasOnes = rightBits.find('1') != string::npos;

                if (hasZeros && hasOnes) {
                    if (logFlag)
                        logFile << "[Shrink Error] Unable to shrink constant (" << (*it)->getName() << ")as it contains 0's and 1's in its right slice." << endl;
                    return false;
                } else {
                    (*it)->setWidth((*it)->getWidth() - shrinkWidth);
                    (*it)->setName((*it)->getName().substr(0, length - shrinkWidth));
                }
            } else {
                (*it)->setWidth((*it)->getWidth() - shrinkWidth);
            }
        }
    }

    return true;
}

bool shrinkNode(Node *node, map<string, int>& alreadyResizedNodes, map<string, string>& rMap, int shrinkWidth) {

    if (alreadyResizedNodes.find(node->getName()) != alreadyResizedNodes.end())
        return true;

    bool isShrinkable = true;

    if (node->getSuccs().size() == 0) { // leaves have already been shrunk
        return true;
    } else if (sameMultibitReturnValueOperators.find(node->getOper()) != sameMultibitReturnValueOperators.end()) {

        vector<Node *>::iterator it = node->getSuccs().begin();
        shrinkNode(*it, alreadyResizedNodes, rMap, shrinkWidth);

        vector<Node *>::iterator secondSucc;
        if (node->getOper() == "ite")
            ++it;

        secondSucc = it;

        shrinkNode(*it, alreadyResizedNodes, rMap, shrinkWidth);

        int succWidth = (*it)->getWidth();
        for (++it; it != node->getSuccs().end(); ++it) {
            shrinkNode(*it, alreadyResizedNodes, rMap, shrinkWidth);

            if ((*it)->getWidth() != succWidth) {
                if (logFile) {
                    logFile << "[Shrink Error] Node: " << node->getName() << ", Reason: successor (" <<
                            (*secondSucc)->getName() << "(" << (*secondSucc)->getWidth() << "), " <<
                            (*it)->getName() << "(" << (*it)->getWidth() << ")) widths differ." << endl;
                }
                isShrinkable = false;
                break;
            }
        }

        alreadyResizedNodes[node->getName()] = node->getWidth() - succWidth;
        node->setWidth(succWidth);
    } else if (node->getOper() == "=" || node->getOper() == "bvuge" || node->getOper() == "bvule") {
        vector<Node *>::iterator it = node->getSuccs().begin();

        shrinkNode(*it, alreadyResizedNodes, rMap, shrinkWidth);

        int succWidth = (*it)->getWidth();
        for (++it; it != node->getSuccs().end(); ++it) {
            shrinkNode(*it, alreadyResizedNodes, rMap, shrinkWidth);

            if ((*it)->getWidth() != succWidth) {
                if (logFlag) {
                    logFile << "[Shrink Error] Node: " << node->getName() << ", Reason: successor (" <<
                            (*(node->getSuccs().begin()))->getName() << "(" << (*(node->getSuccs().begin()))->getWidth() << "), " <<
                            (*it)->getName() << "(" << (*it)->getWidth() << ")) widths differ." << endl;
                }
                isShrinkable = false;
                break;
            }
        }
        alreadyResizedNodes[node->getName()] = 0;
    } else if (node->getOper() == "concat") {

        /***** [begin] Special case for abs_diff. We need to care of this later */
        Node *rightSucc = *(node->getSuccs().rbegin());
        bool sameOneBitChild = true;

        for (vector<Node *>::iterator it = node->getSuccs().begin(); it != node->getSuccs().end(); ++it) {
            if (*it != rightSucc || (*it)->getWidth() > 1) {
                sameOneBitChild = false;
            }
        }

        if (sameOneBitChild) {
            node->setWidth(node->getWidth() - shrinkWidth);
            node->getSuccs().resize(node->getWidth());
            alreadyResizedNodes[node->getName()] = shrinkWidth;
        }/***** [end] Special case for abs_diff. We need to care of this later */
        else {
            int sumOfSuccWidth = 0;
            for (vector<Node *>::iterator it = node->getSuccs().begin(); it != node->getSuccs().end(); ++it) {
                shrinkNode(*it, alreadyResizedNodes, rMap, shrinkWidth);
                sumOfSuccWidth += (*it)->getWidth();
            }

            alreadyResizedNodes[node->getName()] = node->getWidth() - sumOfSuccWidth;
            node->setWidth(sumOfSuccWidth);
        }
    } else if (node->getOper() == "extract") {
        Node *succ = *(node->getSuccs().begin());
        shrinkNode(succ, alreadyResizedNodes, rMap, shrinkWidth);

        if (node->getLb() == 0) {
            alreadyResizedNodes[node->getName()] = alreadyResizedNodes[succ->getName()];
            node->setWidth(node->getWidth() - alreadyResizedNodes[succ->getName()]);
        } else {
            alreadyResizedNodes[node->getName()] = 0;
        }
        node->setUb(node->getUb() - alreadyResizedNodes[succ->getName()]);

        if (node->getLb() > 0)
            node->setLb(node->getLb() - alreadyResizedNodes[succ->getName()]);
    } else if (node->getName() == "__root__"
            || (singleBitReturnValueOperators.find(node->getOper()) != singleBitReturnValueOperators.end())) {
        for (vector<Node *>::iterator it = node->getSuccs().begin(); it != node->getSuccs().end(); ++it)
            isShrinkable = isShrinkable && shrinkNode(*it, alreadyResizedNodes, rMap, shrinkWidth);
    } else if (node->getOper() == "bvmul") {
        replaceNodeWithVariable(node, rMap);
    } else {
        if (logFlag) {
            logFile << "[Shrink Error] Node: " << node->getName() << ", Reason: Unhandled operator: " << node->getOper() << endl;
        }
    }

    return isShrinkable;
}

void replaceNodeWithVariable(Node *node, map<string, string>& rMap) {
    static int counter = 0;
    string tempNodeName = "r__temp__" + toString(counter++);
    Node *newNode = new Node(tempNodeName, "bitvector", node->getWidth(), node->getPreds(), node->getBitSlices());

    nodeMap[tempNodeName] = newNode;
}

void getLeafNodes(map<string, Node *>& nMap, vector<Node *>& leafNodes) {
    for (map<string, Node *>::iterator it = nMap.begin(); it != nMap.end(); ++it) {
        if ((it->second)->getSuccs().size() == 0)
            leafNodes.push_back(it->second);
    }
}

Node* getLeafWithSmallestRightMostSlice(vector<Node *>& nMap, int shrinkToBitWidth) {

    Node *leafWithSmallestRightSlice = NULL;

    for (vector<Node *>::iterator it = nMap.begin(); it != nMap.end(); ++it) {
        Node *node = *it;
        if (node->getWidth() > shrinkToBitWidth) { // check getWidth()
            if (leafWithSmallestRightSlice == NULL || leafWithSmallestRightSlice->getRightSliceWidth() > node->getRightSliceWidth())
                leafWithSmallestRightSlice = node;
        }
    }

    return leafWithSmallestRightSlice;
}

void generateSmtFile(map<string, Node *>& nMap, ofstream& smtFile) {
    smtFile << "; Shrunk SMT constraints for module abs_diff" << endl;
    smtFile << "; Generated by 'Bit Shrinking Pass' of 2-pass STEword " << endl;
    smtFile << ";" << endl;
    smtFile << "(set-logic QF_ABV)" << endl;
    smtFile << "(set-info :smt-lib-version 2.0)" << endl;

    printDeclareStatements(nMap, smtFile);
    smtFile << "(assert (and" << endl;
    smtFile << getNodeAsString(nMap["__root__"]) << endl;
    printSmtGraph(nMap, smtFile);

    smtFile << "))" << endl << endl;
    smtFile << "(check-sat)" << endl;
    smtFile << "(exit)" << endl;
}

void printDeclareStatements(map<string, Node *>& nMap, ofstream& smtFile) {
    for (map<string, Node *>::iterator it = nMap.begin(); it != nMap.end(); ++it) {
        Node *node = it->second;
        if (node->getDatatype() == "bitvector") {
            smtFile << "(declare-fun " << it->first << " () ";
            smtFile << "(_ BitVec " << node->getWidth() << "))" << endl;
        } else if (node->getDatatype() == "bool") {
            smtFile << "(declare-fun " << it->first << " () ";
            smtFile << "Bool)" << endl;
        } else if (node->getDatatype() == "constant" || node->getName() == "__root__") {
            /* do nothing */
        } else {
            cerr << "[" << __FILE__ << ":" << __LINE__ << "] Unhandled node: " << node->getName() << endl;
        }
    }
}

string getNodeAsString(Node *node) {
    string str = "";
    if (node->getOper() == "concat") {
        vector<Node *>::iterator it;
        for (it = node->getSuccs().begin(); it != node->getSuccs().end() - 1; ++it) {
            str += " (concat " + (*it)->getName();
        }

        str += " " + (*it)->getName(); // last node of concat list

        for (unsigned int i = 0; i < node->getSuccs().size() - 1; ++i)
            str += ")";
    } else if (node->getOper() == "extract") {
        str += "((_ extract " + toString(node->getUb() - 1) + " " + toString(node->getLb()) + ") " + node->getSuccs()[0]->getName() + ")";
    } else {
        str += "(" + node->getOper();
        for (vector<Node *>::iterator it = node->getSuccs().begin(); it != node->getSuccs().end(); ++it) {
            str += " " + (*it)->getName();
        }

        str += ")";
    }

    return str;
}

void printSmtGraph(map<string, Node *>& nMap, ofstream& smtFile) {
    queue<Node *> bfsQueue;
    set<Node *> visited;

    Node *root = nMap["__root__"];
    bfsQueue.push(root);
    visited.insert(root);

    while (!bfsQueue.empty()) {
        Node *node = bfsQueue.front();
        bfsQueue.pop();

        if (node->getOper() != "" && (node->getDatatype() == "bitvector" || node->getDatatype() == "bool"))
            smtFile << "(= " << node->getName() << " " << getNodeAsString(node) << ")" << endl;

        for (vector<Node *>::iterator it = node->getSuccs().begin(); it != node->getSuccs().end(); ++it) {
            if (visited.find(*it) == visited.end()) {
                visited.insert(*it);
                bfsQueue.push(*it);
            }
        }
    }
}

bool checkOperators(map<string, Node *>& nMap) {
    bool handledOperators = true;

    for (map<string, Node *>::iterator it = nMap.begin(); it != nMap.end(); ++it) {
        Node *node = it->second;

        if (sameMultibitReturnValueOperators.find(node->getOper()) != sameMultibitReturnValueOperators.end())
            continue;
        else if (diffMultibitReturnValueOperators.find(node->getOper()) != diffMultibitReturnValueOperators.end())
            continue;
        else if (singleBitReturnValueOperators.find(node->getOper()) != singleBitReturnValueOperators.end())
            continue;
        else if (node->getOper() == "")
            continue;
        else {
            if (logFlag) {
                logFile << "Unhandled Operator: " << node->getOper() << endl;
            }
            handledOperators = false;
            break;
        }
    }

    return handledOperators;
}

void updateSlicesForConcat(map<string, Node *>& nMap) {
    for (map<string, Node *>::iterator it = nMap.begin(); it != nMap.end(); ++it) {
        Node *node = it->second;

        if (node->getOper() == "concat") {
            Node *rightSucc = *(node->getSuccs().rbegin());

            /***** Special case for abs_diff. We need to care of this later */
            bool sameOneBitChild = true;

            for (vector<Node *>::iterator it = node->getSuccs().begin(); it != node->getSuccs().end(); ++it) {
                if (*it != rightSucc || (*it)->getWidth() > 1) {
                    sameOneBitChild = false;
                }
            }

            if (sameOneBitChild)
                continue;
            /***** Special case for abs_diff. We need to care of this later */

            node->addBitSlice(rightSucc->getWidth());
        }
    }
}

int getShrinkToBitWidth(map<string, Node *>& nMap, string shrinkAmount) {
    if (shrinkAmount == "log#leaves_including_const") {
        int numLeaves = countLeaves(nodeMap);
        return ceil(log(numLeaves) / log(2));
    } else if (shrinkAmount == "log#nodes") {
        return ceil(log(nMap.size()) / log(2));
    } else if (isInteger(shrinkAmount)) {
        return atoi(shrinkAmount.c_str());
    } else {
        return -1;
    }
}

int countLeaves(map<string, Node *>& nMap) {

    int count = 0;

    for (map<string, Node *>::iterator it = nMap.begin(); it != nMap.end(); ++it) {
        Node *node = it->second;
        if (node->getSuccs().size() == 0)
            count++;
    }

    return count;
}

string toString(int num) {
    stringstream ss;
    ss << num;

    return ss.str();
}

void copyFile(string source, string dest) {
    ifstream src(source.c_str(), std::ios::binary);
    ofstream dst(dest.c_str(), std::ios::binary);

    dst << src.rdbuf();
}

bool isInteger(string str) {
    for (unsigned int i = 0; i < str.length(); ++i)
        if (!isdigit(str.at(i)))
            return false;
    return true;
}

/* [end] - Auxillary functions */

int main(int argc, char **argv) {

    ofstream smtFile;
    // ofstream origSmtFile;
    string shrinkAmount = "log#leaves_including_const";

    if (argc >= 3) {
        yyin = fopen(argv[1], "r");
        smtFile.open(argv[2], std::ofstream::out);

        // origSmtFile.open("origSmt.smt", std::ofstream::out);

        if (smtFile.fail()) {
            cerr << "[Error] unable to open file: " << argv[2] << endl;
            exit(1);
        }
    } else {
        cerr << "[Usage] readSmt <input_smt_file> <output_smt_file> [-shrinkto=<shrink_value>] [-log=<log_file_name>]" << endl;
        exit(1);
    }

    if (argc >= 4) {
        for (int i = 3; i < argc; ++i) {
            string option = argv[i];
            if (option.find("-log=") == 0) {
                logFile.open((option.substr(5)).c_str(), std::ofstream::out);
                if (logFile.fail()) {
                    cerr << "[Error] unable to open log file: " << argv[3] << endl;
                    exit(1);
                }
                logFlag = true;
            } else if (option.find("-shrinkto=") == 0) {
                shrinkAmount = option.substr(10);
            }
        }
    }

    init();
    yyparse();

    if (logFlag) {
        logFile << "MAP VALUES BEFORE SHRINKING" << endl;
        printMap(nodeMap, logFile);
    }
    // generateSmtFile(nodeMap, origSmtFile);

    bool isShrinkable = checkOperators(nodeMap);

    if (isShrinkable) {
        int shrinkToBitWidth = getShrinkToBitWidth(nodeMap, shrinkAmount);

        if (shrinkToBitWidth == -1) {
            cerr << "[Error] Invalid 'shrinkto' value." << endl;
            exit(1);
        }

        bitSliceAndPropagate(nodeMap);

        if (logFlag) {
            logFile << "MAP VALUES AFTER SLICING AND PROPAGATION" << endl;
            printMap(nodeMap, logFile);
        }

        isShrinkable = shrinkSmtGraph(nodeMap, shrinkToBitWidth);

        if (logFlag) {
            logFile << "MAP VALUES AFTER SHRINKING" << endl;
            printMap(nodeMap, logFile);
        }

        if (isShrinkable) {
            generateSmtFile(nodeMap, smtFile);
        }
    }

    if (!isShrinkable) {
        cout << "notshrunk" << endl;
        copyFile(argv[1], argv[2]);
    } else {
        cout << "shrunk" << endl;
    }

    return 0;
}
