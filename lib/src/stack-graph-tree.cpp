
#include <stack-graph-tree.h>
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

using stack_graph::Point;
using stack_graph::Range;
using stack_graph::StackGraphNode;
using stack_graph::StackGraphNodeKind;


struct TSNodeWrapper;
void _do_print_repr(stringstream &ss, TSNodeWrapper node, int level);

struct TSNodeWrapper
{
    TSNode tsnode;

    TSNodeWrapper(TSNode tsnode)
    {
        this->tsnode = tsnode;
    }

    string type()
    {
        return ts_node_type(this->tsnode);
    }

    shared_ptr<TSNodeWrapper> childByType(string type)
    {
        for (auto ch : this->children())
        {
            if (ch->type() == type)
            {
                return ch;
            }
        }
        return nullptr;
    }

    shared_ptr<TSNodeWrapper> childByFieldName(string type)
    {
        return shared_ptr<TSNodeWrapper>(new TSNodeWrapper(
            ts_node_child_by_field_name(this->tsnode, type.c_str(), type.length())));
    }

    vector<shared_ptr<TSNodeWrapper>> children()
    {
        vector<shared_ptr<TSNodeWrapper>> children;
        for (int i = 0; i < ts_node_child_count(this->tsnode); i++)
        {
            children.push_back(shared_ptr<TSNodeWrapper>(new TSNodeWrapper(ts_node_child(this->tsnode, i))));
        }
        return children;
    }

    string text(string &code)
    {
        return code.substr(this->range().start, this->range().end - this->range().start);
    }

    Point editorPosition()
    {
        TSPoint point = ts_node_start_point(this->tsnode);
        struct Point _point = {.line = point.row, .column = point.column};
        return _point;
    }

    Range range()
    {
        struct Range range = {.start = ts_node_start_byte(this->tsnode), .end = ts_node_end_byte(this->tsnode)};
        return range;
    }

    string repr()
    {
        stringstream ss;
        _do_print_repr(ss, *this, 0);
        return ss.str();
    }
};

void _do_print_repr(stringstream &ss, TSNodeWrapper node, int level)
{
    for (int i = 0; i < level; i++)
    {
        ss << "|  ";
    }
    ss << "|-" << node.type() << std::endl;

    for (auto ch : node.children())
    {
        _do_print_repr(ss, *ch, level + 1);
    }
}



const char *kind_names[] = {
    "NAMED_SCOPE",
    "SYMBOL",
    "REFERENCE",
    "UNNAMED_SCOPE"};


void _do_print_repr_stree(stringstream &ss, StackGraphNode node, int level);

std::string StackGraphNode::repr(){
    stringstream ss;
    _do_print_repr_stree(ss, *this, 0);
    return ss.str();
}


void _do_print_repr_stree(stringstream &ss, StackGraphNode node, int level)
{
    for (int i = 0; i < level; i++)
    {
        ss << "|  ";
    }
    ss << "|-" << kind_names[node.kind] << "[" << node.symbol << "]";
    if (node.jump_to != nullptr)
    {
        ss << "~> " << node.jump_to->_type;
    }
    ss << std::endl;

    for (auto ch : node.children)
    {
        _do_print_repr_stree(ss, *ch, level + 1);
    }
}

struct _Context
{
    string state;
    shared_ptr<StackGraphNode> jump_to;
    string type;

    _Context()
    {
        state = "";
        jump_to = nullptr;
        type = "";
    }
};

shared_ptr<StackGraphNode> try_resolve_type(vector<shared_ptr<StackGraphNode>> cpy_stack, string type)
{
    if (cpy_stack.size() == 0)
    {
        return nullptr;
    }

    shared_ptr<StackGraphNode> it;
    while (cpy_stack.size() != 0)
    {
        it = cpy_stack.back();
        cpy_stack.pop_back();

        for (auto ch : it->children)
        {
            if (ch->_type == type && ch->kind == StackGraphNodeKind::NAMED_SCOPE)
            {
                return ch;
            }
        }
    }
    return it;
}

string sanitize_translate_reference(string text)
{
    std::regex subscript_regex("\\[.*\\]");
    std::regex call_regex("\\(.*\\)");
    std::regex pref_regex("->");
    std::regex p_regex("\\*");

    text = std::regex_replace(text, subscript_regex, "");
    text = std::regex_replace(text, call_regex, "");
    text = std::regex_replace(text, pref_regex, ".");
    return std::regex_replace(text, p_regex, "");
}

