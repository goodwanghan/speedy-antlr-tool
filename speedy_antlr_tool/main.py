import os

import jinja2 as jj

from .extractor import extract

def write_cpp_files(grammar_name:str, context_data:str, output_dir:str):
    loader = jj.FileSystemLoader(os.path.join(os.path.dirname(__file__), "templates"))

    jj_env = jj.Environment(
        loader=loader,
        undefined=jj.StrictUndefined
    )

    context = {
        "grammar_name": grammar_name,
        "context_data": context_data,
    }

    # Write out main module source
    template = jj_env.get_template("sa_X_cpp_parser.cpp")
    stream = template.stream(context)
    output_path = os.path.join(output_dir, "sa_%s_cpp_parser.cpp" % grammar_name.lower())
    stream.dump(output_path)

    # Write out translator visitor header
    template = jj_env.get_template("sa_X_translator.h")
    stream = template.stream(context)
    output_path = os.path.join(output_dir, "sa_%s_translator.h" % grammar_name.lower())
    stream.dump(output_path)

    # Write out translator visitor source
    template = jj_env.get_template("sa_X_translator.cpp")
    stream = template.stream(context)
    output_path = os.path.join(output_dir, "sa_%s_translator.cpp" % grammar_name.lower())
    stream.dump(output_path)

    # Write out support lib header
    template = jj_env.get_template("speedy_antlr.h")
    stream = template.stream(context)
    output_path = os.path.join(output_dir, "speedy_antlr.h")
    stream.dump(output_path)

    # Write out support lib source
    template = jj_env.get_template("speedy_antlr.cpp")
    stream = template.stream(context)
    output_path = os.path.join(output_dir, "speedy_antlr.cpp")
    stream.dump(output_path)


def write_py_files(grammar_name:str, context_data:str, output_dir:str):
    loader = jj.FileSystemLoader(os.path.join(os.path.dirname(__file__), "templates"))

    jj_env = jj.Environment(
        loader=loader,
        undefined=jj.StrictUndefined
    )

    context = {
        "grammar_name": grammar_name,
        "context_data": context_data,
    }

    # Write out python file
    template = jj_env.get_template("sa_X.pyt")
    stream = template.stream(context)
    output_path = os.path.join(output_dir, "sa_%s.py" % grammar_name.lower())
    stream.dump(output_path)


def generate(py_parser_path:str, cpp_output_dir:str):
    if not os.path.exists(py_parser_path):
        raise ValueError("File does not exist: %s" % py_parser_path)
    parser_name = os.path.splitext(os.path.basename(py_parser_path))[0]
    if not parser_name.endswith('Parser'):
        raise ValueError("File does not look like a parser: %s" % py_parser_path)

    grammar_name = parser_name[:-6]

    py_output_dir = os.path.dirname(py_parser_path)

    # Parse the Parser.py file and extract context data
    context_data = extract(py_parser_path)

    # Write out files
    write_py_files(grammar_name, context_data, py_output_dir)
    write_cpp_files(grammar_name, context_data, cpp_output_dir)
