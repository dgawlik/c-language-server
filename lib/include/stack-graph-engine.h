#include <utility>
#include <string>
#include <stack-graph-tree.h>
#include <unordered_map>
#include <unordered_set>
#include <tuple>
#include <sstream>
#include <functional>

using std::string;
using std::stringstream;
using std::tuple;
using std::unordered_map;
using std::unordered_set;

#ifndef STACK_GRAPH_ENGINE_H
#define STACK_GRAPH_ENGINE_H

namespace stack_graph
{
    struct Coordinate
    {
        string path;
        uint32_t line;
        uint32_t column;

        Coordinate(string path, uint32_t line, uint32_t column)
        {
            this->path = path;
            this->line = line;
            this->column = column;
        }

        bool operator==(const Coordinate &other) const
        {
            return (path == other.path && line == other.line && column == other.column);
        }
    };
}

template <>
struct std::hash<stack_graph::Coordinate>
{
    std::size_t operator()(const stack_graph::Coordinate &k) const
    {
        using std::hash;
        using std::size_t;
        using std::string;

        std::size_t h1 = std::hash<string>{}(k.path);
        std::size_t h2 = std::hash<uint32_t>{}(k.line);
        std::size_t h3 = std::hash<uint32_t>{}(k.column);
        return 31 * 31 * h3 + 31 * h2 + h1;
    }
};

namespace stack_graph
{

    struct CrossLink
    {
        shared_ptr<StackGraphNode> symbol;
        shared_ptr<StackGraphNode> definition;

        CrossLink(shared_ptr<StackGraphNode> symbol, shared_ptr<StackGraphNode> definition)
        {
            this->symbol = symbol;
            this->definition = definition;
        }

        string repr()
        {
            stringstream ss;

            shared_ptr<StackGraphNode> it = symbol;
            while (it->parent != nullptr)
            {
                it = it->parent;
            }

            auto sym_file = it->symbol;

            it = definition;
            while (it->parent != nullptr)
            {
                it = it->parent;
            }

            auto def_file = it->symbol;

            ss << "Cross Link {" << sym_file << "#" << symbol->symbol << " ~~> " << def_file << "#" << definition->symbol << "}";
            return ss.str();
        }
    };

    struct StackGraphEngine
    {
        unordered_map<Coordinate, shared_ptr<StackGraphNode>> node_table;
        unordered_map<string, shared_ptr<StackGraphNode>> translation_units;
        std::multimap<string, string> name_to_path;
        vector<CrossLink> cross_links;
        unordered_map<string, string> h_to_c;

        bool loadFile(string path);

        void loadDirectoryRecursive(string path, std::vector<string> excludes);

        string resolveImport(string import);

        shared_ptr<Coordinate> resolve(Coordinate c);

        vector<string> importsForTranslationUnit(string path);

        vector<shared_ptr<StackGraphNode>> exportedDefinitionsForTranslationUnit(string path);

        vector<shared_ptr<StackGraphNode>> symbolsForTranslationUnit(string path);

        void _visitUnitsInTopologicalOrder(unordered_map<string, unordered_map<string, shared_ptr<StackGraphNode>>> &cache,
                                           unordered_set<string> &visited,
                                           unordered_map<string, string> &h_to_c,
                                           string unit);

        void crossLink();

        vector<shared_ptr<Coordinate>> findUsages(Coordinate coord);
    };
}

#endif