void build_stack_graph(vector<shared_ptr<StackGraphNode>> &stack, string &code, TSNodeWrapper node, _Context& ctx)
{
    if (node.type() == "function_declarator" && ctx.state == "function_definition")
    {
        auto id_node = node.childByType("identifier");
        stack.back()->symbol = id_node->text(code);
        stack.back()->_type = id_node->text(code);

        auto parameters_node = node.childByType("parameter_list");
        build_stack_graph(stack, code, *parameters_node, ctx);
    }
    else if (node.type() == "type_identifier" && ctx.state == "populate_type")
    {
        ctx.type = node.text(code);
    }
    else if ((node.type() == "identifier" || node.type() == "field_identifier") && ctx.state == "declaration")
    {
        auto symbol_node = new StackGraphNode(StackGraphNodeKind::SYMBOL, node.text(code), node.editorPosition());
        symbol_node->parent = stack.back();
        symbol_node->jump_to = ctx.jump_to;
        symbol_node->_type = ctx.type;
        auto symbol_node_ptr = shared_ptr<StackGraphNode>(symbol_node);
        stack.back()->children.push_back(symbol_node_ptr);
    }
    else if (node.type() == "compound_statement")
    {
        if (ctx.state != "skip_compound")
        {
            auto symbol_node = new StackGraphNode(StackGraphNodeKind::UNNAMED_SCOPE, "", node.editorPosition());
            symbol_node->parent = stack.back();
            auto symbol_node_ptr = shared_ptr<StackGraphNode>(symbol_node);
            stack.back()->children.push_back(symbol_node_ptr);
            stack.push_back(symbol_node_ptr);
        }

        for (auto ch : node.children())
        {
            build_stack_graph(stack, code, *ch, ctx);
        }

        if (ctx.state != "skip_compound")
        {
            stack.pop_back();
        }
    }
    else if (node.type() == "declaration" || node.type() == "parameter_declaration" || node.type() == "field_declaration")
    {
        auto specifiers_node = node.childByFieldName("type");
        _Context ctx2;
        ctx2.state = "populate_type";
        build_stack_graph(stack, code, *specifiers_node, ctx2);

        if (ctx2.jump_to == nullptr)
        {
            ctx2.jump_to = try_resolve_type(stack, ctx2.type);
        }

        for (int i = 1; i < node.children().size(); i += 2)
        {
            ctx2.state = "declaration";
            build_stack_graph(stack, code, *(node.children()[i]), ctx2);
        }
    }
    else if (node.type() == "translation_unit")
    {
        auto sg_node = shared_ptr<StackGraphNode>(new StackGraphNode(StackGraphNodeKind::NAMED_SCOPE, "translation_unit", node.editorPosition()));
        sg_node->_type = "root";
        stack.push_back(sg_node);

        for (auto ch : node.children())
        {
            build_stack_graph(stack, code, *ch, ctx);
        }
    }
    else if (node.type() == "struct_specifier" || node.type() == "enum_specifier")
    {
        auto symbol_node = node.childByType("type_identifier");
        auto kind = symbol_node != nullptr ? StackGraphNodeKind::NAMED_SCOPE : StackGraphNodeKind::UNNAMED_SCOPE;
        auto symbol_node_text = symbol_node != nullptr ? symbol_node->text(code) : "";

        StackGraphNode *struct_node = new StackGraphNode(kind, symbol_node_text, node.editorPosition());
        struct_node->parent = stack.back();
        struct_node->_type = symbol_node_text;
        auto struct_node_ptr = shared_ptr<StackGraphNode>(struct_node);
        stack.back()->children.push_back(struct_node_ptr);
        stack.push_back(struct_node_ptr);

        if (ctx.state == "populate_type")
        {
            ctx.jump_to = struct_node_ptr;
            ctx.type = symbol_node_text;
        }
        else
        {
            ctx.type = symbol_node_text;
        }

        auto list_type = node.type() == "struct_specifier" ? "field_declaration_list" : "enumerator_list";
        auto field_decl_list_node = node.childByType(list_type);

        if (field_decl_list_node != nullptr)
        {
            build_stack_graph(stack, code, *field_decl_list_node, ctx);
        }
        else
        {
            stack.back()->kind = StackGraphNodeKind::SYMBOL;
            stack.back()->jump_to = try_resolve_type(stack, symbol_node_text);
        }

        stack.pop_back();
    }

    else if (node.type() == "function_definition")
    {
        auto kind = StackGraphNodeKind::NAMED_SCOPE;

        StackGraphNode *function_node = new StackGraphNode(kind, "", node.editorPosition());
        function_node->parent = stack.back();
        auto function_node_ptr = shared_ptr<StackGraphNode>(function_node);
        stack.back()->children.push_back(function_node_ptr);
        stack.push_back(function_node_ptr);

        auto declarator_node = node.childByType("function_declarator");
        _Context ctx2;
        ctx2.state = "function_definition";

        build_stack_graph(stack, code, *declarator_node, ctx2);

        ctx2.state = "skip_compound";
        auto body_node = node.childByType("compound_statement");
        build_stack_graph(stack, code, *body_node, ctx2);

        stack.pop_back();
    }
    else if (node.type() == "identifier" || node.type() == "call_expression" || node.type() == "field_expression" || node.type() == "pointer_expression" || node.type() == "subscript_expression")
    {
        auto ref_text = node.text(code);
        ref_text = sanitize_translate_reference(ref_text);

        auto ref_node = new StackGraphNode(StackGraphNodeKind::REFERENCE, ref_text, node.editorPosition());
        ref_node->parent = stack.back();
        auto ref_node_ptr = shared_ptr<StackGraphNode>(ref_node);

        stack.back()->children.push_back(ref_node_ptr);
    }
    else
    {
        for (auto ch : node.children())
        {
            build_stack_graph(stack, code, *ch, ctx);
        }
    }
}

shared_ptr<StackGraphNode> stack_graph::build_stack_graph_tree(TSNode root, const char* source_code){

    vector<shared_ptr<StackGraphNode>> stack;
    string code = source_code;
    _Context context;
    build_stack_graph(stack, code, root, context);

    return stack.back();
}