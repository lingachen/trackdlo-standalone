#include "trackdlo_app.h"

trackdloApp::trackdloApp(const std::string& config_file_path){
    this->config_file_path = config_file_path;
    this->multi_color_dlo = multi_color_dlo;

    load_config_files();
    print_params();
}

pybind11::object trackdloApp::get_result_image(){
    if (!has_result){
        return pybind11::none();
    }

    auto rows = result_tracking_img.rows;
    auto cols = result_tracking_img.cols;
    auto channels = 3;

    std::vector<size_t> shape = { static_cast<size_t>(rows), static_cast<size_t>(cols), static_cast<size_t>(channels) };
    std::vector<size_t> strides = { sizeof(uint8_t) * cols * 3, sizeof(uint8_t) * 3, sizeof(uint8_t) };

    pybind11::array_t<uint8_t> output(
        pybind11::buffer_info(
            result_tracking_img.data,
            sizeof(uint8_t),
            pybind11::format_descriptor<uint8_t>::format(),
            3,
            shape,
            strides
        )
    );
    return output;
}

bool trackdloApp::update_opencv_mask(pybind11::array_t<uint8_t> mask){
    // comment: Input must be bgr.
    if (!updated_opencv_mask){
        // Check the format of input ndarray
        pybind11::buffer_info buf = mask.request();
        if (!(buf.ndim == 3 && buf.shape[2] == 3)) {
            spdlog::error("Occlusion mask must be a 3-channel image.");
            return false;
        }

        Mat color_mat(buf.shape[0], buf.shape[1], CV_8UC3, buf.ptr);
        occlusion_mask = color_mat.clone();

        updated_opencv_mask = true;
        spdlog::info("Occlusion mask is assigned.");
    }else{
        spdlog::warn("Occlusion mask has already been assigned. Call the function reset() first if you want to overwrite it.");
        return false;
    }

    return true;
}

bool trackdloApp::update_init_nodes(const MatrixXd& nodes){
    if (!received_init_nodes){
        init_nodes = nodes;
        received_init_nodes = true;
        spdlog::info("init_nodes is assigned.");
    }else{
        spdlog::warn("init_nodes has already been assigned. Call the function reset() first if you want to overwrite it.");
        return false;
    }

    return true;
}

bool trackdloApp::update_camera_info(const MatrixXd& cam_proj_matrix){
    if (!received_proj_matrix){
        // check the dimension of the projection matrix
        if (cam_proj_matrix.rows() != 3 || cam_proj_matrix.cols() != 4){
            spdlog::error("cam_proj_matrix must be a 3*4 matrix.");
            return false;
        }

        proj_matrix = cam_proj_matrix;
        received_proj_matrix = true;
        spdlog::info("cam_proj_matrix is assigned.");
    }else{
        spdlog::warn("cam_proj_matrix has already been assigned. Call the function reset() first if you want to overwrite it.");
        return false;
    }

    return true;
}

void trackdloApp::reset(){
    updated_opencv_mask = false;
    received_init_nodes = false;
    received_proj_matrix = false;
    spdlog::info("Reset flags of occlusion_mask, init_nodes, and proj_matrix.");
}

