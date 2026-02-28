import cv2
import face_recognition
import urllib.request
import numpy as np
import os
import requests
from datetime import datetime, timedelta

# Paths and configurations
KNOWN_FACES_DIR = "known_faces"
TOLERANCE = 0.6  # Face recognition tolerance
MODEL = "hog"    # Can be "hog" or "cnn" for face detection
WEB_APP_URL = "https://script.google.com/macros/s/AKfycbxmQx7TZM0YPfayyBIIcK5OmaGs4NM_y328xo8WgSw2Q_mexSqmU3j3s6cbtqYS1Bk7mg/exec"  # Replace with your Web App URL
ESP8266_IP_DEFAULT = "http://192.168.165.1/update"  # Default ESP8266 IP
STREAM_URL_DEFAULT = 'http://192.168.28.48:81/stream'  # Default ESP32 stream URL
ESP8266_IP = "http://192.168.165.1/update"  # IP address of your ESP8266
COOLDOWN_TIME = timedelta(seconds=10)  # Cooldown time for sending data

# Ask for IP address and stream URL
print("Do you want to change the ESP8266 IP and Stream URL?")
change_ip = input("Enter 'yes' to change, or press Enter to keep defaults: ").strip().lower()
if change_ip == 'yes':
    ESP8266_IP = input("Enter the new ESP8266 IP address (default: http://192.168.165.1/update): ") or ESP8266_IP_DEFAULT
    STREAM_URL = input("Enter the new Stream URL (default: http://192.168.165.165:81/stream): ") or STREAM_URL_DEFAULT
else:
    ESP8266_IP = ESP8266_IP_DEFAULT
    STREAM_URL = STREAM_URL_DEFAULT

# Load known faces
known_faces = []
known_names = []

def load_known_faces():
    global known_faces, known_names
    known_faces = []
    known_names = []
    for name in os.listdir(KNOWN_FACES_DIR):
        for filename in os.listdir(f"{KNOWN_FACES_DIR}/{name}"):
            image_path = f"{KNOWN_FACES_DIR}/{name}/{filename}"
            image = face_recognition.load_image_file(image_path)
            encodings = face_recognition.face_encodings(image)
            
            # Check if a face encoding exists
            if encodings:
                encoding = encodings[0]  # Take the first detected face
                known_faces.append(encoding)
                known_names.append(name)
            else:
                print(f"Warning: No face found in {image_path}. Skipping this image.")

load_known_faces()  # Initial load

# Camera feed URL
#STREAM_URL = 'http://192.168.165.165:81/stream'  # Replace with your ESP32 stream URL
cv2.namedWindow("Live Transmission", cv2.WINDOW_AUTOSIZE)

# Function to send data to Google Sheets and ESP8266
def send_to_google_sheets_and_esp(name, data_type):
    params = {
        "name": name,
        "type": data_type
    }

    # Send data to Google Sheets
    try:
        response = requests.get(WEB_APP_URL, params=params)
        if response.status_code == 200:
            print(f"Successfully sent {name} data to Google Sheets.")
        else:
            print(f"Failed to send data. HTTP Error: {response.status_code}")
    except Exception as e:
        print(f"Error sending data to Google Sheets: {e}")
    
    # Send data to ESP8266
    try:
        esp_response = requests.get(ESP8266_IP, params=params)
        if esp_response.status_code == 200:
            print(f"Successfully sent {name} data to ESP8266.")
        else:
            print(f"Failed to send data to ESP8266. HTTP Error: {esp_response.status_code}")
    except Exception as e:
        print(f"Error sending data to ESP8266: {e}")

# Dictionary to store last detection time for each face
last_detected = {}

