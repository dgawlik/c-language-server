#include <stack-graph-tree.h>
#include <stack-graph-engine.h>
#include <tree_sitter/api.h>
#include <iostream>
#include <tuple>
#include <string>
#include <json.hpp>
#include <cstdlib>
#include <chrono>

using json = nlohmann::json;

using namespace std::chrono;

using std::shared_ptr;
using std::string;
using std::stringstream;
using std::vector;

using stack_graph::Coordinate;
using stack_graph::Point;
using stack_graph::StackGraphEngine;

// Command Object:
// {
//     "command" : "command-name",
//     "payload" : {...}
// }
// Response Object:
// {
//     "command": "command-name",
//     "status": "ok|error",
//     "payload": {...}
// }

constexpr unsigned int hash(const char *s, int off = 0)
{
    return !s[off] ? 5381 : (hash(s, off + 1) * 33) ^ s[off];
}

struct Reactor
{
    StackGraphEngine engine;

    void loop()
    {
        while (true)
        {
            std::string line;

            std::getline(std::cin, line);

            json parsed = json::parse(line);

            switch (hash(parsed["command"].get<string>().c_str()))
            {
            case hash("stop"):
                exit(0);
            case hash("index"):
                do_index(parsed["payload"]);
                break;
            case hash("resolve"):
                resolve(parsed["payload"]);
                break;
            case hash("find_usages"):
                find_usages(parsed["payload"]);
                break;
            default:
                std::cout << line << std::endl;
            }
        }
    }

    void do_index(json payload){
        string path = payload["path"].get<string>();
        auto excludes = payload["excludes"].get<vector<string>>();
        
        auto start = high_resolution_clock::now();
        engine.loadDirectoryRecursive(path, excludes);
        auto end = high_resolution_clock::now();
        
        auto duration = duration_cast<milliseconds>(end-start);

        json res;
        res["command"] = "index";
        res["status"] = "done_indexing";
        res["time_ms"] = duration.count();

        std::cout << res.dump() << std::endl;

        start = high_resolution_clock::now();
        engine.crossLink();
        end = high_resolution_clock::now();
        
        duration = duration_cast<milliseconds>(end-start);

        res["command"] = "index";
        res["status"] = "done_crosslinking";
        res["time_ms"] = duration.count();

        std::cout << res.dump() << std::endl;
    }

    void resolve(json payload){
        Coordinate coord(
            payload["path"].get<string>(),
            payload["line"].get<int>(),
            payload["column"].get<int>()
        );

        auto start = high_resolution_clock::now();
        shared_ptr<Coordinate> result = engine.resolve(coord);
        auto end = high_resolution_clock::now();

        auto duration = duration_cast<milliseconds>(end-start);

        json res;
        res["command"] = "resolve";
        res["time_ms"] = duration.count();

        if(result == nullptr){
            res["status"] = "not_found";
        }
        else {
            res["status"] = "ok";
            res["coordinate"] = {{"path", result->path}, {"line", result->line}, {"column", result->column}};
        }

        std::cout << res.dump() << std::endl;
    }

    void find_usages(json payload){
        Coordinate coord(
            payload["path"].get<string>(),
            payload["line"].get<int>(),
            payload["column"].get<int>()
        );

        auto start = high_resolution_clock::now();
        vector<shared_ptr<Coordinate>> lst = engine.findUsages(coord);
        auto end = high_resolution_clock::now();

        auto duration = duration_cast<milliseconds>(end-start);

        json res;
        res["command"] = "find_usages";
        res["time_ms"] = duration.count();
        res["status"] = "ok";
        res["coordinates"] = {};
        for(auto & l : lst){
            res["coordinates"].push_back({{"path", l->path}, {"line", l->line}, {"column", l->column}});
        }
        
        std::cout << res.dump() << std::endl;
    }


};

int main()
{
    Reactor r;

    r.loop();

    return 0;
}