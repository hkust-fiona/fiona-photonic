#ifndef FIONA_PHOTONIC_ENGINE
#define FIONA_PHOTONIC_ENGINE

// Python Development Header
#include <Python.h>
#include <abstract.h>
#include <boolobject.h>
#include <import.h>
#include <listobject.h>
#include <longobject.h>
#include <object.h>
#include <tupleobject.h>
#include <unicodeobject.h>

// Standard Library Header
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <map>
#include <utility>
#include <cstdlib>
#include <dlfcn.h>

#ifdef USE_EIGEN
// Eigen Library Header
#include <Eigen/Dense>
#endif /* USE_EIGEN */

// Photonic Model Register Header
#include "register.h"


extern const PyFileFuncVec pyfilefunc_reg;

typedef std::map<PyFileFunc, PyObject*> PyFuncMap;
PyFuncMap pyfunc_map;


/********** Conversion between EigenMatrix and PointerBuffer **********/
#ifdef USE_EIGEN

template <typename T>
Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> matrix_from_buffer(T *array, size_t rows, size_t cols) {
    return Eigen::Map< Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> >(array, rows, cols);
}

template <typename T>
T* buffer_from_matrix(const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& mat, size_t *rows, size_t *cols) {
    *rows = mat.rows();
    *cols = mat.cols();
    
    T buffer[*rows][*cols];
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>::Map(&buffer[0][0], mat.rows(), mat.cols()) = mat;

    T *array = new T[mat.size()];
    for(auto i = 0; i < *rows; ++i) {
        for(auto j = 0; j < *cols; ++j) {
            auto offset = *cols * i + j;
            array[offset] = buffer[i][j];
        }
    }
    return array;
}

#endif /* USE_EIGEN */

/********** Interfaces between C/C++ and Python **********/
void init_python_env() {
    void* const libpython_handle = dlopen("libpython3.10.so", RTLD_LAZY | RTLD_GLOBAL);
    if(!libpython_handle) {
        std::cout << "[INFO] Failed to load dlpython" << std::endl;
    }
    Py_Initialize();
    PyObject *sys = PyImport_ImportModule("sys");
    PyObject *path = PyObject_GetAttrString(sys, "path");
    PyList_Append(path, PyUnicode_FromString("."));

    auto fiona_photonic_dir = std::getenv("FIONA_PHOTONIC_DIR");
    if(fiona_photonic_dir != nullptr) {
        std::cout << "[INFO] env_var: $FIONA_PHOTONIC_DIR = " << fiona_photonic_dir << std::endl;
        PyList_Append(path, PyUnicode_FromString(fiona_photonic_dir));
    } else {
        std::cout << "\033[33m" << "[WARN] environment variable $FIONA_PHOTONIC_DIR is NOT set." << "\033[0m" << std::endl;
        std::cout << "\033[34m" << "[HINT] export FIONA_PHOTONIC_DIR=$(path/to/fiona-photonic)" << "\033[0m" << std::endl;
    }
    

    for(const auto &reg_pair : pyfilefunc_reg) {
        std::string pyfile_fullpath = std::string("pyfunc.") + reg_pair.first;
        PyObject *pName = PyUnicode_DecodeFSDefault(pyfile_fullpath.c_str());
        PyObject *payload = PyImport_Import(pName);
        if (!payload) {
            PyErr_Print();
            std::cout << "[ERROR] Failed to Load Python Func: " << reg_pair.first << "." << reg_pair.second << std::endl;
            exit(-1);
        }
        PyObject *callable = PyObject_GetAttrString(payload, reg_pair.second.c_str());
        pyfunc_map[std::pair<std::string, std::string>(reg_pair.first, reg_pair.second)] = callable;

        Py_DecRef(pName);
        Py_DecRef(payload);
    }
}

void deinit_python_env() {
    for(const auto& pyfunc : pyfunc_map) {
        Py_DecRef(pyfunc.second);
    }
    Py_Finalize();
}

/********** Eigen-enabled Interfaces **********/
#ifdef USE_EIGEN

template <typename T>
static PyObject *get_matrix_size(Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> mat) {
    PyObject *list = PyTuple_New(2);

    PyTuple_SetItem(list, 0, PyLong_FromLong(mat.rows()));
    PyTuple_SetItem(list, 1, PyLong_FromLong(mat.cols()));

    return list;
}

template <typename T>
static PyObject *matrix_get(const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>& mat) {
    PyObject *list = PyList_New(0);
    for(size_t i = 0; i < mat.rows(); ++i) {
        PyObject *row = PyList_New(0);
        for(size_t j = 0; j < mat.cols(); ++j) {
            if(std::is_integral<T>::value) {
                PyList_Append(row, PyLong_FromLong(mat(i, j)));
            } else {
                PyList_Append(row, PyFloat_FromDouble(mat(i, j)));
            }
        }
        PyList_Append(list, row);
    }
    return list;
}

