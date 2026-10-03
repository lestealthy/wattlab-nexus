# WattLab Nexus - UI Design

## Design Language

### Colors

| Name | Hex | Usage |
|------|-----|-------|
| Deep Blue | #1E3A5F | Primary background |
| Electric Blue | #00B4FF | Accent, highlights |
| Dark Near-Black | #0A1628 | Background |
| Neutral Gray | #8090A0 | Secondary text |
| White | #FFFFFF | Primary text |
| Success Green | #00C853 | PASS, OK |
| Warning Orange | #FF9800 | WARN |
| Error Red | #FF5252 | FAIL, ERROR |

### Typography

- **Primary Font**: Roboto (or similar sans-serif)
- **Monospace Font**: Roboto Mono (for data, code)
- **Sizes**: 8, 10, 12, 14, 16, 20, 24, 32

### Layout

- **Display**: 480 × 272 pixels
- **Touch Targets**: Minimum 44 × 44 pixels
- **Margins**: 8 pixels minimum
- **Grid**: 8-pixel grid system

## Screen List

### Splash Screen
- WattLab Nexus logo
- Version number
- Boot progress indicator

### Home Screen
- 3×3 grid of main functions
- Status bar (top and bottom)
- Large touch targets

### Main Functions

| Function | Icon | Description |
|----------|------|-------------|
| DISCOVER | Radar | Scan for connected devices |
| ANALYZER | Chart | Protocol analysis |
| CAPTURE | Waveform | Signal capture |
| TERMINAL | Terminal | UART terminal |
| MEASURE | Gauge | Measurements |
| GENERATOR | Signal | Signal generation |
| TEST ENGINE | Checkmark | Automated testing |
| DEVICES | Chip | Device database |
| POWER | Power | Power profiling |
| FILES | Folder | File browser |
| CALIBRATION | Wrench | Calibration |
| SELF TEST | Stethoscope | Diagnostics |
| SETTINGS | Gear | Settings |
| ABOUT | Info | About screen |

## Navigation

- **Back Button**: Top-left corner
- **Home Button**: Top-right corner
- **Status Bar**: Always visible at top
- **Context Actions**: Bottom of screen

## Widgets

### Status Bar
- System state indicator
- Time
- Storage status
- Connection status
- Warnings

### Bottom Bar
- DUT state
- Power state
- Capture state
- Active protocol
- Error indicators

### Common Widgets
- **Button**: Large touch target, clear label
- **Toggle**: On/off switch
- **Slider**: Value adjustment
- **List**: Scrollable list with touch
- **Graph**: Waveform display
- **Gauge**: Circular measurement display
- **Table**: Data display

## Animations

- **Purpose**: Functional only (no decorative animations)
- **Transitions**: 150ms fade or slide
- **Progress**: Indeterminate for unknown duration
- **Feedback**: Immediate visual response to touch

## Accessibility

- High contrast mode
- Large text option
- Color-blind friendly palette
- Touch target size enforcement
