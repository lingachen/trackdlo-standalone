# C++ Part of Trackdlo

Under this folder is the tracking part, which is also the main part, of trackdlo, and they are implemented with c++.

There is a python package for the class `trackdloApp` built with pybind11.

## Build

1. Before building the package, those packages need to be installed first:
    
    ```bash
    sudo apt-get update
    sudo apt-get install cmake pybind11-dev libyaml-cpp-dev libspdlog-dev libeigen3-dev libpcl-dev libopencv-dev
    ```

2. Create folder for cmake

    ```bash
    cd <repo_root>/trackdlo-c++
    mkdir build && cd build
    ```

3. Start building 

    ```bash
    cmake .. && make
    ```

If everything goes well, there is a `trackdlo_app.cpython-38-aarch64-linux-gnu.so` under the build folder.
You could import this file in python if this file is at the same directory of your python code or is added to `sys.path`.

## How to use

Following is an example to import the class

```python
impot sys
sys.path.append("/path/to/your/module")

from trackdlo_app import trackdloApp
app = trackdloApp("/path/to/config.yaml", False) # The meaning of parameters will be shown below.
```

## Function APIs

### constructor

- `trackdloApp(<config_path>, <multi_color_dlo>)`
    * `config_path` is the absolute path to the config file, which is a .yaml file. Details refer to [`default.yaml`](./config/default.yaml)
    * `multi_color_dlo` is a boolean to decide the process about hsv filtering. If `multi_color_dlo` is true, there are multiple fixed hsv filters applied to the input image. If `multi_color_dlo` is false, a hsv filter with the number in config file is applied to the input image.

### Input necessary components

- `bool update_init_nodes(<init_nodes>)`: input the result from the initialization part of trackdlo, which is implemented with python.
    * `init_nodes` should be a N*3 double-typed `numpy.array` containing the 3D coordinate of nodes.
    * After the nodes are successfully initialized, the variable will be locked to prevent changing. Call `reset()` to unlock.

- `bool update_camera_info(<camera_proj_matrix>)`: input the transformation matrix of the camera, which is part of the `camera_info` message type in ROS package.
    * `camera_proj_matrix` should be a 3*4 `numpy.array`.
    * After the parameters are set, the variable will be locked to prevent changing. Call `reset()` to unlock.

- `void reset()`: the function to unlock the variables to enable updating. You need to call this function everytime before updating the variables.

### Main execution and Get results

- `bool execute(<rgb_image>, <depth_image>)`: the main execution function of the trackdlo tracking.
    * `rgb_image` is a `numpy.array` from `OpenCV.Mat`. It should be a `CV_8UC3` image.
    * `depth_image` is a `numpy.array` from `OpenCV.Mat`. It should be a `CV_16UC1` image.

- `numpy.array get_result_image()`: is the function to get the result after `execute` is successfully called.
    * There is a `numpy.array` returned by this function. Function will return None, if there is no result yet.