void trackdloApp::load_config_files(){
    YAML::Node config = YAML::LoadFile(config_file_path);
    
    multi_color_dlo = config["multi_color_dlo"].as<bool>();

    upper = config["hsv_filter"]["upper"].as<std::vector<int>>();
    lower = config["hsv_filter"]["lower"].as<std::vector<int>>();

    visibility_threshold = config["trackdlo"]["visibility_threshold"].as<double>();
    dlo_pixel_width = config["trackdlo"]["dlo_pixel_width"].as<int>();
    beta = config["trackdlo"]["beta"].as<double>();
    beta_pre_proc = config["trackdlo"]["beta_pre_proc"].as<double>();
    lambda = config["trackdlo"]["lambda"].as<double>();
    lambda_pre_proc = config["trackdlo"]["lambda_pre_proc"].as<double>();
    alpha = config["trackdlo"]["alpha"].as<double>();
    mu = config["trackdlo"]["mu"].as<double>();
    max_iter = config["trackdlo"]["max_iter"].as<int>();
    tol = config["trackdlo"]["tol"].as<double>();
    k_vis = config["trackdlo"]["k_vis"].as<double>();
    d_vis = config["trackdlo"]["d_vis"].as<double>();
    lle_weight = config["trackdlo"]["lle_weight"].as<double>();
    downsample_leaf_size = config["trackdlo"]["downsample_leaf_size"].as<double>();
    if (config["logging_level"]) { logging_level = config["logging_level"].as<std::string>(); }

    std::transform(logging_level.begin(), logging_level.end(), logging_level.begin(), ::tolower);
    if (logging_level == "trace") {
        spdlog::set_level(spdlog::level::trace);
    } else if (logging_level == "debug") {
        spdlog::set_level(spdlog::level::debug);
    } else if (logging_level == "info") {
        spdlog::set_level(spdlog::level::info);
    } else if (logging_level == "warn") {
        spdlog::set_level(spdlog::level::warn);
    } else if (logging_level == "error" || logging_level == "err") {
        spdlog::set_level(spdlog::level::err);
    } else if (logging_level == "critical") {
        spdlog::set_level(spdlog::level::critical);
    } else if (logging_level == "off") {
        spdlog::set_level(spdlog::level::off);
    } else {
        spdlog::warn("Unknown logging level '{}', using default: info", logging_level);
        spdlog::set_level(spdlog::level::info);
    }
    spdlog::info("Logging level set to '{}'", logging_level);
}

void trackdloApp::print_params(){
    std::string output_string = "\nconfig File: " + config_file_path + "\n";
    output_string += "======================= Parameters =======================\n";
    output_string += "HSV Filter:\n";
    output_string += "  upper: " + std::to_string(upper[0]) + ", " + std::to_string(upper[1]) + ", " + std::to_string(upper[2]) + "\n";
    output_string += "  lower: " + std::to_string(lower[0]) + ", " + std::to_string(lower[1]) + ", " + std::to_string(lower[2]) + "\n";
    output_string += "trackdlo:\n";
    output_string += "  multi_color_dlo:        " + std::to_string(multi_color_dlo) + "\n";
    output_string += "  visibility_threshold:   " + std::to_string(visibility_threshold) + "\n";
    output_string += "  dlo_pixel_width:        " + std::to_string(dlo_pixel_width) + "\n";
    output_string += "  beta:                   " + std::to_string(beta) + "\n";
    output_string += "  lambda:                 " + std::to_string(lambda) + "\n";
    output_string += "  alpha:                  " + std::to_string(alpha) + "\n";
    output_string += "  mu:                     " + std::to_string(mu) + "\n";
    output_string += "  max_iter:               " + std::to_string(max_iter) + "\n";
    output_string += "  tol:                    " + std::to_string(tol) + "\n";
    output_string += "  k_vis:                  " + std::to_string(k_vis) + "\n";
    output_string += "  d_vis:                  " + std::to_string(d_vis) + "\n";
    output_string += "  beta_pre_proc:          " + std::to_string(beta_pre_proc) + "\n";
    output_string += "  lambda_pre_proc:        " + std::to_string(lambda_pre_proc) + "\n";
    output_string += "  lle_weight:             " + std::to_string(lle_weight) + "\n";
    output_string += "  downsample_leaf_size:   " + std::to_string(downsample_leaf_size) + "\n";
    output_string += "==========================================================";
    spdlog::info(output_string);
}

Mat trackdloApp::color_thresholding(Mat cur_image_hsv) {
    std::vector<int> lower_blue = {90, 90, 60};
    std::vector<int> upper_blue = {130, 255, 255};

    std::vector<int> lower_red_1 = {130, 60, 50};
    std::vector<int> upper_red_1 = {255, 255, 255};

    std::vector<int> lower_red_2 = {0, 60, 50};
    std::vector<int> upper_red_2 = {10, 255, 255};

    std::vector<int> lower_yellow = {15, 100, 80};
    std::vector<int> upper_yellow = {40, 255, 255};

    Mat mask_blue, mask_red_1, mask_red_2, mask_red, mask_yellow, mask;
    // filter blue
    cv::inRange(cur_image_hsv, cv::Scalar(lower_blue[0], lower_blue[1], lower_blue[2]), cv::Scalar(upper_blue[0], upper_blue[1], upper_blue[2]), mask_blue);

    // filter red
    cv::inRange(cur_image_hsv, cv::Scalar(lower_red_1[0], lower_red_1[1], lower_red_1[2]), cv::Scalar(upper_red_1[0], upper_red_1[1], upper_red_1[2]), mask_red_1);
    cv::inRange(cur_image_hsv, cv::Scalar(lower_red_2[0], lower_red_2[1], lower_red_2[2]), cv::Scalar(upper_red_2[0], upper_red_2[1], upper_red_2[2]), mask_red_2);

    // filter yellow
    cv::inRange(cur_image_hsv, cv::Scalar(lower_yellow[0], lower_yellow[1], lower_yellow[2]), cv::Scalar(upper_yellow[0], upper_yellow[1], upper_yellow[2]), mask_yellow);

    // combine red mask
    cv::bitwise_or(mask_red_1, mask_red_2, mask_red);
    // combine overall mask
    cv::bitwise_or(mask_red, mask_blue, mask);
    cv::bitwise_or(mask_yellow, mask, mask);

    return mask;
}

