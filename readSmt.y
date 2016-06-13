%{
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <assert.h>
  #include <vector>
  #include <algorithm>
  #include "readSmt.h"
  using namespace std;

  extern int yylex();
  extern void yyerror(char *);
  extern FILE *yyin;
%}

%union {
  char *str;
  // vector<Node *> *nodeVector;
  void *nodeVector; // creating a void pointer which is actually used as 'vector<Node *> *'type
  Node *node;
}

%token <str> DECLARE_FUN BITVEC BOOL INT STRING ASSERT CONSTANT CONCAT EXTRACT
%type <str> node_name
%type <nodeVector> node_names concat_list

%%

lines:
  lines line
  |
  ;

line:
  '(' DECLARE_FUN STRING '(' ')' '(' '_' BITVEC INT ')' ')'
    {
      // printf("string: %s, int: %s\n", $3, $9);
      Node *node = new Node($3, "bitvector", atoi($9));
      node->addBitSlice(atoi($9));
      node->addBitSlice(0);

      nodeMap[$3] = node;
    }

  | '(' DECLARE_FUN STRING '(' ')' BOOL ')'
    {
      // printf("string: %s, bool\n", $3);
      Node *node = new Node($3, "bool", 1);

      nodeMap[$3] = node;
    }

  | '(' ASSERT '(' STRING
    {
      // printf("assert, string: %s\n", $4);
    }

  | '(' STRING node_names ')'
    {
      // printf("string: %s, node_names\n", $2);
      assert(nodeMap.find("__root__") == nodeMap.end());

      // printf("Adding root node\n");
      Node *node = new Node("__root__", $2, "void");
      nodeMap["__root__"] = node;

      addSuccsAndPreds(nodeMap["__root__"], (vector<Node *> *)$3);
    }

  | '(' '=' STRING '(' STRING node_names ')' ')'
    {
      // printf("string: %s, string: %s, node_names\n", $3, $5);
      assert(nodeMap.find($3) != nodeMap.end());
      nodeMap[$3]->setOper($5);

      addSuccsAndPreds(nodeMap[$3], (vector<Node *> *)$6);
    }

  | '(' '=' STRING '(' '=' node_names ')' ')'
    {
      // printf("string: %s, =, node_names\n", $3);
      assert(nodeMap.find($3) != nodeMap.end());
      nodeMap[$3]->setOper("=");

      addSuccsAndPreds(nodeMap[$3], (vector<Node *> *)$6);
    }

  | '(' '=' STRING '(' '(' '_' EXTRACT INT INT ')' node_names ')' ')'
    {
      // printf("string: %s, =, node_names\n", $3);
      assert(nodeMap.find($3) != nodeMap.end());
      nodeMap[$3]->setOper("extract");
      nodeMap[$3]->setUb(atoi($8) + 1);
      nodeMap[$3]->setLb(atoi($9));

      addSuccsAndPreds(nodeMap[$3], (vector<Node *> *)$11);
    }

  | '(' '=' STRING concat_list ')'
    {
      // printf("string: %s, concat_list\n", $3);
      assert(nodeMap.find($3) != nodeMap.end());
      nodeMap[$3]->setOper("concat");

      // reverse the vector to correct order of concat list
      vector<Node *> *nodeVector = (vector<Node *> *)$4;
      reverse(nodeVector->begin(), nodeVector->end());

      // nodeMap[$3]->addBitSlice((*(nodeVector->rbegin()))->getWidth()); // add rightbit slice of leftmost child to parent 'concat'

      addSuccsAndPreds(nodeMap[$3], nodeVector);
    }

  | ')' ')'
    {
    }
  ;


node_names:
  node_names node_name
    {
      // printf("string: %s\n", $2);
      assert(nodeMap.find($2) != nodeMap.end());
      $$ = $1;
      ((vector<Node *> *)$$)->push_back(nodeMap[$2]);
    }

  |
    {
      $$ = new vector<Node *>();
    }
  ;

node_name:
  STRING
    {
      // printf("string: %s\n", $1);
      $$ = $1;
    }

  | CONSTANT
    {
      // printf("constant: %s\n", $1);
      $$ = $1;
      int width = strlen($1) - 2;

      if (nodeMap.find($1) == nodeMap.end()) {
        Node *node = new Node($1, "constant", width);
        nodeMap[$1] = node;
      }
    }

concat_list:
  '(' CONCAT node_name concat_list ')'
    {
      $$ = $4;
      ((vector<Node *> *)$$)->push_back(nodeMap[$3]);
    }
  | node_name
    {
      vector<Node *> *nodeVec = new vector<Node *>();
      nodeVec->push_back(nodeMap[$1]);
      $$ = nodeVec;
    }
  ;

%%
