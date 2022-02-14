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
        engine.crossLink();
        auto end = high_resolution_clock::now();
        
        auto duration = duration_cast<milliseconds>(end-start);

        json res;
        res["command"] = "index";
        res["status"] = "ok";
        res["time_ms"] = duration.count();

        std::cout << res.dump() << std::endl;
    }


};

int main()
{
    Reactor r;

    r.loop();

    return 0;
}