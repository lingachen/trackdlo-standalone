# Python Part of Trackdlo

Under this folder is the initialization part of the dlo.

The python class `Initializer` is the only object you need to use. Reading image from camera is not including in this class.

## Installation

```bash
pip3 install -r requirements.txt
```

### API

- `Initializer(config_fp, visualized=False)`: is the construction function of the class.
    * `config_path` is the absolute path to the config file, which is a .yaml file. An example could refer to [`default.yaml`](./config/default.yaml). The necessay part for this class is 

    ```yaml
    multi_color_dlo: false
    num_of_nodes: 30
    hsv_filter:
      upper: [179, 255 ,30]
      lower: [0, 0, 0]
    ```

    * `visualized` is a boolean to decide whether display the process of the initialization process.

- `Initializer.update_cam_info(camera_proj_matrix: np.array)`
    * `camera_proj_matrix` should be a 3*4 `numpy.array`.

- `Initializer.execute(rgb_img, depth_img)`: This is the main function od the Initializer.
    * `rgb_img` is a `numpy.nparray` with data type of `uint8_t`.
    * `depth_img` is a `numpy.nparray` with data type of `uint16_t`.
    * The output of this funciton is a N*3 `numpy.nparray`. This is the 3D coordinate of the initialized nodes. If there is any error, the output will be None.
