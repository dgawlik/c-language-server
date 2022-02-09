#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <tree_sitter/api.h>
#include <memory>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <regex>

using std::shared_ptr;
using std::string;
using std::stringstream;
using std::vector;


#ifndef STACK_GRAPH_TREE_H
#define STACK_GRAPH_TREE_H

namespace stack_graph
{

    struct Point
    {
        uint32_t line;
        uint32_t column;
    };

    struct Range
    {
        uint32_t start;
        uint32_t end;
    };

    enum StackGraphNodeKind
    {
        NAMED_SCOPE,
        SYMBOL,
        REFERENCE,
        UNNAMED_SCOPE,
        IMPORT
    };

    struct StackGraphNode
    {
        string symbol;
        string _type;
        StackGraphNodeKind kind;
        shared_ptr<StackGraphNode> jump_to;
        vector<shared_ptr<StackGraphNode>> children;
        shared_ptr<StackGraphNode> parent;
        Point location;

        StackGraphNode(StackGraphNodeKind kind, string symbol, Point location)
        {
            this->symbol = symbol;
            this->kind = kind;
            this->_type = "";
            this->location = location;
            this->children = vector<shared_ptr<StackGraphNode>>();
            this->parent = nullptr;
        }

        string repr();
    };

    std::shared_ptr<StackGraphNode> build_stack_graph_tree(TSNode root, const char *source_code);
}

#endif
