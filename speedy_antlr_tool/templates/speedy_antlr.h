
#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "antlr4-runtime.h"

namespace speedy_antlr {

    struct LabelMap {
        const char *name;
        void *ref;
    };

    // C++ exception for when python throws an error and already has an exception
    // context associated with it.
    // This exception is only used to shortcut out of the accelerator.
    class PythonException: public std::exception {};


    class Translator {
        public:
        // Reference to target-specific parser class
        PyObject *parser_cls;

        // Current caller's InputStream
        PyObject *input_stream;

        // Cached "antlr4" module
        PyObject *pyAntlr = NULL;

        // Cached "antlr4.Token" module
        PyObject *py_token_module = NULL;

        // Cached "antlr4.tree.Tree" module
        PyObject *py_tree_module = NULL;

        Translator(PyObject *parser_cls, PyObject *input_stream);
        ~Translator();

        PyObject* convert_common_token(antlr4::Token *token);

        PyObject* tnode_from_token(PyObject *py_token, PyObject *py_parent_ctx);

        PyObject* convert_ctx(
            antlr4::tree::AbstractParseTreeVisitor *visitor,
            antlr4::ParserRuleContext *ctx,
            const char *ctx_classname,
            const char *label_ctx_classname=nullptr,
            LabelMap labels[]=nullptr, size_t n_labels=0
        );
    };



    class ErrorTranslatorListener : public antlr4::BaseErrorListener {
        Translator *translator;
        PyObject *sa_err_listener;

        public:
        ErrorTranslatorListener(Translator *translator, PyObject *sa_err_listener);
        
        void syntaxError(
            antlr4::Recognizer *recognizer, antlr4::Token *offendingSymbol, size_t line,
            size_t charPositionInLine, const std::string &msg, std::exception_ptr e
        );
    };
}