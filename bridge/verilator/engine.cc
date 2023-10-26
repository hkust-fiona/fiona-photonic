#include "engine.h"
#include "register.h"

#define BITS_PER_BYTE 8
#define ELEMENT_SIZE_BITS(arr) (svHigh(arr, 0) - svLow(arr, 0) + 1)

extern const PyFileFuncVec pyfilefunc_reg;

typedef std::map<PyFileFunc, PyObject*> PyFuncMap;
PyFuncMap pyfunc_map;

struct iterator_t {
    int low;
    int high;
    int cur;
};


void init_python_env() {
    Py_Initialize();
    PyObject *sys = PyImport_ImportModule("sys");
    PyObject *path = PyObject_GetAttrString(sys, "path");
    PyList_Append(path, PyUnicode_FromString("."));

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
    // std::cout << "[Info] Python functions import successfully. " << std::endl;
}

void deinit_python_env() {
    for(const auto& pyfunc : pyfunc_map) {
        Py_DecRef(pyfunc.second);
    }
    Py_Finalize();
}


static PyObject *transform_bytes_into_list(uint8_t *ptr, size_t length) {
    PyObject *list = PyList_New(0);
    for (int i = 0; i < length; i++) {
        PyList_Append(list, PyLong_FromLong(*(ptr + i)));
    }
    return list;
}

static uint8_t *transform_list_into_bytes(PyObject *obj, size_t *length) {
    uint8_t *bytes = NULL;
    *length = 0;

    if(PyList_Check(obj)) {
        *length = (size_t)PyList_Size(obj);
        bytes = new uint8_t[*length];
        memset(bytes, 0, *length);
        for(auto i = 0; i < *length; i++) {
            PyObject *item = PyList_GetItem(obj, i);
            bytes[i] = (uint8_t)PyLong_AsLong(item);
        }
    }
    return bytes;
}

static PyObject *get_array_size(svOpenArrayHandle arr) {
    PyObject *list = PyList_New(0);

    int dim = svDimensions(arr);
    assert(dim != 0);

    for (int d = 1; d <= dim; d++) {
        auto length = svHigh(arr, d) - svLow(arr, d) + 1;
        PyList_Append(list, PyLong_FromLong(length));
    }
    
    return list;
}

static PyObject *recursive_get(struct iterator_t *iters, int last_layer, int layer, svOpenArrayHandle arr) {
    assert(layer <= last_layer && layer >= 0);
    struct iterator_t *iter = &iters[layer];
    PyObject *list = PyList_New(0);

    if (layer != last_layer) {
        for (iter->cur = iter->low; iter->cur <= iter->high; iter->cur++) {
            PyList_Append(list, recursive_get(iters, last_layer, layer + 1, arr));
        }
    } else {
        for (iter->cur = iter->low; iter->cur <= iter->high; iter->cur++) {
            uint8_t *ptr = NULL;
            switch (last_layer) {
                case 0:
                    ptr = (uint8_t *)svGetArrElemPtr1(arr, iters[0].cur);
                    break;
                case 1:
                    ptr = (uint8_t *)svGetArrElemPtr2(arr, iters[0].cur, iters[1].cur);
                    break;
                case 2:
                    ptr = (uint8_t *)svGetArrElemPtr3(arr, iters[0].cur, iters[1].cur, iters[2].cur);
                    break;
            }
            if (ptr != NULL) {
                /* The dimension 0 will always be the length of the basic element. */
                PyList_Append(list, transform_bytes_into_list(ptr, (ELEMENT_SIZE_BITS(arr) + BITS_PER_BYTE - 1) / BITS_PER_BYTE));
            }
        }
    }
    return list;
}

