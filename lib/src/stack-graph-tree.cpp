
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
#include <re2/re2.h>

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

    TSNodeWrapper(TSNodeWrapper &other){
        this->tsnode = other.tsnode;
    }

    TSNodeWrapper(TSNodeWrapper &&other) : tsnode(std::move(other.tsnode)) {}

    const char *type()
    {
        return ts_node_type(this->tsnode);
    }

    shared_ptr<TSNodeWrapper> childByFieldName(const char *type)
    {
        auto node = ts_node_child_by_field_name(this->tsnode, type, strlen(type));
        return ts_node_is_null(node) ? nullptr : shared_ptr<TSNodeWrapper>(new TSNodeWrapper(node));
    }

    uint32_t child_count()
    {
        return ts_node_child_count(this->tsnode);
    }

    shared_ptr<TSNodeWrapper> child(uint32_t ind)
    {
        return shared_ptr<TSNodeWrapper>(new TSNodeWrapper(ts_node_child(this->tsnode, ind)));
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
        _do_print_repr(ss, std::move(*this), 0);
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

    for (uint32_t i = 0; i < node.child_count(); i++)
    {
        _do_print_repr(ss, std::move(*node.child(i)), level + 1);
    }
}

const char *kind_names[] = {
    "NAMED_SCOPE",
    "SYMBOL",
    "REFERENCE",
    "UNNAMED_SCOPE",
    "IMPORT"};

void _do_print_repr_stree(stringstream &ss, StackGraphNode node, int level);

std::string StackGraphNode::repr()
{
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
    Point location;

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
    return nullptr;
}

