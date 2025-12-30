### 1. `line_following_main.ino` - Primary Control System
- **Function**: Main line following with integrated odometry and 90° turn detection
- **Key Features**:
  - Dual IR sensor line tracking with LM324 operational amplifiers
  - Hybrid encoder system: digital interrupt (left) + analog polling (right)
  - 510cm autonomous stop with 2-second pause
  - State-based turn detection (gentle correction vs aggressive pivot turns)
  - Real-time LCD display (elapsed time + distance traveled)
  - Non-blocking timing for responsive control

### 2. `obstacle_avoidance.ino` - Ultrasonic Safety System
- **Function**: HC-SR04 based obstacle detection and avoidance
- **Key Features**:
  - 7cm obstacle detection threshold
  - 3-state finite state machine (detect → stop → turn)
  - 2-second stop duration with LCD countdown display
  - 600ms sharp left turn avoidance maneuver
  - Temperature-compensated distance calculation
