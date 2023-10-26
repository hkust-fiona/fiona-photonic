import functools
from typing import Callable, Any


def example_roi_func(monitor_dict: dict, spec_dict: dict):
    # suppose this roi_func for `total_energy`
    # register by: profiler.register('total_energy', example_roi_func)
    previous_state = monitor_dict.get('total_energy', 0.0)
    current_consumption = spec_dict['power'] * monitor_dict['cycle_time']
    return previous_state + current_consumption


'''Usage for Profiler Class

    profiler = Profiler(cycle_time=3)
    profiler.register('total_energy', example_roi_func)

    @profiler(latency=1, power=2)
    def mzi():
        print('[info] executed mzi()')
'''
class Profiler:
    def __init__(self, **kwargs):
        self.monitor = {}       # monitor: buffer all the ROI
        self.roi_funcs = {}     # roi: results of interest
        for k, v in kwargs.items():
            self.monitor[k] = v

    def __call__(self, **specs):
        def profiler_decorator(func):
            @functools.wraps(func)
            def wrapped_function(*args, **kwargs):
                for roi_name, roi_func in self.roi_funcs.items():
                    self.monitor[roi_name] = roi_func(self.monitor, specs)
                return func(*args, **kwargs)
            return wrapped_function
        return profiler_decorator

    def register(self, roi_name: str, roi_func: Callable[[dict, dict], Any]):
        self.roi_funcs[roi_name] = roi_func

    def unregister(self, roi_name: str):
        self.roi_funcs.pop(roi_name, None)

    def report(self, roi_list=None, roi_exclude=None):
        print_func = lambda k, v: print(f' @{k}:\t {v}')
        print('=' * 10, ' Monitor Report ', '=' * 10)
        if roi_list:
            for i in roi_list:
                if i in self.monitor:
                    print_func(i, self.monitor[i])
                else:
                    print(f'[WARN] ROI: `{i}` is not in profiler.monitor')
        elif roi_exclude:
            for i in self.monitor:
                if i in roi_exclude:
                    pass
                else:
                    print_func(i, self.monitor[i])
        else:
            for i in self.monitor:
                print_func(i, self.monitor[i])


profiler = Profiler()