static PyObject *pad_list(PyObject *list, int row, int col) { // Number of rows and cols
    // std::cout << "Calling pad list" << std::endl;
    int p_row = PyList_Size(list);
    int p_col = PyList_Size(PyList_GetItem(list, 0));
    int elem_len = PyList_Size(PyList_GetItem(PyList_GetItem(list, 0), 0));
    PyObject *list_flatten = PyList_New(0);
    for(int i = 0; i < p_row; i++) {
      for(int j = 0; j < p_col; j++) {
        PyList_Append(list_flatten, PyList_GetItem(PyList_GetItem(list, i), j));
      }
    }
    // std::cout << p_row << ", " <<  p_col << ", "  << elem_len << ", " << PyList_Size(list_flatten) << std::endl;
    // std::cout << row << ", " <<  col << std::endl;
    assert(p_row >= row && p_col >= col);
    PyObject *new_list = PyList_New(0);
    int i, j, ptr;
    ptr = 0;
    uint32_t zero = 0;
    PyObject *zero_elem = transform_bytes_into_list((uint8_t*)&zero, elem_len);
    for(i = 0; i < row; i++) {
      PyObject *sub_list = PyList_New(0);
      for(j = 0; j < col; j++) {
        PyList_Append(sub_list, PyList_GetItem(list_flatten, ptr++));
      }
      while( j < p_col ) { // Pad the remaining cols to 0
        PyList_Append(sub_list, zero_elem);
        j++;
      }
      PyList_Append(new_list, sub_list);
    }
    PyObject *all_zero_list = PyList_New(0);
    for(int k = 0; k < p_col; k++) {
      PyList_Append(all_zero_list, zero_elem);
    }
    while( i < p_row ) { // Pad the remaining rows to 0
      PyList_Append(new_list, all_zero_list);
      i++;
    }
    // std::cout << "return from pad list" << std::endl;
    int nl_row = PyList_Size(new_list);
    int nl_col = PyList_Size(PyList_GetItem(new_list, 0));
    int nl_elem_len = PyList_Size(PyList_GetItem(PyList_GetItem(new_list, 0), 0));
    // std::cout << nl_row << ", " <<  nl_col << ", "  << nl_elem_len << std::endl;
    return new_list;
}

static void recursive_set(struct iterator_t *iters, int last_layer, int layer, svOpenArrayHandle arr, PyObject *obj) {
    assert(layer <= last_layer && layer >= 0);
    struct iterator_t *iter = &iters[layer];
    
    if (layer != last_layer) {
        for (iter->cur = iter->low; iter->cur <= iter->high; iter->cur++) {
            PyObject *item = PyList_GetItem(obj, iter->cur - iter->low);
            recursive_set(iters, last_layer, layer + 1, arr, item);
        }
    } else {
        for (iter->cur = iter->low; iter->cur <= iter->high; iter->cur++) {
            svBitVecVal *val = NULL;
            size_t *length = new size_t;
            PyObject *item = PyList_GetItem(obj, iter->cur - iter->low);
            switch (last_layer) {
                case 0:
                    val = (svBitVecVal *)transform_list_into_bytes(item, length);
                    svPutBitArrElem1VecVal(arr, val, iters[0].cur);
                    break;
                case 1:
                    val = (svBitVecVal *)transform_list_into_bytes(item, length);
                    svPutBitArrElem2VecVal(arr, val, iters[0].cur, iters[1].cur);
                    break;
                case 2:
                    val = (svBitVecVal *)transform_list_into_bytes(item, length);
                    svPutBitArrElem3VecVal(arr, val, iters[0].cur, iters[1].cur, iters[2].cur);
                    break;
            }
        }
    }
}

void array_handle1(char *pyfilename, char *pyfuncname, svOpenArrayHandle arr_out, svOpenArrayHandle arr_in) {
    // obtain the information of dimensions
    int dim_in = svDimensions(arr_in);
    assert(dim_in != 0);

    int dim_out = svDimensions(arr_out);
    assert(dim_out != 0);

    // configure the bound of the iterators
    struct iterator_t *iters_in = (struct iterator_t *)malloc(sizeof(struct iterator_t) * dim_in);
    memset(iters_in, 0, sizeof(struct iterator_t) * dim_in);
    for (int d = dim_in; d > 0; d--) {
        struct iterator_t *ptr = &iters_in[d - 1];
        ptr->cur = ptr->low = svLow(arr_in, d);
        ptr->high = svHigh(arr_in, d);
    }

    struct iterator_t *iters_out = (struct iterator_t *)malloc(sizeof(struct iterator_t) * dim_out);
    memset(iters_out, 0, sizeof(struct iterator_t) * dim_out);
    for (int d = dim_out; d > 0; d--) {
        struct iterator_t *ptr = &iters_out[d - 1];
        ptr->cur = ptr->low = svLow(arr_out, d);
        ptr->high = svHigh(arr_out, d);
    }

    // match the array and callable
    PyObject *list_in = recursive_get(iters_in, dim_in - 1, 0, arr_in);
    PyObject *list_out = get_array_size(arr_out);

    PyObject *callable = pyfunc_map[PyFileFunc(std::string(pyfilename), std::string(pyfuncname))];
    if (callable && PyCallable_Check(callable)) {
        PyObject *args = PyTuple_Pack(4, list_out, list_in, 
            PyLong_FromLong(ELEMENT_SIZE_BITS(arr_out)), PyLong_FromLong(ELEMENT_SIZE_BITS(arr_in)));
        PyObject *ret_obj = PyObject_CallObject(callable, args);
        recursive_set(iters_out, dim_out - 1, 0, arr_out, ret_obj);
        Py_DecRef(args);
    }

    Py_DecRef(list_in);
    
    free(iters_in);
    free(iters_out);
}

