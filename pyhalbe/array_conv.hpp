/*
 * Wrap std::array to and from Python sequences
 *
 */

#pragma once

#include <iostream>
#include <boost/python.hpp>

namespace bp = boost::python;

template<typename T>
struct XArray_to_PyList {
	static PyObject* convert(T const& a)
	{
		//std::cout << "array_to_py_sequence<" << typestring<T>() << ">::convert()" << std::endl;
		auto* l = new boost::python::list;
		for (auto e : a) {
			l->append(e);
		}
		return boost::python::incref(boost::python::object(*l).ptr());
	}
};

template <typename T>
struct XArray_from_PyObject
{
	static void reg_impl() {}
};


template <template <typename, std::size_t> class Array, typename T, std::size_t size>
struct XArray_from_PyObject< Array<T, size> >
{
	typedef Array<T, size> array_type;

	static void* convertible(PyObject* py_obj) {
		//std::cout << "array_from_py_sequence<" << typestring<T>() << ">::convertible()" << std::endl;
		//if(py_obj) {
		//	std::cout << "PyObject: ";
		//	PyObject_Print(py_obj, stdout, Py_PRINT_RAW);
		//	std::cout << std::endl;
		//}

		if (!PySequence_Check(py_obj))
			return nullptr;

		if (!PyObject_HasAttrString(py_obj, "__len__"))
			return nullptr;

		bp::object py_sequence(bp::handle<>(bp::borrowed(py_obj)));

		if (size != static_cast<size_t>(len(py_sequence)))
			return nullptr;

		for (size_t i = 0; i < size; i++) {
			bp::object element = py_sequence[i];
			bp::extract<T> type_checker(element);
			if (!type_checker.check())
				return nullptr;
		}
		return py_obj;
	}

	static void construct(
		PyObject* py_obj,
		boost::python::converter::rvalue_from_python_stage1_data* data
	) {
		//std::cout << "array_from_py_sequence<" << typestring<T>() << ">::construct()" << std::endl;

		typedef boost::python::converter::rvalue_from_python_storage<array_type> storage_t;
		storage_t* the_storage = reinterpret_cast<storage_t*>(data);
		void* memory_chunk = the_storage->storage.bytes;

		array_type * array = new (memory_chunk) array_type();
		data->convertible = memory_chunk;

		bp::object py_sequence(bp::handle<>(bp::borrowed(py_obj)));
		for (size_t i = 0; i < size; i++) {
			bp::object element = py_sequence[i];
			(*array)[i] = bp::extract<T>(py_sequence[i]);
		}
	}

	static void reg() {
		//throw "Fuck up";
		reg_impl();
	}

	// configure-once flag
	static bool configured;

	static void reg_impl() {
		XArray_from_PyObject<T>::reg_impl();

		if (!configured) {
			bp::converter::registry::push_back(&convertible, &construct, bp::type_id<array_type>());
			bp::to_python_converter<array_type, XArray_to_PyList<array_type> >();
			configured = true;
		}
	}
};

template <template <typename, std::size_t> class Array, typename T, std::size_t size>
bool XArray_from_PyObject< Array<T, size> >::configured = false;
