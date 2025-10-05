/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2020,2021 SiXDP2 Inc.
 *
 * Authors: Felipe Magno de Almeida <felipe@expertise.dev>
 *          João Paulo Taylor Ienczak Zanette <joao.tiz@expertise.dev>
 *          Lucas Cavalcante de Sousa <lucas@expertise.dev>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef XDP2GEN_PYTHON_GENERATORS_H
#define XDP2GEN_PYTHON_GENERATORS_H

#include <vector>

#include <Python.h>

#include <xdp2gen/clang-ast/metadata_spec.h>
#include <xdp2gen/program-options/log_handler.h>

extern const char *pyratempsrc;
extern const char *template_gen;
extern const char *common_parser_template_str;
extern const char *c_def_template_str;
extern const char* p4_def_template_str;
extern const char *xdp_def_template_str;

namespace xdp2gen::python
{

template <typename T>
auto ensure_not_null(T *t, std::string const &msg)
{
    if (t == NULL) {
        throw std::runtime_error(msg);
    }

    return t;
}

void decref(PyObject *obj)
{
    Py_DECREF(obj);
}

using python_object_deleter_t = std::function<decltype(decref)>;
using python_object_t = std::unique_ptr<PyObject, python_object_deleter_t>;

auto make_python_object(PyObject *obj)
{
    return python_object_t{ obj, decref };
}

auto make_python_object(int value)
{
    return make_python_object(PyLong_FromLong(value));
}

auto make_python_object(long value)
{
    return make_python_object(PyLong_FromLong(value));
}

auto make_python_object(bool value)
{
    return make_python_object(PyBool_FromLong(static_cast<long>(value)));
}

auto make_python_object(char const *str)
{
    return make_python_object(PyUnicode_FromString(str));
}

auto make_python_object(std::string str)
{
    return make_python_object(str.c_str());
}

template <typename T>
void push_back_python_object(std::vector<python_object_t> &v, T value)
{
    v.push_back(make_python_object(value));
}

void push_back_python_object(std::vector<python_object_t> &v,
                             python_object_t value)
{
    v.push_back(std::move(value));
}

template <typename... T>
auto make_python_object_vector(T... raw_values)
{
    auto v = std::vector<python_object_t>{};
    ((push_back_python_object(v, std::forward<T>(raw_values))), ...);
    return v;
}

/**
 * Wrapper for python's tuple data type.
 */
struct tuple {
    template <typename... T>
    tuple(T... raw_values)
    {
        auto values = make_python_object_vector(std::forward<T>(raw_values)...);
        auto length = values.size();
        auto py_tuple = PyTuple_New(length);
        auto i = 0;
        for (auto &&py_value : values) {
            PyTuple_SetItem(py_tuple, i, py_value.release());
            ++i;
        }
        tuple_obj = make_python_object(py_tuple);
    }

    tuple(tuple const &) = default;
    tuple(tuple &&) = default;

    auto get() const
    {
        return tuple_obj.get();
    }

    python_object_t tuple_obj;
};

auto make_python_object(tuple &&py_tuple)
{
    return make_python_object(py_tuple.tuple_obj.release());
}

/**
 * Wrapper for python's list data type.
 */
struct list {
    template <typename... T>
    list(T... raw_values)
    {
        auto py_list =
            PyList_New(sizeof...(raw_values), std::forward<T>(raw_values)...);
        list_obj = make_python_object(py_list);
    }

    list(list const &) = default;
    list(list &&) = default;

    auto get() const
    {
        return list_obj.get();
    }

    template <typename Value>
    auto set(ssize_t i, Value v)
    {
        auto py_v = make_python_object(v);
        auto success = static_cast<bool>(
            PyList_SetItem(list_obj.get(), i, py_v.release()));
        return success;
    }

    template <typename Value>
    auto append(Value v)
    {
        auto py_v = make_python_object(std::move(v));
        auto success =
            static_cast<bool>(PyList_Append(list_obj.get(), py_v.release()));
        return success;
    }