void array_handle2(char *pyfilename, char *pyfuncname, svOpenArrayHandle arr_out, 
        svOpenArrayHandle arr_in1, svOpenArrayHandle arr_in2,
                   svBitVecVal* in1_cols, svBitVecVal* in1_rows, 
                   svBitVecVal* in2_cols, svBitVecVal* in2_rows, 
                   svBitVecVal* out_cols, svBitVecVal* out_rows
                   ) {
    // obtain the information of dimensions
    int dim_in1 = svDimensions(arr_in1);
    assert(dim_in1 != 0);
    int dim_in2 = svDimensions(arr_in2);
    assert(dim_in2 != 0);

    int dim_out = svDimensions(arr_out);
    assert(dim_out != 0);

    // configure the bound of the iterators
    struct iterator_t *iters_in1 = (struct iterator_t *)malloc(sizeof(struct iterator_t) * dim_in1);
    memset(iters_in1, 0, sizeof(struct iterator_t) * dim_in1);
    for (int d = dim_in1; d > 0; d--) {
        struct iterator_t *ptr = &iters_in1[d - 1];
        ptr->cur = ptr->low = svLow(arr_in1, d);
        ptr->high = svHigh(arr_in1, d);
    }
    struct iterator_t *iters_in2 = (struct iterator_t *)malloc(sizeof(struct iterator_t) * dim_in2);
    memset(iters_in2, 0, sizeof(struct iterator_t) * dim_in2);
    for (int d = dim_in2; d > 0; d--) {
        struct iterator_t *ptr = &iters_in2[d - 1];
        ptr->cur = ptr->low = svLow(arr_in2, d);
        ptr->high = svHigh(arr_in2, d);
    }

    struct iterator_t *iters_out = (struct iterator_t *)malloc(sizeof(struct iterator_t) * dim_out);
    memset(iters_out, 0, sizeof(struct iterator_t) * dim_out);
    for (int d = dim_out; d > 0; d--) {
        struct iterator_t *ptr = &iters_out[d - 1];
        ptr->cur = ptr->low = svLow(arr_out, d);
        ptr->high = svHigh(arr_out, d);
    }

    // match the array and callable
    PyObject *list_in1 = recursive_get(iters_in1, dim_in1 - 1, 0, arr_in1);
    PyObject *list_in2 = recursive_get(iters_in2, dim_in2 - 1, 0, arr_in2);
    PyObject *list_out = get_array_size(arr_out);
    
    int in1_rows_pad = *in1_rows;
    int in1_cols_pad = *in1_cols;
    int in2_rows_pad = *in2_rows;
    int in2_cols_pad = *in2_cols;
    int out_rows_pad = *out_rows;
    int out_cols_pad = *out_cols;

    PyObject *list_in1_padded = pad_list(list_in1, in1_rows_pad, in1_cols_pad);
    PyObject *list_in2_padded = pad_list(list_in2, in2_rows_pad, in2_cols_pad);

    PyObject *callable = pyfunc_map[PyFileFunc(std::string(pyfilename), std::string(pyfuncname))];
    if (callable && PyCallable_Check(callable)) {
        PyObject *args = PyTuple_Pack(6, list_out, list_in1_padded, list_in2_padded,
            PyLong_FromLong(ELEMENT_SIZE_BITS(arr_out)),
            PyLong_FromLong(ELEMENT_SIZE_BITS(arr_in1)), PyLong_FromLong(ELEMENT_SIZE_BITS(arr_in2)));
        PyObject *ret_obj = PyObject_CallObject(callable, args);
        // printf("%p\n", ret_obj);
        if(ret_obj == NULL) {
          printf("call failed\n");
          PyErr_Print();
          fflush(stdout);
          exit(-1);
        }
        recursive_set(iters_out, dim_out - 1, 0, arr_out, ret_obj);
        std::cerr << "Set return args" << std::endl;
        Py_DecRef(args);
    }

    Py_DecRef(list_in1);
    Py_DecRef(list_in2);
    Py_DecRef(list_in1_padded);
    Py_DecRef(list_in2_padded);
    
    free(iters_in1);
    free(iters_in2);
    free(iters_out);
}

