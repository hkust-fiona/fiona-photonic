import numpy as np
from .utils.parser import Parser


def ops_single(shape_out, matrix_in, bit_out=None, bit_in=None):

    print('[Python] Shape_out = ', str(shape_out))
    print('[Python] Matrix_in = ', str(matrix_in))
    
    parser = Parser(shape_out, matrix_in, bit_out=bit_out, bit_in1=bit_in)
    comp_in = parser.get_in()
    comp_out = comp_in.T + 1
    print('[Python] comp_out.shape = ', comp_out.shape)
    print('[Python] comp_out = \n', comp_out)
    retval = parser.set_out(comp_out)
    
    return retval


def ops_dual(shape_out, matrix_in1, matrix_in2, bit_out=None, bit_in1=None, bit_in2=None):

    print('[Python] Shape_out = ', str(shape_out))
    print('[Python] Matrix_in1 = ', str(matrix_in1))
    print('[Python] Matrix_in2 = ', str(matrix_in2))
    
    parser = Parser(shape_out, matrix_in1, matrix_in2, bit_out=bit_out, bit_in1=bit_in1, bit_in2=bit_in2)
    comp_in1, comp_in2 = parser.get_in()
    comp_out = comp_in1.T + comp_in2
    print('[Python] comp_out.shape = ', comp_out.shape)
    print('[Python] comp_out = \n', comp_out)
    retval = parser.set_out(comp_out)
    
    return retval