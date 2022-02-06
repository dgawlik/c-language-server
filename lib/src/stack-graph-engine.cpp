

#include <stack-graph-engine.h>
#include <fstream>
#include <tuple>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>

using stack_graph::build_stack_graph_tree;
using stack_graph::Coordinate;
using stack_graph::CustomHash;
using stack_graph::StackGraphEngine;
using stack_graph::StackGraphNode;
using stack_graph::StackGraphNodeKind;

extern "C" TSLanguage *tree_sitter_c();

void _index(shared_ptr<StackGraphNode> node, unordered_map<Coordinate, shared_ptr<StackGraphNode>, CustomHash> &map)
{
    Coordinate coord(node->location.line, node->location.column, node->symbol);
    map.insert({coord, node});
    for (auto ch : node->children)
    {
        _index(ch, map);
    }
}

void StackGraphEngine::loadFile(string path)
{
    std::ifstream file_stream(path);
    std::stringstream buffer;
    buffer << file_stream.rdbuf();

    auto contents = buffer.str();
    auto source_code = contents.c_str();

    // Create a parser.
    TSParser *parser = ts_parser_new();

    // Set the parser's language (JSON in this case).
    ts_parser_set_language(parser, tree_sitter_c());

    TSTree *tree = ts_parser_parse_string(
        parser,
        NULL,
        source_code,
        strlen(source_code));

    TSNode root_node = ts_tree_root_node(tree);

    auto sg_tree = build_stack_graph_tree(root_node, source_code);

    this->symbol_tree = std::tuple<std::string, shared_ptr<StackGraphNode>>(path, sg_tree);
    _index(sg_tree, this->node_table);
}

string _pop_stack(string &stack)
{
    size_t pos_end = stack.find(".");
    if (pos_end == string::npos)
    {
        string prev = stack;
        stack = "";
        return prev;
    }
    else
    {
        string ret = stack.substr(0, pos_end);
        stack = stack.substr(pos_end + 1, string::npos);
        return ret;
    }
}

void _push_stack(string &stack, string val)
{
    stack = val + "." + stack;
}

shared_ptr<StackGraphNode> _find_in_parents(shared_ptr<StackGraphNode> node, string elem)
{
    shared_ptr<StackGraphNode> it = node;
    while (it != nullptr)
    {
        for (auto ch : it->children)
        {
            if (ch->symbol == elem)
            {
                return ch;
            }
        }
        it = it->parent;
    }
    return nullptr;
}

shared_ptr<StackGraphNode> _find_in_children(shared_ptr<StackGraphNode> node, string elem)
{
    for (auto ch : node->children)
    {
        if (ch->symbol == elem)
        {
            return ch;
        }
    }
    return nullptr;
}

shared_ptr<Coordinate> StackGraphEngine::resolve(Coordinate coord)
{
    auto search = this->node_table.find(coord);
    if (search == this->node_table.end())
    {
        return nullptr;
    }

    auto value = search->second;

    if (value->kind != StackGraphNodeKind::REFERENCE)
    {
        return nullptr;
    }

    string stack = value->symbol;

    std::cout << "Looking up reference" << std::endl;
    std::cout << "Stack: " << stack << std::endl;

    shared_ptr<StackGraphNode> current = value;

    string elem;

    while (stack != "")
    {
        if (current->kind == StackGraphNodeKind::REFERENCE)
        {
            elem = _pop_stack(stack);
            auto next_val = _find_in_parents(current, elem);
            if (next_val == nullptr)
                break;
            current = next_val;
        }
        else if (current->kind == StackGraphNodeKind::SYMBOL)
        {
            auto next_val = current->jump_to;
            current = next_val;
            std::cout << "Node Jump" << std::endl;
        }
        else if (current->kind == StackGraphNodeKind::NAMED_SCOPE)
        {
            elem = _pop_stack(stack);
            auto next_val = _find_in_children(current, elem);
            if (next_val == nullptr)
                break;
            current = next_val;
        }

        std::cout << "Stack: " << stack << std::endl;
    }

    if (stack == "")
    {
        Point p = current->location;
        return shared_ptr<Coordinate>(new Coordinate(p.line, p.column, current->symbol));
    }
    else
    {
        return nullptr;
    }
}