import cv2
import numpy as np
import argparse

parser = argparse.ArgumentParser(description="Pick HSV color threshold")
parser.add_argument("path", help="Indicate /full-or-relative/path/to/image_file.png")
args = parser.parse_args()
img_path = f"{args.path}"

image = cv2.imread(img_path) 
hsv_image = cv2.cvtColor(image, cv2.COLOR_BGR2HSV)

h, s, v = cv2.split(hsv_image)

cv2.imshow("Original Image", image)
cv2.imshow("Hue (H)", h)
cv2.imshow("Saturation (S)", s)
cv2.imshow("Value (V)", v)

cv2.waitKey(0)
cv2.destroyAllWindows()