int trackdloApp::execute(pybind11::array_t<uint8_t> rgb_img_array, pybind11::array_t<uint16_t> depth_img_array){
    // convert pybind11 data structure to cv::Mat
    pybind11::buffer_info rgb_buf = rgb_img_array.request();
    pybind11::buffer_info depth_buf = depth_img_array.request();

    Mat rgb_mat_ptr(rgb_buf.shape[0], rgb_buf.shape[1], CV_8UC3, rgb_buf.ptr);
    Mat depth_mat_ptr(depth_buf.shape[0], depth_buf.shape[1], CV_16UC1, depth_buf.ptr);

    Mat cur_image_orig = rgb_mat_ptr.clone();
    Mat cur_depth = depth_mat_ptr.clone();

    try{
        if (!initialized) {
            if (received_init_nodes && received_proj_matrix) {
                tracker = trackdlo(init_nodes.rows(), visibility_threshold, beta, lambda, alpha, k_vis, mu, max_iter, tol, beta_pre_proc, lambda_pre_proc, lle_weight);
    
                sigma2 = 0.001;
    
                // record geodesic coord
                double cur_sum = 0;
                for (int i = 0; i < init_nodes.rows()-1; i ++) {
                    cur_sum += (init_nodes.row(i+1) - init_nodes.row(i)).norm();
                    converted_node_coord.push_back(cur_sum);
                }
    
                tracker.initialize_nodes(init_nodes);
                tracker.initialize_geodesic_coord(converted_node_coord);
                Y = init_nodes.replicate(1, 1);
    
                initialized = true;
                spdlog::info("First time receive the init_nodes, wait for next call and start tracking.");
                return 0;
            }
            spdlog::warn("Have not received init_nodes or proj_matrixs yet");
        }else{
            // log time
            std::chrono::high_resolution_clock::time_point cur_time_cb = std::chrono::high_resolution_clock::now();
            double time_diff;
            std::chrono::high_resolution_clock::time_point cur_time;
    
            Mat mask, mask_rgb, mask_without_occlusion_block;
            Mat cur_image_hsv;
    
            // convert color
            cv::cvtColor(cur_image_orig, cur_image_hsv, cv::COLOR_BGR2HSV);
    
            if (!multi_color_dlo) {
                // color_thresholding
                cv::inRange(cur_image_hsv, cv::Scalar(lower[0], lower[1], lower[2]), cv::Scalar(upper[0], upper[1], upper[2]), mask_without_occlusion_block);
            }else{
                mask_without_occlusion_block = color_thresholding(cur_image_hsv);
            }
    
            // cv::morphologyEx(mask_without_occlusion_block, mask_without_occlusion_block, 
            //     cv::MORPH_CLOSE, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(10, 10)));
    
            // update cur image for visualization
            Mat cur_image;
            Mat occlusion_mask_gray;
            if (updated_opencv_mask) {
                cv::cvtColor(occlusion_mask, occlusion_mask_gray, cv::COLOR_BGR2GRAY);
                cv::bitwise_and(mask_without_occlusion_block, occlusion_mask_gray, mask);
                cv::bitwise_and(cur_image_orig, occlusion_mask, cur_image);
            }
            else {
                mask_without_occlusion_block.copyTo(mask);
                cur_image_orig.copyTo(cur_image);
            }
    
            cv::cvtColor(mask, mask_rgb, cv::COLOR_GRAY2BGR);
    
            bool simulated_occlusion = false;
            int occlusion_corner_i = -1;
            int occlusion_corner_j = -1;
            int occlusion_corner_i_2 = -1;
            int occlusion_corner_j_2 = -1;
    
            // filter point cloud
            pcl::PointCloud<pcl::PointXYZRGB> cur_pc;
            pcl::PointCloud<pcl::PointXYZRGB> cur_pc_downsampled;
    
            // filter point cloud from mask
            for (int i = 0; i < mask.rows; i ++) {
                for (int j = 0; j < mask.cols; j ++) {
                    // for text label (visualization)
                    if (updated_opencv_mask && !simulated_occlusion && occlusion_mask_gray.at<uchar>(i, j) == 0) {
                        occlusion_corner_i = i;
                        occlusion_corner_j = j;
                        simulated_occlusion = true;
                    }
    
                    // update the other corner of occlusion mask (visualization)
                    if (updated_opencv_mask && occlusion_mask_gray.at<uchar>(i, j) == 0) {
                        occlusion_corner_i_2 = i;
                        occlusion_corner_j_2 = j;
                    }
    
                    if (mask.at<uchar>(i, j) != 0) {
                        // point cloud from image pixel coordinates and depth value
                        pcl::PointXYZRGB point;
                        double pixel_x = static_cast<double>(j);
                        double pixel_y = static_cast<double>(i);
                        double cx = proj_matrix(0, 2);
                        double cy = proj_matrix(1, 2);
                        double fx = proj_matrix(0, 0);
                        double fy = proj_matrix(1, 1);
                        double pc_z = cur_depth.at<uint16_t>(i, j) / 1000.0;
    
                        point.x = (pixel_x - cx) * pc_z / fx;
                        point.y = (pixel_y - cy) * pc_z / fy;
                        point.z = pc_z;
    
                        // currently something so color doesn't show up in rviz
                        point.r = cur_image_orig.at<cv::Vec3b>(i, j)[0];
                        point.g = cur_image_orig.at<cv::Vec3b>(i, j)[1];
                        point.b = cur_image_orig.at<cv::Vec3b>(i, j)[2];
    
                        cur_pc.push_back(point);
                    }
                }
            }
    
            // Perform downsampling
            pcl::PointCloud<pcl::PointXYZRGB>::ConstPtr cloudPtr(cur_pc.makeShared());
            pcl::VoxelGrid<pcl::PointXYZRGB> sor;
            sor.setInputCloud (cloudPtr);
            sor.setLeafSize (downsample_leaf_size, downsample_leaf_size, downsample_leaf_size);
            sor.filter(cur_pc_downsampled);
    
            MatrixXd X = cur_pc_downsampled.getMatrixXfMap().topRows(3).transpose().cast<double>();
            spdlog::info("Number of points in downsampled point cloud: " + std::to_string(X.rows()));
    
            MatrixXd guide_nodes;
            std::vector<MatrixXd> priors;
    
            // log time
            time_diff = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - cur_time_cb).count() / 1000.0;
            spdlog::info("Before tracking step: " + std::to_string(time_diff) + " ms");
            pre_proc_total += time_diff;
            cur_time = std::chrono::high_resolution_clock::now();
    
            // calculate node visibility
            // for each node in Y, determine its shortest distance to X
            // for each point in X, determine its shortest distance to Y
            std::map<int, double> shortest_node_pt_dists;
            std::vector<double> shortest_pt_node_dists(X.rows(), 100000.0);
            for (int m = 0; m < Y.rows(); m ++) {
                int closest_pt_idx = 0;
                double shortest_dist = 100000;
                // loop through all points in X
                for (int n = 0; n < X.rows(); n ++) {
                    double dist = (Y.row(m) - X.row(n)).norm();
                    // update shortest dist for Y
                    if (dist < shortest_dist) {
                        closest_pt_idx = n;
                        shortest_dist = dist;
                    }
    
                    // update shortest dist for X
                    if (dist < shortest_pt_node_dists[n]) {
                        shortest_pt_node_dists[n] = dist;
                    }
                }
                shortest_node_pt_dists.insert(std::pair<int, double>(m, shortest_dist));
            }
    
            // for current nodes and edges in Y, sort them based on how far away they are from the camera
            std::vector<double> averaged_node_camera_dists = {};
            std::vector<int> indices_vec = {};
            for (int i = 0; i < Y.rows()-1; i ++) {
                averaged_node_camera_dists.push_back(((Y.row(i) + Y.row(i+1)) / 2).norm());
                indices_vec.push_back(i);
            }
            // sort
            std::sort(indices_vec.begin(), indices_vec.end(),
                [&](const int& a, const int& b) {
                    return (averaged_node_camera_dists[a] < averaged_node_camera_dists[b]);
                }
            );
            Mat projected_edges = Mat::zeros(mask.rows, mask.cols, CV_8U);
    
            // project Y^{t-1} onto projected_edges
            MatrixXd Y_h = Y.replicate(1, 1);
            Y_h.conservativeResize(Y_h.rows(), Y_h.cols()+1);
            Y_h.col(Y_h.cols()-1) = MatrixXd::Ones(Y_h.rows(), 1);
            MatrixXd image_coords_mask = (proj_matrix * Y_h.transpose()).transpose();
    
            std::vector<int> visible_nodes = {};
            std::vector<int> self_occluded_nodes = {};
            std::vector<int> not_self_occluded_nodes = {};
            std::vector<int> self_occluding_nodes = {};
    
            // draw edges closest to the camera first
            for (int idx : indices_vec) {
                int col_1 = static_cast<int>(image_coords_mask(idx, 0)/image_coords_mask(idx, 2));
                int row_1 = static_cast<int>(image_coords_mask(idx, 1)/image_coords_mask(idx, 2));
    
                int col_2 = static_cast<int>(image_coords_mask(idx+1, 0)/image_coords_mask(idx+1, 2));
                int row_2 = static_cast<int>(image_coords_mask(idx+1, 1)/image_coords_mask(idx+1, 2));
    
                // only add to visible nodes if did not overlap with existing edges
                if (projected_edges.at<uchar>(row_1, col_1) == 0) {
                    if (shortest_node_pt_dists[idx] <= visibility_threshold) {
                        if (std::find(visible_nodes.begin(), visible_nodes.end(), idx) == visible_nodes.end()) {
                            visible_nodes.push_back(idx);
                        }
                    }
                    if (std::find(not_self_occluded_nodes.begin(), not_self_occluded_nodes.end(), idx) == not_self_occluded_nodes.end()) {
                        not_self_occluded_nodes.push_back(idx);
                    }
                }
    
                // do not consider adjacent nodes directly on top of each other
                if (projected_edges.at<uchar>(row_2, col_2) == 0) {
                    if (shortest_node_pt_dists[idx+1] <= visibility_threshold) {
                        if (std::find(visible_nodes.begin(), visible_nodes.end(), idx+1) == visible_nodes.end()) {
                            visible_nodes.push_back(idx+1);
                        }
                    }
                    if (std::find(not_self_occluded_nodes.begin(), not_self_occluded_nodes.end(), idx+1) == not_self_occluded_nodes.end()) {
                        not_self_occluded_nodes.push_back(idx+1);
                    }
                }
    
                // add edges for checking overlap with upcoming nodes
                double x1 = col_1;
                double y1 = row_1;
                double x2 = col_2;
                double y2 = row_2;
                cv::line(projected_edges, cv::Point(x1, y1), cv::Point(x2, y2), cv::Scalar(255, 255, 255), dlo_pixel_width);
            }
    
            // sort visible nodes to preserve the original connectivity
            std::sort(visible_nodes.begin(), visible_nodes.end());
    
            // minor mid-section occlusion is usually fine
            // extend visible nodes so that gaps as small as 2 to 3 nodes are filled
            std::vector<int> visible_nodes_extended = {};
            spdlog::debug("visible_nodes.size(): " + std::to_string(visible_nodes.size()));
            if (visible_nodes.size() <= 0) throw std::runtime_error("No visible_nodes");
            for (int i = 0; i < visible_nodes.size()-1; i ++) {
                visible_nodes_extended.push_back(visible_nodes[i]);
                // extend visible nodes
                if (fabs(converted_node_coord[visible_nodes[i+1]] - converted_node_coord[visible_nodes[i]]) <= d_vis) {
                    for (int j = 1; j < visible_nodes[i+1] - visible_nodes[i]; j ++) {
                        visible_nodes_extended.push_back(visible_nodes[i] + j);
                    }
                }
            }
            visible_nodes_extended.push_back(visible_nodes[visible_nodes.size()-1]);
    
            // store Y_0 for post processing
            MatrixXd Y_0 = Y.replicate(1, 1);
    
            // step tracker
            tracker.tracking_step(X, visible_nodes, visible_nodes_extended, proj_matrix, mask.rows, mask.cols);
            Y = tracker.get_tracking_result();
            guide_nodes = tracker.get_guide_nodes();
            priors = tracker.get_correspondence_pairs();
    
            // log time
            time_diff = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - cur_time).count() / 1000.0;
            spdlog::info("Tracking step: " + std::to_string(time_diff) + " ms");
            algo_total += time_diff;
            cur_time = std::chrono::high_resolution_clock::now();
    
            // projection and pub image
            averaged_node_camera_dists = {};
            indices_vec = {};
            for (int i = 0; i < Y.rows()-1; i ++) {
                averaged_node_camera_dists.push_back(((Y.row(i) + Y.row(i+1)) / 2).norm());
                indices_vec.push_back(i);
            }
            // sort
            std::sort(indices_vec.begin(), indices_vec.end(),
                [&](const int& a, const int& b) {
                    return (averaged_node_camera_dists[a] < averaged_node_camera_dists[b]);
                }
            );
            std::reverse(indices_vec.begin(), indices_vec.end());
    
            MatrixXd nodes_h = Y.replicate(1, 1);
            nodes_h.conservativeResize(nodes_h.rows(), nodes_h.cols()+1);
            nodes_h.col(nodes_h.cols()-1) = MatrixXd::Ones(nodes_h.rows(), 1);
            MatrixXd image_coords = (proj_matrix * nodes_h.transpose()).transpose();
    
            Mat tracking_img;
            tracking_img = 0.5*cur_image_orig + 0.5*cur_image;
    
            // std::vector<int> vis = visible_nodes;
            std::vector<int> vis = not_self_occluded_nodes;
    
            // draw points
            for (int idx : indices_vec) {
    
                int x = static_cast<int>(image_coords(idx, 0)/image_coords(idx, 2));
                int y = static_cast<int>(image_coords(idx, 1)/image_coords(idx, 2));
    
                cv::Scalar point_color;
                cv::Scalar line_color;
    
                if (std::find(vis.begin(), vis.end(), idx) != vis.end()) {
                    point_color = cv::Scalar(0, 150, 255);
                    line_color = cv::Scalar(0, 255, 0);
                }
                else {
                    point_color = cv::Scalar(0, 0, 255);
    
                    // line is colored red only when both bounding nodes are not visible
                    if (std::find(vis.begin(), vis.end(), idx+1) == vis.end()) {
                        line_color = cv::Scalar(0, 0, 255);
                    }
                    else {
                        line_color = cv::Scalar(0, 255, 0);
                    }
                }
    
                cv::line(tracking_img, cv::Point(x, y),
                                       cv::Point(static_cast<int>(image_coords(idx+1, 0)/image_coords(idx+1, 2)), 
                                                 static_cast<int>(image_coords(idx+1, 1)/image_coords(idx+1, 2))),
                                       line_color, 5);
    
                cv::circle(tracking_img, cv::Point(x, y), 7, point_color, -1);
    
                if (std::find(vis.begin(), vis.end(), idx+1) != vis.end()) {
                    point_color = cv::Scalar(0, 150, 255);
                }
                else {
                    point_color = cv::Scalar(0, 0, 255);
                }
                cv::circle(tracking_img, cv::Point(static_cast<int>(image_coords(idx+1, 0)/image_coords(idx+1, 2)), 
                                                    static_cast<int>(image_coords(idx+1, 1)/image_coords(idx+1, 2))),
                                                    7, point_color, -1);
            }
    
            // add text
            if (updated_opencv_mask && simulated_occlusion) {
                cv::putText(tracking_img, "occlusion", cv::Point(occlusion_corner_j, occlusion_corner_i-10), cv::FONT_HERSHEY_DUPLEX, 1.2, cv::Scalar(0, 0, 240), 2);
            }

            // store the results
            result_tracking_img = tracking_img.clone();
            has_result = true;
    
            return 1;
        }
    }
    catch (const std::exception& e){
        std::string err_msg = e.what();
        spdlog::error(err_msg);
    }
    catch (...) {
        spdlog::error("Unknown exception caught!");
    }
    
    return -1;
}