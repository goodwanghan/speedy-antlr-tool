
#pragma once

#include "{{grammar_name}}BaseVisitor.h"
#include "speedy_antlr.h"

class SA_{{grammar_name}}Translator : public {{grammar_name}}BaseVisitor {
    speedy_antlr::Translator *translator;

    public:
    SA_{{grammar_name}}Translator(speedy_antlr::Translator *translator);
    ~SA_{{grammar_name}}Translator();

{%- for d in context_data if not d.is_label_parent %}
    antlrcpp::Any visit{{d.Rule_name}}({{grammar_name}}Parser::{{d.Rule_name}}Context *ctx);
{% endfor %}
};
