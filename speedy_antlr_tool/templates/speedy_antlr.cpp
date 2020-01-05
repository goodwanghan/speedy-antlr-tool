
#include "speedy_antlr.h"

using namespace speedy_antlr;

Translator::Translator(PyObject *parser_cls, PyObject *input_stream) {
    this->parser_cls = parser_cls;
    this->input_stream = input_stream;

    // Cache some things for conveinience
    try {
        pyAntlr = PyImport_ImportModule("antlr4");
        if(!pyAntlr) throw PythonException();

        py_tree_module = PyImport_ImportModule("antlr4.tree.Tree");
        if(!py_tree_module) throw PythonException();

        py_token_module = PyImport_ImportModule("antlr4.Token");
        if(!py_token_module) throw PythonException();
    } catch(PythonException &e) {
        Py_XDECREF(pyAntlr);
        Py_XDECREF(py_token_module);
        Py_XDECREF(py_tree_module);
        throw;
    }
}


Translator::~Translator() {
    Py_XDECREF(pyAntlr);
    Py_XDECREF(py_token_module);
    Py_XDECREF(py_tree_module);
}


PyObject* Translator::convert_common_token(antlr4::Token *token){
    PyObject *py_token = PyObject_CallMethod(
        py_token_module, "CommonToken",
        "(sO)nnnn",
        NULL, input_stream, // source tuple --> (TokenSource, InputStream)
        token->getType(), // type id
        0, // channel
        token->getStartIndex(), // start
        token->getStopIndex() // stop
    );

    if(!py_token) throw PythonException();

    // Assign attributes
    PyObject *py_tokenIndex = PyLong_FromSize_t(token->getTokenIndex());
    PyObject_SetAttrString(py_token, "tokenIndex", py_tokenIndex);
    Py_DECREF(py_tokenIndex);

    PyObject *py_line = PyLong_FromSize_t(token->getLine());
    PyObject_SetAttrString(py_token, "line", py_line);
    Py_DECREF(py_line);

    PyObject *py_column = PyLong_FromSize_t(token->getCharPositionInLine());
    PyObject_SetAttrString(py_token, "column", py_column);
    Py_DECREF(py_column);

    PyObject *py_text = PyUnicode_FromString(token->getText().c_str());
    PyObject_SetAttrString(py_token, "_text", py_text);
    Py_DECREF(py_text);

    return py_token;
}


PyObject* Translator::tnode_from_token(PyObject *py_token, PyObject *py_parent_ctx){
    // Wrap token in TerminalNodeImpl
    PyObject *py_tnode = PyObject_CallMethod(
        py_tree_module, "TerminalNodeImpl",
        "O", py_token
    );
    if(!py_tnode) throw PythonException();

    // Assign attributes
    PyObject_SetAttrString(py_tnode, "parentCtx", py_parent_ctx);
    return py_tnode;
}


