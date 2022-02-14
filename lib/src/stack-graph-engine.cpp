

#include <stack-graph-engine.h>
#include <fstream>
#include <tuple>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include <re2/re2.h>

namespace fs = std::filesystem;

using stack_graph::build_stack_graph_tree;
using stack_graph::Coordinate;
using stack_graph::Point;
using stack_graph::StackGraphEngine;
using stack_graph::StackGraphNode;
using stack_graph::StackGraphNodeKind;

extern "C" TSLanguage *tree_sitter_c();

void _index(string &path, shared_ptr<StackGraphNode> node, unordered_map<Coordinate, shared_ptr<StackGraphNode>> &map)
{
    Coordinate coord(path, node->location.line, node->location.column);
    map[coord] = node;
    for (auto ch : node->children)
    {
        _index(path, ch, map);
    }
}

bool StackGraphEngine::loadFile(string path)
{
    std::ifstream file_stream(path);
    std::stringstream buffer;
    buffer << file_stream.rdbuf();

    auto contents = buffer.str();
    auto source_code = contents.c_str();

    file_stream.close();

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
    bool ret = false;
    if (sg_tree != nullptr)
    {
        sg_tree->symbol = path;
        // std::cout << sg_tree->repr() << std::endl;

        this->translation_units[path] = sg_tree;
        _index(path, sg_tree, this->node_table);
        ret = true;
    }

    ts_tree_delete(tree);
    return ret;
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

bool _contains_segment(string stack, string segment)
{
    size_t pos_begin = 0, pos_end;
    while (true)
    {
        pos_end = stack.find(".", pos_begin);
        string seg = stack.substr(pos_begin, pos_end);

        if (seg == segment)
        {
            return true;
        }

        if (pos_end == string::npos)
        {
            return false;
        }
        else
        {
            pos_begin = pos_end + 1;
        }
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

    if (value->kind == StackGraphNodeKind::NAMED_SCOPE)
    {
        return nullptr;
    }

    string stack = value->symbol;

    // std::cout << "Looking up reference" << std::endl;
    // std::cout << "Stack: " << stack << std::endl;

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
            if (current->jump_to == nullptr)
            {
                break;
            }
            current = next_val;
            // std::cout << "Node Jump" << std::endl;
        }
        else if (current->kind == StackGraphNodeKind::NAMED_SCOPE)
        {
            elem = _pop_stack(stack);
            auto next_val = _find_in_children(current, elem);
            if (next_val == nullptr)
                break;
            current = next_val;
        }

        // std::cout << "Stack: " << stack << std::endl;
    }

    if (stack == "")
    {
        auto it = current;
        while (it->parent != nullptr)
            it = it->parent;

        auto res = new Coordinate(it->symbol, current->location.line, current->location.column);
        return shared_ptr<Coordinate>(res);
    }
    else
    {
        return nullptr;
    }
}

void StackGraphEngine::loadDirectoryRecursive(string path, std::vector<string> excludes)
{
    std::regex regex("[a-z0-9\\-_]*\\.(c|h)");
    for (const auto &entry : fs::recursive_directory_iterator(path))
    {
        if (!entry.is_directory())
        {
            auto path = entry.path();
            auto file = path.filename().string();

            if (std::any_of(excludes.begin(), excludes.end(), [&](string r)
                            { return RE2::PartialMatch(path.string(), r); }))
            {
                continue;
            }

            if (std::regex_match(file, regex))
            {
                // std::cout << path.string() << std::endl;
                if (this->loadFile(path))
                {
                    this->name_to_path.insert({file, path.string()});
                }
            }
        }
    }
}

void _walk_tree(shared_ptr<StackGraphNode> node, std::function<bool(shared_ptr<StackGraphNode>)> pred, std::function<void(shared_ptr<StackGraphNode>)> cbk)
{
    if (pred(node))
    {
        cbk(node);
    }
    for (auto ch : node->children)
    {
        _walk_tree(ch, pred, cbk);
    }
}

vector<string> StackGraphEngine::importsForTranslationUnit(string path)
{
    vector<string> lst;

    try
    {
        auto root = this->translation_units.at(path);
        _walk_tree(
            root,
            [](shared_ptr<StackGraphNode> node)
            { return node->kind == StackGraphNodeKind::IMPORT; },
            [&](shared_ptr<StackGraphNode> node)
            { lst.push_back(node->symbol); });
    }
    catch (std::out_of_range ex)
    {
    }

    return lst;
}

vector<shared_ptr<StackGraphNode>> StackGraphEngine::exportedDefinitionsForTranslationUnit(string path)
{
    vector<shared_ptr<StackGraphNode>> lst;

    auto itr = this->translation_units.find(path);
    if (itr == this->translation_units.end())
    {
        return lst;
    }

    auto val = itr->second;

    for (auto ch : val->children)
    {
        if (ch->kind == StackGraphNodeKind::NAMED_SCOPE)
        {
            lst.push_back(ch);
        }
    }

    return lst;
}