template <typename T>
static Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> matrix_set(PyObject *obj) {
    size_t rows = PyList_Size(obj);
    size_t cols = PyList_Size(PyList_GetItem(obj, 0));

    T *buffer = new T[rows * cols];
    for(size_t i = 0; i < rows; ++i) {
        PyObject *vec = PyList_GetItem(obj, i);
        for(size_t j = 0; j < cols; ++j) {
            size_t offset = i * cols + j;
            if(std::is_integral<T>::value) {
                buffer[offset] = (T)PyLong_AsLong(PyList_GetItem(vec, j));
            } else {
                buffer[offset] = (T)PyFloat_AsDouble(PyList_GetItem(vec, j));
            }
        }
    }
    return Eigen::Map< Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> >(buffer, rows, cols);
}

template <typename I, typename O>
void array_handle(std::string pyfilename, std::string pyfuncname, 
    Eigen::Matrix<O, Eigen::Dynamic, Eigen::Dynamic>& mat_out,
    const Eigen::Matrix<I, Eigen::Dynamic, Eigen::Dynamic>& mat_in) {

    // match the array and callable
    PyObject *shape_out = get_matrix_size<O>(mat_out);
    PyObject *list_in = matrix_get<I>(mat_in);
    
    PyObject *callable = pyfunc_map[PyFileFunc(std::string(pyfilename), std::string(pyfuncname))];
    if (callable && PyCallable_Check(callable)) {
        PyObject *args = PyTuple_Pack(2, shape_out, list_in);
        PyObject *ret_obj = PyObject_CallObject(callable, args);
        mat_out = matrix_set<O>(ret_obj);
        Py_DecRef(args);
    }

    Py_DecRef(list_in);
    Py_DecRef(shape_out);
}

template <typename I, typename O>
void array_handle(std::string pyfilename, std::string pyfuncname, 
    Eigen::Matrix<O, Eigen::Dynamic, Eigen::Dynamic>& mat_out,
    const Eigen::Matrix<I, Eigen::Dynamic, Eigen::Dynamic>& mat_in1,
    const Eigen::Matrix<I, Eigen::Dynamic, Eigen::Dynamic>& mat_in2) {

    // match the array and callable
    PyObject *shape_out = get_matrix_size<O>(mat_out);
    PyObject *list_in1 = matrix_get<I>(mat_in1);
    PyObject *list_in2 = matrix_get<I>(mat_in2);

    PyObject *callable = pyfunc_map[PyFileFunc(std::string(pyfilename), std::string(pyfuncname))];
    if (callable && PyCallable_Check(callable)) {
        PyObject *args = PyTuple_Pack(3, shape_out, list_in1, list_in2);
        PyObject *ret_obj = PyObject_CallObject(callable, args);
        mat_out = matrix_set<O>(ret_obj);
        Py_DecRef(args);
    }

    Py_DecRef(list_in1);
    Py_DecRef(list_in2);
    Py_DecRef(shape_out);
}

/********** Wrap Helper Function **********/
template <typename I, typename O>
void array_handle(std::string pyfilename, std::string pyfuncname, 
    O** buffer_out, size_t out_rows, size_t out_cols,
    I* buffer_in, size_t in_rows, size_t in_cols) {
    Eigen::Matrix<I, Eigen::Dynamic, Eigen::Dynamic> m_in = matrix_from_buffer<I>(buffer_in, in_rows, in_cols);
    Eigen::Matrix<O, Eigen::Dynamic, Eigen::Dynamic> m_out(out_rows, out_cols);
    array_handle(pyfilename, pyfuncname, m_out, m_in);

    size_t rows, cols;
    *buffer_out = buffer_from_matrix<O>(m_out, &rows, &cols);

    if(rows != out_rows) std::cout << "[WARN] array_handle(): rows of output from python do NOT equal to expected out rows number." << std::endl;
    if(cols != out_cols) std::cout << "[WARN] array_handle(): columns of output from python do NOT equal to expected out columns number." << std::endl;
}

template <typename I, typename O>
void array_handle(std::string pyfilename, std::string pyfuncname, 
    O** buffer_out, size_t out_rows, size_t out_cols,
    I* buffer_in1, size_t in1_rows, size_t in1_cols,
    I* buffer_in2, size_t in2_rows, size_t in2_cols) {
    Eigen::Matrix<I, Eigen::Dynamic, Eigen::Dynamic> m_in1 = matrix_from_buffer<I>(buffer_in1, in1_rows, in1_cols);
    Eigen::Matrix<I, Eigen::Dynamic, Eigen::Dynamic> m_in2 = matrix_from_buffer<I>(buffer_in2, in2_rows, in2_cols);
    Eigen::Matrix<O, Eigen::Dynamic, Eigen::Dynamic> m_out(out_rows, out_cols);
    array_handle(pyfilename, pyfuncname, m_out, m_in1, m_in2);

    size_t rows, cols;
    *buffer_out = buffer_from_matrix<O>(m_out, &rows, &cols);

    if(rows != out_rows) std::cout << "[WARN] array_handle(): rows of output from python do NOT equal to expected out rows number." << std::endl;
    if(cols != out_cols) std::cout << "[WARN] array_handle(): columns of output from python do NOT equal to expected out columns number." << std::endl;
}

