pragma Singleton
import QtQuick

QtObject {
    id: theme

    // Colors - Dark Theme Focus
    readonly property color background: "#121212"
    readonly property color surface: "#1e1e1e"
    readonly property color surfaceElevated: "#2d2d2d"
    
    readonly property color primaryText: "#ffffff"
    readonly property color secondaryText: "#a0a0a0"
    
    readonly property color accent: "#3498db"
    readonly property color accentHover: "#2980b9"
    readonly property color accentPressed: "#1c598a"
    
    readonly property color border: "#333333"
    readonly property color borderFocus: "#555555"

    readonly property color danger: "#e74c3c"
    readonly property color success: "#2ecc71"
    readonly property color warning: "#f1c40f"
    
    // Overlays
    readonly property color overlayDark: "#e0000000"
    readonly property color overlayMedium: "#d9000000"
    readonly property color overlay: "#d0000000"
    
    // Spacing
    readonly property int spacingXs: 4
    readonly property int spacingSm: 8
    readonly property int spacingMd: 16
    readonly property int spacingLg: 24
    readonly property int spacingXl: 32

    // Border Radius
    readonly property int radiusSm: 4
    readonly property int radiusMd: 8
    readonly property int radiusLg: 12

    // Controls
    readonly property int controlHeight: 36
    readonly property int iconSizeSm: 16
    readonly property int iconSizeMd: 24
    readonly property int iconSizeLg: 32

    // Typography (fonts)
    readonly property int fontSizeSm: 12
    readonly property int fontSizeMd: 14
    readonly property int fontSizeLg: 18
    readonly property int fontSizeXl: 24
}
