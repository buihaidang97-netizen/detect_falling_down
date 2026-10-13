# AI Fall Detection + ESP32-S3 Emergency Response System (Demo Version)

## 1. Project Overview

This project demonstrates a prototype AI-based fall detection and
emergency response system designed to assist vulnerable individuals such
as elderly people living alone or patients with mobility issues.

The system detects when a person falls using an AI model (YOLO) and
executes an automated emergency handling scenario using ESP32‑S3.

Key functions: - Detect fall events using YOLO - Send detection results
to an API - ESP32 executes emergency scenario - Notify family via
Blynk - Automatically trigger SOS if no response

------------------------------------------------------------------------

# 2. System Architecture

Camera → YOLO AI → API Server → ESP32‑S3 → Blynk → SOS

Components:

  Component    Purpose
  ------------ --------------------
  Camera       Capture video
  YOLO Model   Detect fall
  API Server   Transfer event
  ESP32-S3     Emergency logic
  Microphone   Voice input
  Speaker      Voice output
  Blynk        Send notifications

------------------------------------------------------------------------

# 3. AI Detection

YOLO model processes frames from camera.

If a fall is detected:

``` json
{
  "event":"fall_detected",
  "confidence":0.93,
  "time":"timestamp",
  "image":"frame.jpg"
}
```

This data is sent to the API server.

------------------------------------------------------------------------

# 4. API Server

Role: - Receive AI events - Store data - Provide endpoint for ESP32

Example:

GET /fall-event

Response:

``` json
{
 "fall_detected": true,
 "image_url":"frame.jpg"
}
```

------------------------------------------------------------------------

# 5. ESP32-S3 Controller

ESP32 performs emergency response logic.

Responsibilities: - Poll API - Play voice prompts - Listen for victim
response - Send Blynk alerts - Execute SOS if necessary

------------------------------------------------------------------------

# 6. Hardware

  Device     Function
  ---------- --------------
  ESP32-S3   Controller
  Mic        Voice input
  Speaker    Audio output
  WiFi       Network

------------------------------------------------------------------------

# 7. Scenario Start

Event begins when:

AI → fall detected\
API → store event\
ESP32 → read event

------------------------------------------------------------------------

# 8. LEVEL 1 (0--2 minutes)

Goal: check if victim is conscious.

ESP32 says:

"Bạn có ổn không?"

Possible responses:

  Command         Action
  --------------- --------------
  "Tôi ổn"        cancel alert
  "Không sao"     cancel alert
  "Gọi cấp cứu"   start SOS

If victim confirms safe → system stops.

------------------------------------------------------------------------

# 9. LEVEL 2 (2--5 minutes)

If no response after 2 minutes:

ESP32: - plays local alarm - sends Blynk notifications every 20 seconds

Schedule:

2:00 alert\
2:20 alert\
2:40 alert\
3:00 alert\
3:20 alert\
3:40 alert\
4:00 alert\
4:20 alert\
4:40 alert

------------------------------------------------------------------------

# 10. Family Response

Blynk app provides buttons:

Cancel → stop alert\
SOS → trigger emergency

------------------------------------------------------------------------

# 11. Automatic SOS

If no response within 5 minutes:

ESP32 triggers SOS automatically.

------------------------------------------------------------------------

# 12. SOS Message

Message includes:

-   Emergency notice
-   Address
-   GPS
-   Image evidence

Example:

SOS ALERT Fall detected and user not responding.

------------------------------------------------------------------------

# 13. Timeline

  Time      Action
  --------- --------------------
  0 min     fall detected
  0-2 min   victim interaction
  2-5 min   Blynk alerts
  5 min     SOS

------------------------------------------------------------------------

# 14. State Machine

IDLE → FALL_DETECTED → LEVEL_1 → LEVEL_2 → SOS

------------------------------------------------------------------------

# 15. Demo Scope

This demo includes: - YOLO fall detection - API communication - ESP32
emergency logic - Blynk alerts
