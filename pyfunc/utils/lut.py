import numpy as np
from typing import Union
from scipy.interpolate import LinearNDInterpolator

import xml.etree.ElementTree as ET


def lut_from_file(xml_file: str):
    tree = ET.parse(xml_file)
    root = tree.getroot()
    return root
    

def lut_from_text(xml_str: str):
    root = ET.fromstring(xml_str)
    return root


class LookUpTable:
    def __init__(self, xml_file: str=None):
        self.n_dim = 0
        self.vectors = {}
        self.values = {}
        self.interp_funcs = {}
        self.x_names = []
        if xml_file:
            self.parse(xml_file)
            self.build()

    def parse(self, xml_file: str=None, xml_str: str=None):
        if xml_file:
            xml_root = lut_from_file(xml_file)
        elif xml_str:
            xml_root = lut_from_text(xml_str)
        else:
            print('[ERROR] please give `xml_file=` or `xml_str=` as arguments.')
            return
        
        for association in xml_root.iter('association'):
            design = association.find('design')
            extracted = association.find('extracted')
            for val in design:
                if val.attrib['type'].lower() in ['single', 'double', 'float', 'real']:
                    self.vectors.setdefault(val.attrib['name'], []).append(float(val.text))
            for val in extracted.iter('value'):
                if val.attrib['type'].lower() in ['single', 'double', 'float', 'real']:
                    self.values.setdefault(val.attrib['name'], []).append(float(val.text))
        for val in self.values.keys():
            self.interp_funcs.setdefault(val, lambda *args, **kwargs: np.NaN)
            
    def build(self):
        self.x_names = []
        x_vars = []
        y_vals = []
        for k, v in self.vectors.items():
            self.x_names.append(k)
            x_vars.append(v)
        for k, v in self.values.items():
            interp_func = LinearNDInterpolator(list(zip(*x_vars)), v)
            self.interp_funcs[k] = interp_func

    def factory_x(self, **kwargs):
        if len(kwargs) == 0:
            print('==== Factory Function to Build variable X ====')
            print(' [hint] lut = LookUpTable(xml_file)')
            print(' [hint] x_vars = lut.factory_x(width=1.0e-7, height=2.0e-7, ...)')
            print(' [hint] neff_val = lut.interp("neff", x_vars)')
            print('----------- Available X variables: -----------')
            for x_name in self.x_names:
                print(' @{}:\t min={:.4e}, max={:.4e}'.format(x_name, np.min(self.vectors[x_name]), np.max(self.vectors[x_name])))
            return
        x_vars = []
        for k in self.x_names:
            if k in kwargs:
                x_vars.append(kwargs[k])
            else:
                v = np.min(self.vectors[k])
                print('[WARN] the value of `{}` is not given, default as its min-value = {:.4e}'.format(k, v))
                x_vars.append(v)
        return x_vars

    def interp(self, name: str, x: Union[list, tuple, np.ndarray]):
        if name in self.interp_funcs:
            return self.interp_funcs[name](x)
        else:
            print(f'[ERROR] value key: `{name}` is not in the loaded Look-Up Table.')
            print(f'**** Available names: [', ', '.join(self.values.keys()), ']')
            return None