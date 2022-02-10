#include <utility>
#include <string>
#include <stack-graph-tree.h>
#include <unordered_map>
#include <unordered_set>
#include <tuple>
#include <sstream>

using std::string;
using std::stringstream;
using std::tuple;
using std::unordered_map;
using std::unordered_set;

#ifndef STACK_GRAPH_ENGINE_H
#define STACK_GRAPH_ENGINE_H

namespace stack_graph
{
    typedef tuple<string, string> Coordinate;
    typedef tuple<string, uint32_t, uint32_t> Resolution;

    class CustomHash
    {
    public:
        // Implement a hash function
        std::size_t operator()(const Coordinate &k) const
        {
            std::size_t h1 = std::hash<string>{}(std::get<0>(k));
            std::size_t h2 = std::hash<string>{}(std::get<1>(k));
            return 31 * h2 + h1;
        }
    };

    struct CrossLink {
        shared_ptr<StackGraphNode> symbol;
        shared_ptr<StackGraphNode> definition;

        CrossLink(shared_ptr<StackGraphNode> symbol, shared_ptr<StackGraphNode> definition){
            this->symbol = symbol;
            this->definition = definition;
        }

        string repr(){
            stringstream ss;

            shared_ptr<StackGraphNode> it = symbol;
            while(it->parent != nullptr){
                it = it->parent;
            }

            auto sym_file = it->symbol;

            it=definition;
            while(it->parent != nullptr){
                it = it->parent;
            }

            auto def_file = it->symbol;

            ss << "Cross Link {" << sym_file << "#" << symbol->symbol << " ~~> " << def_file << "#" << definition->symbol << "}";
            return ss.str(); 
        }
    };

    struct StackGraphEngine
    {
        unordered_map<Coordinate, shared_ptr<StackGraphNode>, CustomHash> node_table;
        unordered_map<string, shared_ptr<StackGraphNode>> translation_units;
        vector<CrossLink> cross_links;

        void loadFile(string path);

        void loadDirectoryRecursive(string path);

        string resolveImport(string import);

        shared_ptr<Resolution> resolve(Coordinate c);

        vector<string> importsForTranslationUnit(string path);

        vector<shared_ptr<StackGraphNode>> exportedDefinitionsForTranslationUnit(string path);

        vector<shared_ptr<StackGraphNode>> symbolsForTranslationUnit(string path);

        void _visitUnitsInTopologicalOrder(unordered_map<string, unordered_map<string, shared_ptr<StackGraphNode>>>& cache,
                                           unordered_set<string>& visited,
                                           unordered_map<string, string>& h_to_c,
                                           string unit);

        void crossLink();
    };
}

#endif