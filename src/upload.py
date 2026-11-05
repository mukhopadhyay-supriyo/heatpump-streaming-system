import paho.mqtt.client as mqtt
import os
import time
import struct
import zlib

BROKER = "ec2-51-21-3-129.eu-north-1.compute.amazonaws.com"
PORT = 1883

CHUNK_SIZE = 1024

TOPIC_START = "esp32/file/start"
TOPIC_CHUNK = "esp32/file/chunk"
TOPIC_END = "esp32/file/end"
TOPIC_ACK = "esp32/file/ack"


class MQTTFileUploader:

    def __init__(self):

        self.ack_received = None

        self.client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)  # type: ignore

        self.client.on_message = self.on_message

        self.client.connect(BROKER, PORT, 60)

        self.client.loop_start()

        self.client.subscribe(TOPIC_ACK, qos=1)

        time.sleep(1)

    def on_message(self, client, userdata, msg):

        self.ack_received = msg.payload.decode()

    def wait_ack(self, expected, timeout=10):

        start = time.time()

        while time.time() - start < timeout:

            if self.ack_received == expected:

                self.ack_received = None
                return True

            time.sleep(0.01)

        return False

    def upload(self, file_path):

        with open(file_path, "rb") as f:
            data = f.read()

        file_size = len(data)

        total_chunks = (file_size + CHUNK_SIZE - 1) // CHUNK_SIZE

        checksum = zlib.crc32(data) & 0xFFFFFFFF

        print("File:", file_path)
        print("Size:", file_size)
        print("Chunks:", total_chunks)

        # Start transfer
        self.client.publish(TOPIC_START, str(file_size), qos=1)

        if not self.wait_ack("READY"):
            raise TimeoutError("ESP32 not ready")

        # Upload chunks
        for i in range(total_chunks):

            chunk = data[i * CHUNK_SIZE : (i + 1) * CHUNK_SIZE]

            payload = struct.pack(">I", i) + chunk

            self.client.publish(TOPIC_CHUNK, payload, qos=1)

            if not self.wait_ack(f"CHUNK:{i}"):
                raise TimeoutError(f"Failed to upload chunk {i}")

            print(f"Progress: {i+1}/{total_chunks}")

        # Finish transfer
        self.client.publish(TOPIC_END, f"{file_size}:{checksum}", qos=1)

        if not self.wait_ack("COMPLETE"):
            raise TimeoutError("Upload incomplete")

        print("File uploaded successfully!")

    def close(self):

        self.client.loop_stop()
        self.client.disconnect()


if __name__ == "__main__":

    uploader = MQTTFileUploader()

    try:

        file_path = input("Enter file path: ").strip().strip('"')

        uploader.upload(file_path)

    finally:

        uploader.close()