    python_object_t list_obj;
};

auto make_python_object(list &&py_list)
{
    return make_python_object(py_list.list_obj.release());
}

/**
 * Wrapper for python's dict data type.
 *
 * For simplicity, it accepts only bools, strings and integers as key.
 */
struct dict {
    dict()
        : py_dict{ make_python_object(PyDict_New()) }
    {
    }

    auto operator[](python_object_t py_key) const
    {
        auto py_value =
            make_python_object(PyDict_GetItem(py_dict.get(), py_key.get()));
        return ensure_not_null(py_value.get(),
                               "Dict object doesn't have the specified key.");
    }

    template <typename V>
    auto set(std::string const &key, V value)
    {
        auto py_key = make_python_object(key);
        auto py_value = make_python_object(std::move(value));
        auto success = static_cast<bool>(
            PyDict_SetItem(py_dict.get(), py_key.get(), py_value.release()));
        return success;
    }

    auto get() const
    {
        return py_dict.get();
    }

    python_object_t py_dict;
};

auto make_python_object(dict py_dict)
{
    return make_python_object(py_dict.py_dict.release());
}

auto make_edge_list(graph_t const &graph, vertex_descriptor_t const &v)
{
    python::list l;
    auto oedges = out_edges(v, graph);
    for (auto &&e : boost::make_iterator_range(oedges.first, oedges.second)) {
        python::dict d;
        d.set("macro_name", graph[e].macro_name);
        d.set("parser_node", graph[e].parser_node);
        d.set("back", graph[e].back);
        d.set("macro_name_value", static_cast<int>(graph[e].macro_name_value));
        d.set("target", graph[target(e, graph)].name);
        l.append(std::move(d));
    }

    return l;
}

template <typename R>
auto make_python_object(graph_t const &graph, std::vector<R> const &roots)
{
    auto list = python::list{};

    for (auto &&r : roots) {
        auto l = python::dict{};
        l.set("parser_name", r.parser_name);
        l.set("node_name", graph[r.root].name);
        l.set("parser_add", r.dummy);
        l.set("parser_ext", r.ext);
        l.set("max_nodes", r.max_nodes);
	l.set("max_frames", r.max_frames);
        l.set("max_encaps", r.max_encaps);
        l.set("metameta_size", r.metameta_size);
	l.set("frame_size", r.frame_size);
	l.set("num_counters", r.num_counters);
	l.set("num_keys", r.num_keys);
	l.set("okay_node", r.okay_target_set ?
	      graph[r.okay_target].name : "");
	l.set("fail_node", r.fail_target_set ?
	      graph[r.fail_target].name : "");
	l.set("atencap_node", r.encap_target_set ?
	      graph[r.encap_target].name : "");

        list.append(std::move(l));
    }

    return list;
}

/**
 * Creates a Python Object for a graph vertex.
 */
auto make_python_object(graph_t const &graph, vertex_descriptor_t const &vertex)
{
    auto obj = dict{};

    python::list tlv_nodes;

    for (auto &&t : graph[vertex].tlv_nodes) {
        python::dict tlv;
        tlv.set("name", t.name);
        tlv.set("string_name", t.string_name);
        tlv.set("metadata", t.metadata);
        tlv.set("handler", t.handler);
        tlv.set("type", t.type);
        tlv.set("overlay_table", t.overlay_table);
        tlv.set("unknown_overlay_ret", t.unknown_overlay_ret);
        tlv.set("wildcard_node", t.wildcard_node);
        tlv.set("check_length", t.check_length);
        {
            python::list overlay_nodes;
            for (auto &&overlay : t.tlv_nodes) {
                plog::log(std::cout) << "overlay is not empty, adding "
                                     << overlay.name << std::endl;
                python::dict tlv_overlay;
                tlv_overlay.set("name", overlay.name);
                tlv_overlay.set("string_name", overlay.string_name);
                tlv_overlay.set("metadata", overlay.metadata);
                tlv_overlay.set("handler", overlay.handler);
                tlv_overlay.set("type", overlay.type);
                tlv_overlay.set("unknown_overlay_ret",
                                overlay.unknown_overlay_ret);
                tlv_overlay.set("wildcard_node", overlay.wildcard_node);
                overlay_nodes.append(std::move(tlv_overlay));
            }
            tlv.set("overlay_nodes", std::move(overlay_nodes));
        }
        tlv_nodes.append(std::move(tlv));
    }

    python::list flag_fields_nodes;

    for (auto &&f : graph[vertex].flag_fields_nodes) {
        python::dict flag;
        flag.set("name", f.name);
        flag.set("string_name", f.string_name);
        flag.set("metadata", f.metadata);
        flag.set("handler", f.handler);
        flag.set("index", f.index);
        flag_fields_nodes.append(std::move(flag));
    }

    auto &v = graph[vertex];
    obj.set("name", v.name);
    obj.set("parser_node", v.parser_node);
    obj.set("metadata", v.metadata);
    obj.set("handler", v.handler);
    obj.set("table", v.table);
    obj.set("tlv_table", v.table);
    obj.set("flag_fields_table", v.flag_fields_table);
    obj.set("unknown_proto_ret", v.unknown_proto_ret);
    obj.set("wildcard_proto_node", v.wildcard_proto_node);
    obj.set("proto_min_len",
            static_cast<int>(v.proto_min_len ? *v.proto_min_len : 0));
    python::dict next_proto_info;
    if (v.next_proto_data) {
        next_proto_info.set("bit_offset",
                            static_cast<int>(v.next_proto_data->bit_offset));
        next_proto_info.set("bit_size",
                            static_cast<int>(v.next_proto_data->bit_size));
        next_proto_info.set("bit_mask",
                            static_cast<int>(v.next_proto_data->bit_mask));
        next_proto_info.set("multiplier",
                            static_cast<int>(v.next_proto_data->multiplier));
    }
    else {
	  next_proto_info.set("bit_offset", 0);
	  next_proto_info.set("bit_size", 0);
	  next_proto_info.set("bit_mask", 0);
	  next_proto_info.set("multiplier", 0);
    }
    python::list metadata_transfers;
    for (auto &&m : v.metadata_transfers) {
        python::dict transfer;
        if (auto p =
                std::get_if<xdp2gen::llvm::metadata_transfer>(&m.transfer)) {
            transfer.set("dst_off", static_cast<int>(p->dst_bit_offset));
            transfer.set("name", m.name);
            transfer.set("src_off", static_cast<int>(p->src_bit_offset));
            transfer.set("length", static_cast<int>(p->bit_size));
        } else if (auto p =
                       std::get_if<xdp2gen::llvm::metadata_write_constant>(
                           &m.transfer)) {
            transfer.set("value", static_cast<int>(p->value));
            transfer.set("name", m.name);
            transfer.set("dst_off", static_cast<int>(p->dst_bit_offset));
            transfer.set("length", static_cast<int>(p->bit_size));
        }
        metadata_transfers.append(std::move(transfer));
    }
    obj.set("metadata_transfers", std::move(metadata_transfers));
    obj.set("next_proto_info", std::move(next_proto_info));
    obj.set("tlv_nodes", std::move(tlv_nodes));
    obj.set("flag_fields_nodes", std::move(flag_fields_nodes));
    obj.set("out_edges", make_edge_list(graph, vertex));

    return obj;
}

python::dict make_python_object(
    std::variant<clang_ast::metadata_integer, clang_ast::metadata_record>
        record);
python::dict make_python_object(
    std::variant<clang_ast::metadata_integer, clang_ast::metadata_record,
                 clang_ast::metadata_array>
        v);

python::dict make_python_object(clang_ast::metadata_integer integer)
{
    python::dict obj;
    obj.set("size", static_cast<int>(integer.bit_size));
    return obj;
}
python::dict make_python_object(clang_ast::metadata_array array)
{
    python::dict obj;
    obj.set("size", static_cast<int>(array.size));
    obj.set("type", make_python_object(array.type));
    return obj;
}
python::dict make_python_object(clang_ast::metadata_record record)
{
    python::dict obj;

    obj.set("is_union", record.is_union);
    python::list fields;
    for (auto &&f : record.fields) {
        auto field = make_python_object(f.type);
        field.set("name", f.name);
        fields.append(std::move(field));
    }
    obj.set("fields", std::move(fields));
    return obj;
}
python::dict make_python_object(
    std::variant<clang_ast::metadata_integer, clang_ast::metadata_record> v)
{
    return std::visit([](auto &&o) { return make_python_object(o); }, v);
}
python::dict make_python_object(
    std::variant<clang_ast::metadata_integer, clang_ast::metadata_record,
                 clang_ast::metadata_array>
        v)
{
    return std::visit([](auto &&o) { return make_python_object(o); }, v);
}

/**
 * Creates a Python Object for a graph.
 *
 * Object is represented as a dictionary of vertex names to their data and
 * edges. For a single vertex, its edges are the names of the adjacent
 * vertices.
 */
auto make_python_object(graph_t const &graph)
{
    auto obj = dict{};

    for (auto &&v_descriptor : boost::make_iterator_range(vertices(graph))) {
        auto &v = graph[v_descriptor];

        obj.set(v.name, make_python_object(graph, v_descriptor));
    }

    return obj;
}

struct module {
    auto get_function(std::string const &name) const
    {
        return make_python_object(ensure_not_null(
            PyObject_GetAttrString(py_module.get(), name.c_str()),
            std::string{ "Failed to get '" } + name + "' from module"));
    }