PyObject* Translator::convert_ctx(
    antlr4::tree::AbstractParseTreeVisitor *visitor,
    antlr4::ParserRuleContext *ctx,
    const char *ctx_classname,
    const char *label_ctx_classname,
    LabelMap labels[], size_t n_labels
){
    // Create py context class
    PyObject *py_ctx = PyObject_CallMethod(
        parser_cls, ctx_classname,
        "ssn",
        NULL, // parser (Set to None since this is not translated)
        NULL, // parent ctx (gets assigned later)
        ctx->invokingState
    );
    if(!py_ctx) throw PythonException();

    if(label_ctx_classname){
        // This is a labelled context. Wrap it in its actual name
        PyObject *base_ctx = py_ctx;
        py_ctx = PyObject_CallMethod(
            parser_cls, label_ctx_classname,
            "sO",
            NULL, // parser
            base_ctx // ctx
        );
        Py_DECREF(base_ctx);
        if(!py_ctx) throw PythonException();
    }

    PyObject *start = NULL;
    PyObject *stop = NULL;

    // Convert all children
    PyObject *py_children = PyList_New(ctx->children.size());
    for (size_t i=0; i < ctx->children.size(); i++) {
        PyObject *py_child = NULL;
        PyObject *py_label_candidate = NULL;
        void *child_ref = NULL;
        if (antlrcpp::is<antlr4::tree::TerminalNode *>(ctx->children[i])) {
            // Child is a token
            antlr4::tree::TerminalNode *tnode = dynamic_cast<antlr4::tree::TerminalNode *>(ctx->children[i]);

            // Convert the token
            antlr4::Token *token = tnode->getSymbol();
            PyObject *py_token = NULL;
            try {
                py_token = convert_common_token(token);
                py_child = tnode_from_token(py_token, py_ctx);
            } catch(PythonException &e) {
                Py_XDECREF(py_token);
                throw;
            }
            child_ref = static_cast<void*>(token);
            py_label_candidate = py_token;
            Py_INCREF(py_label_candidate);

            // Get start/stop
            if(!start){
                start = py_token;
                Py_INCREF(start);
            }
            if(token->getType() != antlr4::IntStream::EOF) {
                // Always set stop to current token
                stop = py_token;
                Py_INCREF(stop);
            }
            Py_DECREF(py_token);
        } else if (antlrcpp::is<antlr4::ParserRuleContext *>(ctx->children[i])) {
            child_ref = static_cast<void*>(ctx->children[i]);
            py_child = visitor->visit(ctx->children[i]);
            PyObject_SetAttrString(py_child, "parentCtx", py_ctx);
            py_label_candidate = py_child;
            Py_INCREF(py_label_candidate);

            // Get start/stop
            if(i == 0) {
                start = PyObject_GetAttrString(py_child, "start");
            }
            if(i == ctx->children.size() - 1){
                stop = PyObject_GetAttrString(py_child, "stop");
            }
        } else {
            PyErr_SetString(PyExc_RuntimeError, "Unknown child type");
            throw PythonException();
        }

        // Check if child matches one of the labels
        for(size_t j=0; j<n_labels; j++) {
            if(child_ref == labels[j].ref){
                PyObject_SetAttrString(py_ctx, labels[j].name, py_label_candidate);
            }
        }
        Py_DECREF(py_label_candidate);

        // Steals reference to py_child
        PyList_SetItem(py_children, i, py_child);

    }

    // Assign start/stop
    if(start) PyObject_SetAttrString(py_ctx, "start", start);
    Py_XDECREF(start);
    if(stop) PyObject_SetAttrString(py_ctx, "stop", stop);
    Py_XDECREF(stop);
    
    // Assign child list to context
    PyObject_SetAttrString(py_ctx, "children", py_children);
    Py_DECREF(py_children);

    return py_ctx;
}





ErrorTranslatorListener::ErrorTranslatorListener(Translator *translator, PyObject *sa_err_listener) {
    this->translator = translator;
    this->sa_err_listener = sa_err_listener;
}
    
void ErrorTranslatorListener::syntaxError(
    antlr4::Recognizer *recognizer, antlr4::Token *offendingSymbol, size_t line,
    size_t charPositionInLine, const std::string &msg, std::exception_ptr e
) {
    // Get input stream from recognizer
    antlr4::IntStream *input_stream;
    if (antlrcpp::is<antlr4::Lexer *>(recognizer)) {
        antlr4::Lexer *lexer = dynamic_cast<antlr4::Lexer *>(recognizer);
        input_stream = lexer->getInputStream();
    } else if (antlrcpp::is<antlr4::Parser *>(recognizer)) {
        antlr4::Parser *parser = dynamic_cast<antlr4::Parser *>(recognizer);
        input_stream = parser->getInputStream();
    } else {
        PyErr_SetString(PyExc_RuntimeError, "Unknown recognizer type");
        throw PythonException();
    }

    size_t char_index = input_stream->index();

    PyObject *py_token;
    if(offendingSymbol){
        py_token = translator->convert_common_token(offendingSymbol);
    } else {
        py_token = Py_None;
        Py_INCREF(py_token);
    }

    PyObject *ret = PyObject_CallMethod(
        sa_err_listener, "syntaxError",
        "OOnnns",
        translator->input_stream, // input_stream
        py_token, // offendingSymbol
        char_index, // char_index
        line, // line
        charPositionInLine, // column
        msg.c_str()// msg
    );
    Py_DECREF(py_token);
    if(!ret) throw PythonException();
    Py_DECREF(ret);
}
