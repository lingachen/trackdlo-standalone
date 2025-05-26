import pyrealsense2 as rs
import numpy as np
import cv2
import time
import logging
import argparse

import sys
sys.path.append('/home/chenla/Desktop/mount/trackdlo-standalone/trackdlo_c++/build/')

from trackdlo_app import trackdloApp
from trackdlo_python.Initializer import Initializer

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)

from websocket import create_connection
import base64

def get_camera_proj_matrix(rs_profile):
    video_stream = rs_profile.get_stream(rs.stream.color).as_video_stream_profile()
    intr = video_stream.get_intrinsics()

    P = np.array([
        [intr.fx, 0,       intr.ppx, 0],
        [0,       intr.fy, intr.ppy, 0],
        [0,       0,       1,        0]
    ])

    return P

class Tracking:
    def __init__(self, args):
        self.ws_server_address = args.websocket_server_address
        self.video_fp = args.video_file if args.video_file else None
        self.config_fp = args.config_path

        self.run_flag = True

    def connect_ws_server(self):
        try:
            self.ws = create_connection(self.ws_server_address, timeout=3)
        except:
            print("Failed to connect to Web server. Please make sure that the web server is on.")
            self.ws = None

    def send_message(self, message):
        if self.ws is not None:
            self.ws.send(message)

    def send_image(self, image):
        if self.ws is not None:
            _, buffer = cv2.imencode('.jpg', image)
            jpg_as_text = base64.b64encode(buffer).decode('utf-8')
            self.ws.send(f'image: data:image/jpeg;base64,{jpg_as_text}')
            try:
                result = self.ws.recv()
            except:
                self.run_flag = False
            
            if result == "end":
                self.run_flag = False

    def send_nodes(self, nodes):
        if self.ws is not None:
            arr_bytes = nodes.tobytes()
            arr_base64 = base64.b64encode(arr_bytes).decode()
            self.ws.send(f'nodes: {arr_base64}')
            try:
                result = self.ws.recv()
            except:
                self.run_flag = False
            
            if result == "End":
                self.run_flag = False

    def camera_streaming(self):
        ctx = rs.context()
        pipeline = rs.pipeline(ctx)
        config = rs.config()

        config.enable_stream(rs.stream.depth, 1280, 720, rs.format.z16, 15)
        config.enable_stream(rs.stream.color, 1280, 720, rs.format.bgr8, 15)
        profile = pipeline.start(config)

        # print("Wait Auto-exposure become stable...")
        time.sleep(2)

        cam_proj_matrix = get_camera_proj_matrix(profile)

        initializer = Initializer(config_path=self.config_fp)
        initializer.update_cam_info(cam_proj_matrix)
        init_nodes = None

        tA = trackdloApp(self.config_fp)
        tA.update_camera_info(cam_proj_matrix)
        has_init = False

        try:
            while self.run_flag:
                time.sleep(0.03)

                frames = pipeline.wait_for_frames()

                depth_frame = frames.get_depth_frame()
                color_frame = frames.get_color_frame()
                if not depth_frame or not color_frame:
                    continue

                depth_image = np.asanyarray(depth_frame.get_data())
                color_image = np.asanyarray(color_frame.get_data())

                if init_nodes is None:
                    # print("Get init_nodes...")
                    init_nodes = initializer.execute(color_image, depth_image)
                
                
                if init_nodes is not None:
                    if not has_init:
                        # print("Update init_nodes...")
                        r = tA.update_init_nodes(init_nodes)

                        if r:
                            has_init = True

                    # print("Start execute trackdloApp")
                    r = tA.execute(color_image, depth_image)

                    if r>0:
                        result = tA.get_result_image()
                        self.send_image(result)
                        nodes = tA.get_nodes_array()
                        self.send_nodes(nodes)
                        # print("Get result!")
                    elif r<0:
                        # print("trackApp execute failed")
                        pass

        except Exception as e:
            print(str(e))
            pass

        finally:
            self.send_message("Death")
            pipeline.stop()
            cv2.destroyAllWindows()


    def run(self):
        self.connect_ws_server()
        if self.video_fp is None:
            self.camera_streaming()

def build_parser():
    parser = argparse.ArgumentParser()

    parser.add_argument("websocket_server_address")


    parser.add_argument("--config_path", default="./config/default.yaml", help="input video file path")
    parser.add_argument("--video_file", help="input video file path")
    return parser


if __name__ == "__main__":
    parser = build_parser()
    args = parser.parse_args()

    tracking = Tracking(args)
    tracking.run()

