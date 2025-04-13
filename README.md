# trackdlo-standalone

This repository provides a standalone implementation of the [trackdlo](https://github.com/RMDLO/trackdlo) algorithm without relying on any ROS packages.

## Installation

Please refer to the `README.md` files in both the `trackdlo_c++` and `trackdlo_python` folders for installation instructions.

## Usage

An example of the full pipeline can be found in [`example_pipeline.py`](./example_pipeline.py).

## Determine HSV Filter Range

Setting accurate upper and lower bounds for the HSV filter is crucial for the stability and success of the pipeline. Several utility scripts are provided to help you determine suitable HSV bounds:

1. `save_image.py`: This script captures and saves images from a RealSense camera to the current directory for later color analysis. Run it with:
    ```bash
    python3 utils/save_image.py
    ```

2. `rgb2hsv.py`: A simple utility to visualize the individual H, S, and V channels.
You can move your cursor over the wire or line in the image to get a rough idea of its HSV range. Use it as follows:
    ```bash
    python3 utils/rgb2hsv.py <Image Path>
    ```
    and press `q` to quit.

3. `color_picker.py` This is the main tool for determining the HSV filter bounds.It is adapted from the original [trackdlo repository](https://github.com/RMDLO/trackdlo/blob/master/docs/COLOR_THRESHOLD.md). Run it with:
    ```bash
    python3 utils/color_picker.py <Image Path>
    ```
    and press `q` to quit.