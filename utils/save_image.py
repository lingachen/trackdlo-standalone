import pyrealsense2 as rs
import numpy as np
import cv2
import time

ctx = rs.context()
devices = ctx.query_devices()

if len(ctx.devices) > 0:
    for dev in ctx.devices:
        print('Found device:', dev.get_info(rs.camera_info.name), dev.get_info(rs.camera_info.serial_number))
else:
    print("No Intel Device connected")

pipeline = rs.pipeline(ctx)
config = rs.config()

config.enable_stream(rs.stream.depth, 1280, 720, rs.format.z16, 15)
config.enable_stream(rs.stream.color, 1280, 720, rs.format.bgr8, 15)

pipeline.start(config)
print("Wait Auto-exposure become stable...")
time.sleep(2)

idx = 0
try:
    while True:
        frames = pipeline.wait_for_frames()

        depth_frame = frames.get_depth_frame()
        color_frame = frames.get_color_frame()
        if not depth_frame or not color_frame:
            continue

        depth_image = np.asanyarray(depth_frame.get_data())
        color_image = np.asanyarray(color_frame.get_data())

        
        cv2.imwrite(f"image_{idx:04}.jpg", color_image)
        print(f"Save image: image_{idx:04}.jpg")

        time.sleep(0.3)
        idx += 1

finally:
    pipeline.stop()