    auto get() const
    {
        return py_module.get();
    }

    python_object_t py_module;
};

auto import(std::string const &name)
{
    return module{ make_python_object(
        ensure_not_null(PyImport_ImportModule("template_gen"),
                        "Failed to import module 'template_gen'")) };
}

template <typename... T>
auto call_function(python_object_t const &function, T... raw_args)
{
    auto args = tuple(std::forward<T>(raw_args)...);
    auto call_result = PyObject_CallObject(function.get(), args.get());
    return ensure_not_null(call_result, "Failed to call function");
}

auto decode_locale(char const *str, size_t *size)
{
    static auto ptr = [](auto *p) { PyMem_RawFree(p); };
    return std::unique_ptr<wchar_t[], decltype(ptr)>(Py_DecodeLocale(str, size),
                                                     ptr);
}

struct error_checker {
    ~error_checker()
    {
        if (PyErr_Occurred()) {
            PyErr_Print();
        }
    }
};

void show_py_exception()
{
    if (PyErr_Occurred()) {
        PyErr_Print();
    }
}

/* Generate a plain C optimized parser based on the common template and the
 * C template
 */
int generate_root_parser_c(std::string filename, std::string output,
                           graph_t graph, std::vector<parser<graph_t>> roots,
                           clang_ast::metadata_record record)
{
    {
        auto ptr = [](auto *p) { PyMem_RawFree(p); };
        auto program_name = decode_locale("main.py", NULL);
        auto template_str = std::string(common_parser_template_str) +
                            std::string(c_def_template_str);

//       Py_SetProgramName(program_name.get());
        PyStatus status;
        PyConfig config;
        PyConfig_InitPythonConfig(&config);

        status = PyConfig_SetString(&config, &config.program_name,
				    program_name.get());
        if (PyStatus_Exception(status)) {
            plog::log(std::cerr)
                << "Error running generation template" << std::endl;
	    return 120;
        }

        status = Py_InitializeFromConfig(&config);
        if (PyStatus_Exception(status)) {
            plog::log(std::cerr)
                << "Error running generation template" << std::endl;
	    return 120;
        }

        Py_Initialize();

        auto checker = error_checker{};

        PyRun_SimpleString(pyratempsrc);
        PyRun_SimpleString(template_gen);

        auto generate_parser_entry_function =
            make_python_object(ensure_not_null(
                PyObject_GetAttrString(PyImport_AddModule("__main__"),
                                       "generate_parser_function"),
                std::string{ "Failed to get 'generate_parser_function'" }));

        {
            auto py_graph = make_python_object(graph);
            auto py_roots = make_python_object(graph, roots);
            auto py_metadata_record = make_python_object(record);

            call_function(generate_parser_entry_function, filename, output,
                          py_graph.get(), py_roots.get(),
                          py_metadata_record.get(), template_str.c_str());
        }
    }

    if (Py_FinalizeEx() < 0) {
        plog::log(std::cerr)
            << "Error running generation template" << std::endl;
        return 120;
    }

    return 0;
}

int generate_root_parser_xdp_c(std::string filename, std::string output,
                               graph_t graph,
                               std::vector<parser<graph_t>> roots,
                               clang_ast::metadata_record record)
{
    {
        auto ptr = [](auto *p) { PyMem_RawFree(p); };
        auto program_name = decode_locale("main.py", NULL);
        auto template_str = std::string(common_parser_template_str) +
                            std::string(xdp_def_template_str);

//        Py_SetProgramName(program_name.get());
        PyStatus status;
        PyConfig config;
        PyConfig_InitPythonConfig(&config);

        status = PyConfig_SetString(&config, &config.program_name,
				    program_name.get());
        if (PyStatus_Exception(status)) {
            plog::log(std::cerr)
                << "Error running generation template" << std::endl;
	    return 120;
        }

        status = Py_InitializeFromConfig(&config);
        if (PyStatus_Exception(status)) {
            plog::log(std::cerr)
                << "Error running generation template" << std::endl;
	    return 120;
        }

        Py_Initialize();

        auto checker = error_checker{};

        PyRun_SimpleString(pyratempsrc);
        PyRun_SimpleString(template_gen);

        auto generate_parser_entry_function =
            make_python_object(ensure_not_null(
                PyObject_GetAttrString(PyImport_AddModule("__main__"),
                                       "generate_parser_function"),
                std::string{ "Failed to get 'generate_parser_function'" }));

        {
            auto py_graph = make_python_object(graph);
            auto py_roots = make_python_object(graph, roots);
            auto py_metadata_record = make_python_object(record);

            call_function(generate_parser_entry_function, filename, output,
                          py_graph.get(), py_roots.get(),
                          py_metadata_record.get(), template_str.c_str());
        }
    }

    if (Py_FinalizeEx() < 0) {
        plog::log(std::cerr)
            << "Error running generation template" << std::endl;
        return 120;
    }

    return 0;
}

int generate_root_parser_p4(std::string filename,
						   std::string output,
						   graph_t graph,
                            std::vector<parser<graph_t>> roots
			    , clang_ast::metadata_record record)
{
	{
		auto ptr = [](auto* p) { PyMem_RawFree(p); };
		auto program_name = decode_locale("main.py", NULL);
		auto template_str = std::string(p4_def_template_str);

		Py_SetProgramName(program_name.get());
		Py_Initialize();

		auto checker = error_checker{};

		PyRun_SimpleString(pyratempsrc);
		PyRun_SimpleString(template_gen);

		auto generate_parser_entry_function = make_python_object(
        ensure_not_null(
           PyObject_GetAttrString(PyImport_AddModule("__main__"), "generate_parser_function"),
           std::string{"Failed to get 'generate_parser_function'"}
          ));

		{
			auto py_graph = make_python_object(graph);
			auto py_roots = make_python_object(graph, roots);
			auto py_metadata_record = make_python_object(record);

			call_function(
						  generate_parser_entry_function,
						  filename,
						  output,
						  py_graph.get(),
						  py_roots.get(),
						  py_metadata_record.get(),
						  template_str.c_str()
						  );
		}
	}

	if (Py_FinalizeEx() < 0) {
		std::cerr << "Error running generation template" << std::endl;
		return 120;
	}

	return 0;
}

}

#endif