#else

/********** Eigen-disabled Interfaces **********/
template <typename T>
static PyObject *matrix_get(const T* mat, size_t rows, size_t cols) {
    PyObject *list = PyList_New(0);
    for(size_t i = 0; i < rows; ++i) {
        PyObject *row = PyList_New(0);
        for(size_t j = 0; j < cols; ++j) {
            size_t offset = i * cols + j;
            if(std::is_integral<T>::value) {
                PyList_Append(row, PyLong_FromLong(mat[offset]));
            } else {
                PyList_Append(row, PyFloat_FromDouble(mat[offset]));
            }
        }
        PyList_Append(list, row);
    }
    return list;
}

template <typename T>
static T* matrix_set(PyObject *obj, size_t &rows, size_t &cols) {
    rows = PyList_Size(obj);
    cols = PyList_Size(PyList_GetItem(obj, 0));

    T *buffer = new T[rows * cols];
    for(size_t i = 0; i < rows; ++i) {
        PyObject *vec = PyList_GetItem(obj, i);
        for(size_t j = 0; j < cols; ++j) {
            size_t offset = i * cols + j;
            if(std::is_integral<T>::value) {
                buffer[offset] = (T)PyLong_AsLong(PyList_GetItem(vec, j));
            } else {
                buffer[offset] = (T)PyFloat_AsDouble(PyList_GetItem(vec, j));
            }
        }
    }
    return buffer;
}

template <typename I, typename O>
void array_handle(std::string pyfilename, std::string pyfuncname, 
    O** buffer_out, size_t out_rows, size_t out_cols,
    I* buffer_in, size_t in_rows, size_t in_cols) {
    PyObject *list_in = matrix_get<I>(buffer_in, in_rows, in_cols);
    PyObject *shape_out = PyTuple_New(2);
    PyTuple_SetItem(shape_out, 0, PyLong_FromLong(out_rows));
    PyTuple_SetItem(shape_out, 1, PyLong_FromLong(out_cols));

    size_t rows, cols;
    PyObject *callable = pyfunc_map[PyFileFunc(std::string(pyfilename), std::string(pyfuncname))];
    if (callable && PyCallable_Check(callable)) {
        PyObject *args = PyTuple_Pack(2, shape_out, list_in);
        PyObject *ret_obj = PyObject_CallObject(callable, args);
        *buffer_out = matrix_set<O>(ret_obj, rows, cols);
        Py_DecRef(args);
    }

    if(rows != out_rows) std::cout << "[WARN] array_handle(): rows of output from python do NOT equal to expected out rows number." << std::endl;
    if(cols != out_cols) std::cout << "[WARN] array_handle(): columns of output from python do NOT equal to expected out columns number." << std::endl;

    Py_DecRef(list_in);
    Py_DecRef(shape_out);
}

template <typename I, typename O>
void array_handle(std::string pyfilename, std::string pyfuncname, 
    O** buffer_out, size_t out_rows, size_t out_cols,
    I* buffer_in1, size_t in1_rows, size_t in1_cols,
    I* buffer_in2, size_t in2_rows, size_t in2_cols) {
    PyObject *list_in1 = matrix_get<I>(buffer_in1, in1_rows, in1_cols);
    PyObject *list_in2 = matrix_get<I>(buffer_in2, in2_rows, in2_cols);
    PyObject *shape_out = PyTuple_New(2);
    PyTuple_SetItem(shape_out, 0, PyLong_FromLong(out_rows));
    PyTuple_SetItem(shape_out, 1, PyLong_FromLong(out_cols));

    size_t rows = 0, cols = 0;
    PyObject *callable = pyfunc_map[PyFileFunc(std::string(pyfilename), std::string(pyfuncname))];
    if (callable && PyCallable_Check(callable)) {
        PyObject *args = PyTuple_Pack(3, shape_out, list_in1, list_in2);
        PyObject *ret_obj = PyObject_CallObject(callable, args);
        if(ret_obj == nullptr) {
            PyErr_Print();
            exit(-1);
        }
        *buffer_out = matrix_set<O>(ret_obj, rows, cols);
        Py_DecRef(args);
    }

    if(rows != out_rows) std::cout << "[WARN] array_handle(): rows of output from python do NOT equal to expected out rows number." << std::endl;
    if(cols != out_cols) std::cout << "[WARN] array_handle(): columns of output from python do NOT equal to expected out columns number." << std::endl;

    Py_DecRef(list_in1);
    Py_DecRef(list_in2);
    Py_DecRef(shape_out);
}

#endif /* USE_EIGEN */



#endif /* FIONA_PHOTONIC_ENGINE */