vector<shared_ptr<StackGraphNode>> StackGraphEngine::symbolsForTranslationUnit(string path)
{
    vector<shared_ptr<StackGraphNode>> lst;

    auto itr = this->translation_units.find(path);
    if (itr == this->translation_units.end())
    {
        return lst;
    }

    _walk_tree(
        itr->second,
        [](shared_ptr<StackGraphNode> node)
        { return node->kind == StackGraphNodeKind::SYMBOL; },
        [&](shared_ptr<StackGraphNode> node)
        { lst.push_back(node); });

    return lst;
}

string StackGraphEngine::resolveImport(string import)
{
    auto file = fs::path(import).filename().string();

    const auto found = this->name_to_path.equal_range(file);

    for (auto i = found.first; i != found.second; ++i)
    {
        if (i->second.find(import) != string::npos)
        {
            return i->second;
        }
    }

    return "";
}

void StackGraphEngine::_visitUnitsInTopologicalOrder(
    unordered_map<string, unordered_map<string, shared_ptr<StackGraphNode>>> &cache,
    unordered_set<string> &visited,
    unordered_map<string, string> &h_to_c,
    string unit)
{

    if (visited.find(unit) != visited.end())
    {
        return;
    }
    visited.insert(unit);

    // std::cout << unit << std::endl;

    unordered_map<string, shared_ptr<StackGraphNode>> transitive_defs;

    for (auto import : this->importsForTranslationUnit(unit))
    {
        auto path_import = this->resolveImport(import);

        if (path_import != "")
        {

            if (cache.find(path_import) == cache.end() && visited.find(path_import) == visited.end())
            {

                this->_visitUnitsInTopologicalOrder(cache, visited, h_to_c, path_import);
            }

            if (cache.find(path_import) != cache.end())
            {
                auto defs = cache.find(path_import)->second;
                for (auto &entry : defs)
                {
                    transitive_defs.insert(entry);
                }
            }
        }
    }

    for (auto &def : this->exportedDefinitionsForTranslationUnit(unit))
    {
        transitive_defs.insert({def->symbol, def});
    }

    if (h_to_c.find(unit) != h_to_c.end())
    {
        auto c_file = h_to_c.find(unit)->second;

        for (auto &def : this->exportedDefinitionsForTranslationUnit(c_file))
        {
            transitive_defs.insert({def->symbol, def});
        }
    }

    // now all the transitive definitions are loaded

    for (auto &sym : this->symbolsForTranslationUnit(unit))
    {
        if (transitive_defs.find(sym->symbol) != transitive_defs.end())
        {
            auto def = transitive_defs.find(sym->symbol)->second;
            sym->jump_to = def;
            this->cross_links.push_back(CrossLink(sym, def));
        }
    }

    cache.insert({unit, transitive_defs});
}

void StackGraphEngine::crossLink()
{

    this->h_to_c.clear();

    for (auto &entry : this->translation_units)
    {
        auto k = entry.first;
        auto v = entry.second;

        if (RE2::FullMatch(fs::path(k).filename().string(), "[a-z0-9\\-_]*\\.c"))
        {
            for (auto &import : this->importsForTranslationUnit(k))
            {
                auto abs_import = this->resolveImport(import);

                if (RE2::FullMatch(import, "[a-z0-9\\-_]*\\.h") && abs_import != "")
                {
                    this->h_to_c[abs_import] = k;
                    // std::cout << k << std::endl;
                }
            }
        }
    }

    unordered_map<string, unordered_map<string, shared_ptr<StackGraphNode>>> cache;
    unordered_set<string> visited;
    this->cross_links.clear();

    for (auto &entry : this->translation_units)
    {
        _visitUnitsInTopologicalOrder(cache, visited, h_to_c, entry.first);
    }
}

vector<shared_ptr<Coordinate>> StackGraphEngine::findUsages(Coordinate coord)
{
    vector<shared_ptr<Coordinate>> lst;
    auto search = this->node_table.find(coord);
    if (search == this->node_table.end())
    {
        return lst;
    }

    auto value = search->second;

    if (value->kind == StackGraphNodeKind::NAMED_SCOPE)
    {
        for (auto &kv : this->node_table)
        {
            auto v = kv.second;
            if (v->kind == StackGraphNodeKind::SYMBOL && v->jump_to != nullptr && v->jump_to == value)
            {

                auto it = v;
                while (it->parent != nullptr)
                {
                    it = it->parent;
                }

                auto res = new Coordinate(it->symbol, v->location.line, v->location.column);
                lst.push_back(shared_ptr<Coordinate>(res));
            }
        }
    }
    else if (value->kind == StackGraphNodeKind::SYMBOL)
    {
        for (auto &kv : this->node_table)
        {
            auto v = kv.second;
            if (v->kind == StackGraphNodeKind::REFERENCE && _contains_segment(v->symbol, value->symbol))
            {

                auto it = v;
                while (it->parent != nullptr)
                {
                    it = it->parent;
                }

                auto res = new Coordinate(it->symbol, v->location.line, v->location.column);
                lst.push_back(shared_ptr<Coordinate>(res));
            }
        }
    }

    return lst;
}