def recognize_faces():
    # Start processing the stream
    stream = urllib.request.urlopen(STREAM_URL)  # Open the stream URL
    buffer = b""  # Initialize buffer

    while True:
        try:
            # Read data from the stream
            buffer += stream.read(1024)
            a = buffer.find(b'\xff\xd8')  # Start of JPEG frame
            b = buffer.find(b'\xff\xd9')  # End of JPEG frame
            
            if a != -1 and b != -1:
                jpg = buffer[a:b + 2]
                buffer = buffer[b + 2:]

                # Decode the JPEG frame
                img = cv2.imdecode(np.frombuffer(jpg, dtype=np.uint8), cv2.IMREAD_COLOR)

                # Convert to RGB
                rgb_img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)

                # Use face_recognition for face locations
                face_locations = face_recognition.face_locations(rgb_img, model=MODEL)
                face_encodings = face_recognition.face_encodings(rgb_img, face_locations)

                current_time = datetime.now()

                for face_encoding, face_location in zip(face_encodings, face_locations):
                    # Match the detected face to known faces
                    matches = face_recognition.compare_faces(known_faces, face_encoding, TOLERANCE)
                    name = "Unknown"

                    if True in matches:
                        match_index = matches.index(True)
                        name = known_names[match_index]

                        # Check cooldown before sending data
                        if name not in last_detected or current_time - last_detected[name] > COOLDOWN_TIME:
                            send_to_google_sheets_and_esp(name, "Face Detection")
                            last_detected[name] = current_time  # Update last detection time

                    # Get bounding box from face_location
                    top, right, bottom, left = face_location
                    cv2.rectangle(img, (left, top), (right, bottom), (0, 255, 0), 2)
                    cv2.putText(img, name, (left, top - 10), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)

                # Display the video feed
                cv2.imshow("Live Transmission", img)

                # Quit the loop on 'q' key
                if cv2.waitKey(5) & 0xFF == ord('q'):
                    break

        except Exception as e:
            print(f"Error reading stream: {e}")
            break

    cv2.destroyAllWindows()

def enroll_faces():
    print("Starting Enrollment Process...")
    name = input("Enter the name of the person: ").strip()

    if not name:
        print("Name cannot be empty. Enrollment canceled.")
        return

    # Create directory for the new person if it doesn't exist
    person_dir = f"{KNOWN_FACES_DIR}/{name}"
    os.makedirs(person_dir, exist_ok=True)

    print("Look at the camera to capture your face...")

    # Start processing the stream
    stream = urllib.request.urlopen(STREAM_URL)  # Open the stream URL
    buffer = b""  # Initialize buffer
    captured = False

    while not captured:
        try:
            buffer += stream.read(1024)
            a = buffer.find(b'\xff\xd8')  # Start of JPEG frame
            b = buffer.find(b'\xff\xd9')  # End of JPEG frame
            
            if a != -1 and b != -1:
                jpg = buffer[a:b + 2]
                buffer = buffer[b + 2:]

                # Decode the JPEG frame
                img = cv2.imdecode(np.frombuffer(jpg, dtype=np.uint8), cv2.IMREAD_COLOR)

                # Convert to RGB
                rgb_img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)

                # Detect face in the frame
                face_locations = face_recognition.face_locations(rgb_img, model=MODEL)

                if face_locations:
                    timestamp = datetime.now().strftime("%Y%m%d%H%M%S")
                    filename = f"{person_dir}/{timestamp}.jpg"
                    cv2.imwrite(filename, img)
                    print(f"Face image saved as {filename}")
                    captured = True
                else:
                    print("No face detected. Please adjust your position.")

                # Show the video feed for guidance
                cv2.imshow("Enrollment", img)

                # Quit the loop on 'q' key
                if cv2.waitKey(5) & 0xFF == ord('q'):
                    print("Enrollment canceled.")
                    break

        except Exception as e:
            print(f"Error during enrollment: {e}")
            break

    stream.close()
    cv2.destroyAllWindows()

    # Reload known faces
    load_known_faces()
    print(f"Enrollment completed for {name}.")

if __name__ == "__main__":
    print("Select Mode:")
    print("1. Enroll New Face")
    print("2. Take Attendance")
    choice = input("Enter your choice (1/2): ")

    if choice == "1":
        enroll_faces()
    elif choice == "2":
        recognize_faces()
    else:
        print("Invalid choice. Exiting.")