void build_stack_graph(vector<shared_ptr<StackGraphNode>> &stack, string &code, TSNodeWrapper node, _Context &ctx)
{

    if (strcmp(node.type(), "ERROR") == 0)
    {
        return;
    }
    else if (strcmp(node.type(), "identifier") == 0 && ctx.state == "function_declarator")
    {
        stack.back()->symbol = node.text(code);
        stack.back()->_type = node.text(code);
        stack.back()->location = node.editorPosition();
    }
    if (strcmp(node.type(), "function_declarator") == 0 && ctx.state == "function_definition")
    {
        auto id_node = node.childByFieldName("declarator");
        _Context ctx2;
        ctx2.state = "function_declarator";
        build_stack_graph(stack, code, std::move(*id_node), ctx2);

        auto parameters_node = node.childByFieldName("parameters");
        build_stack_graph(stack, code, std::move(*parameters_node), ctx);
    }
    else if (strcmp(node.type(), "type_identifier") == 0 && ctx.state == "populate_type")
    {
        ctx.type = node.text(code);
        ctx.location = node.editorPosition();
    }
    else if ((strcmp(node.type(), "identifier") == 0 || strcmp(node.type(), "field_identifier") == 0) && ctx.state == "declaration")
    {
        auto symbol_node = new StackGraphNode(StackGraphNodeKind::SYMBOL, node.text(code), node.editorPosition());
        symbol_node->parent = stack.back();
        symbol_node->jump_to = ctx.jump_to;
        symbol_node->_type = ctx.type;
        auto symbol_node_ptr = shared_ptr<StackGraphNode>(symbol_node);
        stack.back()->children.push_back(symbol_node_ptr);
    }
    else if (strcmp(node.type(), "compound_statement") == 0)
    {
        if (ctx.state != "skip_compound")
        {
            auto symbol_node = new StackGraphNode(StackGraphNodeKind::UNNAMED_SCOPE, "", node.editorPosition());
            symbol_node->parent = stack.back();
            auto symbol_node_ptr = shared_ptr<StackGraphNode>(symbol_node);
            stack.back()->children.push_back(symbol_node_ptr);
            stack.push_back(symbol_node_ptr);
        }

        for (uint32_t i = 0; i < node.child_count(); i++)
        {
            build_stack_graph(stack, code, std::move(*node.child(i)), ctx);
        }

        if (ctx.state != "skip_compound")
        {
            stack.pop_back();
        }
    }
    else if (strcmp(node.type(), "declaration") == 0 || strcmp(node.type(), "parameter_declaration") == 0 || strcmp(node.type(), "field_declaration") == 0)
    {
        auto specifiers_node = node.childByFieldName("type");
        _Context ctx2;
        ctx2.state = "populate_type";
        build_stack_graph(stack, code, std::move(*specifiers_node), ctx2);

        if (ctx2.jump_to == nullptr)
        {
            ctx2.jump_to = try_resolve_type(stack, ctx2.type);
        }

        for (uint32_t i = 1; i < node.child_count(); i += 2)
        {
            ctx2.state = "declaration";
            build_stack_graph(stack, code, std::move(*node.child(i)), ctx2);
        }
    }
    else if (strcmp(node.type(), "translation_unit") == 0)
    {
        auto sg_node = shared_ptr<StackGraphNode>(new StackGraphNode(StackGraphNodeKind::NAMED_SCOPE, "translation_unit", node.editorPosition()));
        sg_node->_type = "root";
        stack.push_back(sg_node);

        for (uint32_t i = 0; i < node.child_count(); i++)
        {
            build_stack_graph(stack, code, std::move(*node.child(i)), ctx);
        }
    }
    else if (strcmp(node.type(), "struct_specifier") == 0 || strcmp(node.type(), "enum_specifier") == 0)
    {
        auto symbol_node = node.childByFieldName("name");
        auto kind = symbol_node != nullptr ? StackGraphNodeKind::NAMED_SCOPE : StackGraphNodeKind::UNNAMED_SCOPE;
        auto symbol_node_text = symbol_node != nullptr ? symbol_node->text(code) : "";

        StackGraphNode *struct_node = new StackGraphNode(kind, symbol_node_text, node.editorPosition());
        struct_node->parent = stack.back();
        struct_node->_type = symbol_node_text;
        if (symbol_node != nullptr)
        {
            struct_node->location = symbol_node->editorPosition();
        }
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

        auto field_decl_list_node = node.childByFieldName("body");

        if (field_decl_list_node != nullptr)
        {
            build_stack_graph(stack, code, std::move(*field_decl_list_node), ctx);
        }
        else
        {
            stack.back()->kind = StackGraphNodeKind::SYMBOL;
            vector<shared_ptr<StackGraphNode>> cpy_stack(stack);
            stack.back()->jump_to = try_resolve_type(cpy_stack, symbol_node_text);
        }

        stack.pop_back();
    }

    else if (strcmp(node.type(), "function_definition") == 0)
    {
        auto kind = StackGraphNodeKind::NAMED_SCOPE;

        StackGraphNode *function_node = new StackGraphNode(kind, "", node.editorPosition());
        function_node->parent = stack.back();
        auto function_node_ptr = shared_ptr<StackGraphNode>(function_node);
        stack.back()->children.push_back(function_node_ptr);
        stack.push_back(function_node_ptr);

        auto declarator_node = node.childByFieldName("declarator");
        _Context ctx2;
        ctx2.state = "function_definition";

        build_stack_graph(stack, code, std::move(*declarator_node), ctx2);

        ctx2.state = "skip_compound";
        auto body_node = node.childByFieldName("body");
        build_stack_graph(stack, code, std::move(*body_node), ctx2);

        stack.pop_back();
    }
    else if (strcmp(node.type(), "preproc_include") == 0)
    {

        auto include_node = node.childByFieldName("path");

        auto text = include_node->text(code);
        text = text.substr(1, text.size() - 2);

        auto n = shared_ptr<StackGraphNode>(new StackGraphNode(StackGraphNodeKind::IMPORT, text, node.editorPosition()));

        n->parent = stack.back();
        stack.back()->children.push_back(n);
    }
    else if (strcmp(node.type(), "identifier") == 0 && ctx.state == "reference")
    {
        ctx.type = node.text(code);
    }
    else if (strcmp(node.type(), "call_expression") == 0 && ctx.state == "reference")
    {
        auto val = node.childByFieldName("function");
        build_stack_graph(stack, code, std::move(*val), ctx);
    }
    else if (strcmp(node.type(), "pointer_expression") == 0 && ctx.state == "reference")
    {
        auto val = node.childByFieldName("argument");
        build_stack_graph(stack, code, std::move(*val), ctx);
    }
    else if (strcmp(node.type(), "subscript_expression") == 0 && ctx.state == "reference")
    {
        auto val = node.childByFieldName("argument");
        build_stack_graph(stack, code, std::move(*val), ctx);
    }
    else if (strcmp(node.type(), "field_expression") == 0 && ctx.state == "reference")
    {
        auto val = node.childByFieldName("argument");
        build_stack_graph(stack, code, std::move(*val), ctx);
        auto val2 = node.childByFieldName("field");
        ctx.type = ctx.type + "." + val2->text(code);
    }
    else if (strcmp(node.type(), "identifier") == 0 || strcmp(node.type(), "call_expression") == 0 || strcmp(node.type(), "field_expression") == 0 || strcmp(node.type(), "pointer_expression") == 0 || strcmp(node.type(), "subscript_expression") == 0)
    {
        _Context ctx2;
        ctx2.state = "reference";
        ctx2.type = "";

        TSNodeWrapper node_cpy(node);
        build_stack_graph(stack, code, node_cpy, ctx2);
        auto ref_text = ctx2.type;

        auto ref_node = new StackGraphNode(StackGraphNodeKind::REFERENCE, ref_text, node.editorPosition());
        ref_node->parent = stack.back();
        auto ref_node_ptr = shared_ptr<StackGraphNode>(ref_node);

        stack.back()->children.push_back(ref_node_ptr);
    }
    else
    {
        for (uint32_t i = 0; i < node.child_count(); i++)
        {
            build_stack_graph(stack, code, std::move(*node.child(i)), ctx);
        }
    }
}

shared_ptr<StackGraphNode> stack_graph::build_stack_graph_tree(TSNode root, const char *source_code)
{

    vector<shared_ptr<StackGraphNode>> stack;
    string code = source_code;
    _Context context;
    build_stack_graph(stack, code, root, context);
    return stack.size() == 0 ? nullptr : stack.back();
}