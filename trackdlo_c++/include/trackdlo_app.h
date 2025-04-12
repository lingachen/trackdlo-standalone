#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <pybind11/stl.h>
#include <yaml-cpp/yaml.h>
#include <stdexcept>

#include "trackdlo.h"
#include "utils.h"

#ifndef TRACKDLO_APP_H
#define TRACKDLO_APP_H

using cv::Mat;
using Eigen::MatrixXd;
using Eigen::RowVectorXd;

class trackdloApp
{
public:
    trackdloApp(const std::string& config_file_path);

    bool update_opencv_mask(pybind11::array_t<uint8_t> mask);
    bool update_init_nodes(const MatrixXd& nodes);
    bool update_camera_info(const MatrixXd& cam_proj_matrix);

    int execute(pybind11::array_t<uint8_t> rgb_img_array, pybind11::array_t<uint16_t> depth_img_array);
    void reset();

    // helper functions to get results
    pybind11::object get_result_image();

private:
    // parameters from input
    std::string config_file_path;

    // parameters from config file
    bool multi_color_dlo;
    double visibility_threshold;
    int dlo_pixel_width;
    double beta, beta_pre_proc;
    double lambda, lambda_pre_proc;
    double alpha;
    double lle_weight;
    double mu;
    int max_iter;
    double tol;
    double k_vis, d_vis;
    double downsample_leaf_size;
    std::vector<int> upper;
    std::vector<int> lower;

    std::string logging_level = "info";

    // class variables
    trackdlo tracker;

    bool updated_opencv_mask = false;
    Mat occlusion_mask;
    bool received_init_nodes = false;
    MatrixXd init_nodes;
    bool received_proj_matrix = false;
    MatrixXd proj_matrix;//(3, 4);

    double pre_proc_total = 0;
    double algo_total = 0;

    bool initialized = false;
    double sigma2;
    std::vector<double> converted_node_coord = {0.0};
    MatrixXd Y;

    // results
    bool has_result = false;
    Mat result_tracking_img;

    
    // functions
    void load_config_files();
    void print_params();
    Mat color_thresholding(Mat cur_image_hsv);
};

PYBIND11_MODULE(trackdlo_app, m) {
    pybind11::class_<trackdloApp>(m, "trackdloApp")
        .def(pybind11::init<const std::string&>())
        .def("update_camera_info", &trackdloApp::update_camera_info)
        .def("update_opencv_mask", &trackdloApp::update_opencv_mask)
        .def("update_init_nodes", &trackdloApp::update_init_nodes)
        .def("execute", &trackdloApp::execute)
        .def("reset", &trackdloApp::reset)
        .def("get_result_image", &trackdloApp::get_result_image)
        ;
}

#endif
