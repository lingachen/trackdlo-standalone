import copy
import time
import cv2
import numpy as np
import traceback
import yaml
import logging

from scipy import interpolate

from .utils import extract_connected_skeleton

class Initializer:
    def __init__(self, config_path, visualized=False):
        self.config_path = config_path

        self.visualized = visualized

        # parameters
        self.num_of_nodes = None
        self.multi_color_dlo = False
        self.hsv_filter_upper = None
        self.hsv_filter_lower = None

        self.proj_matrix = None

        self.load_config()
        self.print_params()
        
    def load_config(self, config_path=None):
        if config_path is not None:
            self.config_path = config_path
        
        with open(self.config_path, 'r') as f:
            config = yaml.load(f)

        self.num_of_nodes = config["num_of_nodes"]
        self.multi_color_dlo = config["multi_color_dlo"]
        self.hsv_filter_upper = config["hsv_filter"]["upper"]
        self.hsv_filter_lower = config["hsv_filter"]["lower"]

    def print_params(self):
        logging.info(f"config File: {self.config_path}")
        logging.info("======================= Parameters =======================")
        logging.info(f"multi_color_dlo: {self.multi_color_dlo}")
        logging.info(f"number of nodes: {self.num_of_nodes}")
        logging.info("HSV Filter:")
        logging.info(f"     upper:{str(self.hsv_filter_upper)}")
        logging.info(f"     lower:{str(self.hsv_filter_lower)}")

    def update_cam_info(self, camera_proj_matrix: np.array):
        assert camera_proj_matrix.shape == (3,4)
        
        if self.proj_matrix is not None:
            logging.warning("Overwrite camera_proj_matrix!")

        self.proj_matrix = copy.deepcopy(camera_proj_matrix)

    def remove_duplicate_rows(self, array):
        _, idx = np.unique(array, axis=0, return_index=True)
        data = array[np.sort(idx)]
        self.num_of_nodes = len(data) # ??

        return data
    
    def execute(self, rgb_img, depth_img):

        hsv_image = cv2.cvtColor(rgb_img.copy(), cv2.COLOR_RGB2HSV)
        h, s, v = cv2.split(hsv_image)

        if not self.multi_color_dlo:
            mask = cv2.inRange(hsv_image, np.array(self.hsv_filter_lower), np.array(self.hsv_filter_upper))

            if self.visualized:
                logging.info("Press esc to keep going")
                cv2.imshow('mask', mask)
                while True:
                    key = cv2.waitKey(10)
                    if key == 27:  # escape
                        cv2.destroyAllWindows()
                        break
        else:
            # TODO: implement multi_color_dlo
            mask = None
            mask_tip = None
            pass

        try:
            start_time = time.time()
            mask = cv2.cvtColor(mask.copy(), cv2.COLOR_GRAY2BGR)

            # returns the pixel coord of points (in order). a list of lists
            img_scale = 1
            extracted_chains = extract_connected_skeleton(self.visualized, mask, img_scale=img_scale, seg_length=8, max_curvature=25)

            all_pixel_coords = []
            for chain in extracted_chains:
                all_pixel_coords += chain
            logging.info(f'Finished extracting chains. Time taken: {time.time()-start_time}')

            all_pixel_coords = np.array(all_pixel_coords) * img_scale
            all_pixel_coords = np.flip(all_pixel_coords, 1)

            pc_z = depth_img[tuple(map(tuple, all_pixel_coords.T))] / 1000.0
            fx = self.proj_matrix[0, 0]
            fy = self.proj_matrix[1, 1]
            cx = self.proj_matrix[0, 2]
            cy = self.proj_matrix[1, 2]
            pixel_x = all_pixel_coords[:, 1]
            pixel_y = all_pixel_coords[:, 0]
            # if the first mask value is not in the tip mask, reverse the pixel order
            if self.multi_color_dlo:
                pixel_value1 = mask_tip[pixel_y[-1],pixel_x[-1]]
                if pixel_value1 == 255:
                    pixel_x, pixel_y = pixel_x[::-1], pixel_y[::-1]

            pc_x = (pixel_x - cx) * pc_z / fx
            pc_y = (pixel_y - cy) * pc_z / fy
            extracted_chains_3d = np.vstack((pc_x, pc_y))
            extracted_chains_3d = np.vstack((extracted_chains_3d, pc_z))
            extracted_chains_3d = extracted_chains_3d.T

            # do not include those without depth values
            extracted_chains_3d = extracted_chains_3d[((extracted_chains_3d[:, 0] != 0) | (extracted_chains_3d[:, 1] != 0) | (extracted_chains_3d[:, 2] != 0))]

            if self.multi_color_dlo:
                depth_threshold = 0.57  # m
                extracted_chains_3d = extracted_chains_3d[extracted_chains_3d[:, 2] > depth_threshold]

            # tck, u = interpolate.splprep(extracted_chains_3d.T, s=0.001)
            tck, u = interpolate.splprep(extracted_chains_3d.T, s=0.0005)
            # 1st fit, less points
            u_fine = np.linspace(0, 1, 300) # <-- num fit points
            x_fine, y_fine, z_fine = interpolate.splev(u_fine, tck)
            spline_pts = np.vstack((x_fine, y_fine, z_fine)).T

            # 2nd fit, higher accuracy
            num_true_pts = int(np.sum(np.sqrt(np.sum(np.square(np.diff(spline_pts, axis=0)), axis=1))) * 1000)
            u_fine = np.linspace(0, 1, num_true_pts) # <-- num true points
            x_fine, y_fine, z_fine = interpolate.splev(u_fine, tck)
            spline_pts = np.vstack((x_fine, y_fine, z_fine)).T

            nodes = spline_pts[np.linspace(0, num_true_pts-1, self.num_of_nodes).astype(int)]

            init_nodes = self.remove_duplicate_rows(nodes)

            return init_nodes
        except Exception as e:
            logging.error("Failed to extract splines.")
            logging.error(f"{str(traceback.format_exc())}")

        return None
