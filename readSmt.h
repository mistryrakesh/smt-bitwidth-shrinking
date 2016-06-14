#ifndef __readSmt__h_
#define __readSmt__h_

#include <map>
#include <vector>
#include <set>
#include <queue>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

using namespace std;

extern int yyparse();

class Node {
private:
    string name;
    string oper;
    string datatype; // allowed datatypes: bitvector, bool, constant, void
    int width;
    set<Node *> preds;
    vector<Node *> succs;
    set<int> bitSlices;
    int lb, ub; // currently these fields are only relevant for 'extract' type operators

public:
    Node(string n, string o, string dt);
    Node(string n, string dt);
    Node(string n, string dt, int w);
    Node(string n, string dt, int w, set<Node *> preds, set<int> bitslices);

    /* getters */
    string getName();
    string getOper();
    string getDatatype();
    int getWidth();
    vector<Node *>& getSuccs();
    set<Node *>& getPreds();
    set<int>& getBitSlices();
    int getLb();
    int getUb();

    /* setters */
    void setName(string n);
    void setOper(string o);
    void setDatatype(string dt);
    void setWidth(int w);
    void setLb(int l);
    void setUb(int u);

    /* functions */
    void addSucc(Node *node);
    void addPred(Node *node);
    bool addBitSlice(int index);
    int getRightSliceWidth();
    void replaceSucc(Node *oldNode, Node *newNode);
    string toString();
};

extern map<string, Node *> nodeMap;

/*
 * The following sets contain operators based on value returned as compared to
 * that of the children.
 */
extern set<string> sameMultibitReturnValueOperators;
extern set<string> diffMultibitReturnValueOperators;
extern set<string> singleBitReturnValueOperators;


void printMap(map<string, Node *>& nMap, ostream& out);
void addSuccsAndPreds(Node *parent, vector<Node *> *succs);
void getExtractNodes(Node *root, queue<Node *>& bitSliceQueue);

void bitSliceAndPropagate(map<string, Node *>& nMap);
void propagateBitSlice(Node *node, queue<Node *>& bitSliceQueue, map<string, string>& rMap);
void propagateBitSliceToSuccs(Node *node, queue<Node *>& bitSliceQueue);
void propagateBitSliceToSiblings(Node *node, queue<Node *>& bitSliceQueue);
bool updateBitSliceOfNode(Node *src, Node *dest);

bool shrinkSmtGraph(map<string, Node *>& nMap, int shrinkToBitWidth);
bool shrinkLeaves(vector<Node *>& leafNodes, int shrinkToBitWidth, int shrinkWidth);
bool shrinkNode(Node *node, map<string, int>& alreadyResizedNodes, int shrinkWidth);

Node* replaceNodeWithVariable(Node *node, map<string, string>& rMap);

void getLeafNodes(map<string, Node *>& nMap, vector<Node *>& leafNodes);
Node* getLeafWithSmallestRightMostSlice(vector<Node *>& nMap, int shrinkToBitWidth);

void generateSmtFile(map<string, Node *>& nMap, ofstream& smtFile);
void printDeclareStatements(map<string, Node *>& nMap, ofstream& smtFile);
string getNodeAsString(Node *node);
void printSmtGraph(map<string, Node *>& nMap, ofstream& smtFile);

bool checkOperators(map<string, Node *>& nMap);
void updateSlicesForConcat(map<string, Node *>& nMap);
int getShrinkToBitWidth(map<string, Node *>& nMap, string shrinkAmount);
int countLeaves(map<string, Node *>& nMap);
bool isInteger(string str);


string toString(int num);
void copyFile(string source, string dest);

#endif // __readSmt__h_
