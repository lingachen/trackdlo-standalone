import cv2
import numpy as np
import base64
import threading
from nicegui import ui, app, Client
import nicegui.events as events

from websocket_server import WebsocketServer
import os
import subprocess

def extract_server_and_port(ws_server_address):
    addr = ws_server_address.replace("ws://", '')
    split_addr = addr.split(':')
    return split_addr[0], int(split_addr[1])


class TrackDLOWeb:
    flag = False
    def __init__(self):
        self.node_data = None
        self.support_videotype = ['mp4', 'avi']

        self.ws_server_address = "ws://127.0.0.1:9999"

        self.ui_init()
        self.streaming_flag = False


    def ui_init(self):
        with Client.auto_index_client:
            with ui.row().classes('items-stretch q-col-gutter-md q-pa-md'):
                self.sub_image = ui.interactive_image(size='100%')
                with ui.scene(height=300) as scene:
                    self.point_cloud = scene.point_cloud([], [], point_size=0.01)
                    scene.move_camera(x=0, y=0, z=1)
            
            mode = ui.select(['streaming', 'file'], value='streaming')

            button_area = ui.column().style('margin-top: 10px')

            def update_controls():
                button_area.clear()

                if mode.value == 'streaming':
                    with button_area:
                        self.start_stream_btn = ui.button('Start Streaming', on_click=self.start_streaming_callback)
                        self.stop_stream_btn = ui.button('Stop Streaming', on_click=self.stop_streaming_callback)
                        self.stop_stream_btn.disable()
                elif mode.value == 'file':
                    with button_area:
                        self.file_upload_ui = ui.upload(label='Upload a Video', auto_upload=True, on_upload=self.handle_upload)
                        ui.button('Start Tracking', on_click=lambda: print('Start tracking')).disable()

            mode.on_value_change(lambda: update_controls())
            update_controls()

    def start_streaming_callback(self):
        ui.notify('Start streaming')
        self.streaming_flag = True
        self.start_stream_btn.disable()
        self.stop_stream_btn.enable()
        subprocess.Popen(["python3", "image_sender.py", self.ws_server_address])

    def stop_streaming_callback(self):
        ui.notify('Stop streaming')
        self.streaming_flag = False
        self.start_stream_btn.enable()
        self.stop_stream_btn.disable()

    def handle_upload(self, e: events.UploadEventArguments):
        filename = e.name
        filetype = filename.split('.')[-1]

        if filetype not in self.support_videotype:
            ui.notify(f'Please upload {self.support_videotype}')
            self.file_upload_ui.reset()
            return

        filedata = e.content.read()
        savepath = os.path.join('uploads',filename) 
        with open(savepath, 'wb') as f:
            f.write(filedata) 




    def receive_message(self, client, server, message):
        if self.streaming_flag:
            if message == "Death":
                self.stop_streaming_callback()
            elif message[:7] == 'image: ':
                self.sub_image.set_source(message[7:])
                
            elif message[:7] == 'nodes: ':
                arr_bytes = base64.b64decode(message[7:])
                self.node_data = np.frombuffer(arr_bytes, dtype=float).reshape(-1, 3)
                color = np.zeros((self.node_data.shape[0], 3))
                color[:, 0] = 1 # set to red
                self.point_cloud.set_points(self.node_data, color)
        
        if self.streaming_flag:
            return_message = "OK"
        else:
            return_message = "End"

        server.send_message(client, return_message)


    def new_client(self, client, server):
        print(f"New client(ID={client['id']}) is connected")
    
    def client_left(self, client, server):
        print(f"A client(ID={client['id']}) has left")

    def start_server(self):
        addr, port = extract_server_and_port(self.ws_server_address)
        self.server = WebsocketServer(host=addr, port=port)

        self.server.set_fn_message_received(self.receive_message)
        self.server.set_fn_new_client(self.new_client)
        self.server.set_fn_client_left(self.client_left)

        self.server.run_forever()
    
    def stop_server(self):
        if self.server:
            self.server.shutdown()
            print("WebSocket server stopped.")

trackWeb = TrackDLOWeb()
    
app.on_startup(lambda: threading.Thread(target=trackWeb.start_server).start())
ui.run()