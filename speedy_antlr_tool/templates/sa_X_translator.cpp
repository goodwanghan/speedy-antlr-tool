
#include "sa_{{grammar_name|lower}}_translator.h"


SA_{{grammar_name}}Translator::SA_{{grammar_name}}Translator(speedy_antlr::Translator *translator) {
    this->translator = translator;
}

SA_{{grammar_name}}Translator::~SA_{{grammar_name}}Translator() {
}

{% for d in context_data if not d.is_label_parent %}
antlrcpp::Any SA_{{grammar_name}}Translator::visit{{d.Rule_name}}({{grammar_name}}Parser::{{d.Rule_name}}Context *ctx){
{%- if d.labels %}
    speedy_antlr::LabelMap labels[] = {
    {%- for label in d.labels %}
        {"{{label}}", static_cast<void*>(ctx->{{label}})}{% if not loop.last %},{% endif %}
    {%- endfor %}
    };
{%- endif %}
    PyObject *py_ctx = translator->convert_ctx(this, ctx, "{{d.ctx_classname}}"
    {%- if d.is_label_ctx %}, "{{d.label_ctx_classname}}"{% else %}, nullptr{% endif %}
    {%- if d.labels %}, labels, {{d.labels|length}}{% endif %});
    return py_ctx;
}
{% endfor %}
