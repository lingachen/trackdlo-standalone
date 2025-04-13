import pyrealsense2 as rs
import numpy as np
import cv2
import time
import logging

import open3d as o3d

import sys
sys.path.append('/home/chenla/Desktop/mount/trackdlo-standalone/trackdlo_c++/build/')

from trackdlo_app import trackdloApp
from trackdlo_python.Initializer import Initializer

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)

def get_camera_proj_matrix(input_profile):
    video_stream = input_profile.get_stream(rs.stream.color).as_video_stream_profile()
    intr = video_stream.get_intrinsics()

    P = np.array([
        [intr.fx, 0,       intr.ppx, 0],
        [0,       intr.fy, intr.ppy, 0],
        [0,       0,       1,        0]
    ])

    return P




VISUALIZED = False

ctx = rs.context()
pipeline = rs.pipeline(ctx)
config = rs.config()

config.enable_stream(rs.stream.depth, 1280, 720, rs.format.z16, 15)
config.enable_stream(rs.stream.color, 1280, 720, rs.format.bgr8, 15)
profile = pipeline.start(config)

print("Wait Auto-exposure become stable...")
time.sleep(2)

cam_proj_matrix = get_camera_proj_matrix(profile)
config_path = "/home/chenla/Desktop/mount/trackdlo-standalone/config/default.yaml"

initializer = Initializer(config_path=config_path, visualized=VISUALIZED)
initializer.update_cam_info(cam_proj_matrix)
init_nodes = None
print("Init initializer")

tA = trackdloApp(config_path)
tA.update_camera_info(cam_proj_matrix)
has_init = False
print("Init trackdloApp")

try:
    while True:
        time.sleep(0.01)

        frames = pipeline.wait_for_frames()

        depth_frame = frames.get_depth_frame()
        color_frame = frames.get_color_frame()
        if not depth_frame or not color_frame:
            continue

        depth_image = np.asanyarray(depth_frame.get_data())
        color_image = np.asanyarray(color_frame.get_data())

        # cv2.imshow("RealSense RGB", color_image)
        # cv2.imshow("RealSense Depth", depth_image)

        # if cv2.waitKey(1) & 0xFF == ord('q'):
        #     break

        if init_nodes is None:
            print("Get init_nodes...")
            init_nodes = initializer.execute(color_image, depth_image)
        
        
        if init_nodes is not None:
            if not has_init:
                print("Update init_nodes...")
                r = tA.update_init_nodes(init_nodes)

                # Visualization
                if VISUALIZED:
                    pcd = o3d.geometry.PointCloud()
                    pcd.points = o3d.utility.Vector3dVector(init_nodes)
                    o3d.visualization.draw_geometries([pcd])

                if r:
                    has_init = True

            print("Start execute trackdloApp")
            r = tA.execute(color_image, depth_image)

            if r>0:
                result = tA.get_result_image()
                print("Get result!")
                cv2.imshow("trackdloResult", result)
                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break
            elif r<0:
                print("trackApp execute failed")

except Exception as e:
    print(str(e))
    pass

finally:
    pipeline.stop()
    cv2.destroyAllWindows()