#pragma once

#include <vector>
#include <string>
#include <functional>
#include <glm/glm.hpp>

namespace FarmEngine {

enum class KeyCode {
    Unknown = -1,
    Space = 32,
    Apostrophe = 39,
    Comma = 44,
    Minus = 45,
    Period = 46,
    Slash = 47,
    D0 = 48,
    D1, D2, D3, D4, D5, D6, D7, D8, D9,
    Semicolon = 59,
    Equal = 61,
    A = 65, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    LeftBracket = 91,
    Backslash = 92,
    RightBracket = 93,
    GraveAccent = 96,
    Escape = 256,
    Enter, Tab, Backspace, Insert, Delete,
    Right, Left, Down, Up,
    PageUp, PageDown, Home, End,
    CapsLock = 280,
    ScrollLock, NumLock, PrintScreen, Pause,
    F1 = 290, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    LeftShift = 340, LeftControl, LeftAlt, LeftSuper,
    RightShift, RightControl, RightAlt, RightSuper, Menu
};

enum class MouseButton {
    Button0 = 0,
    Button1 = 1,
    Button2 = 2,
    Button3 = 3,
    Button4 = 4,
    Button5 = 5,
    ButtonLast = Button5,
    Left = Button0,
    Right = Button1,
    Middle = Button2
};

enum class InputAction {
    Press,
    Release,
    Repeat
};

enum class InputMode {
    Normal,         // Cursor visible y responde normalmente
    Hidden,         // Cursor oculto pero input normal
    Disabled,       // Cursor capturado, solo movimiento relativo
    Locked          // Cursor capturado en el centro
};

class Keyboard {
public:
    static Keyboard& getInstance();
    
    bool isKeyPressed(KeyCode key) const;
    bool isKeyJustPressed(KeyCode key) const;
    bool isKeyJustReleased(KeyCode key) const;
    
    void setKeyCallback(std::function<void(KeyCode, InputAction)> callback);
    
    // Estado interno (llamado por Window)
    void setKeyState(KeyCode key, bool pressed);
    void processFrame();
    
private:
    Keyboard() = default;
    
    std::vector<bool> currentKeys;
    std::vector<bool> previousKeys;
    std::vector<bool> justPressed;
    std::vector<bool> justReleased;
    
    std::function<void(KeyCode, InputAction)> keyCallback;
};

class Mouse {
public:
    static Mouse& getInstance();
    
    // Posición
    double getX() const { return posX; }
    double getY() const { return posY; }
    glm::vec2 getPosition() const { return {static_cast<float>(posX), static_cast<float>(posY)}; }
    
    // Botones
    bool isButtonPressed(MouseButton button) const;
    bool isButtonJustPressed(MouseButton button) const;
    bool isButtonJustReleased(MouseButton button) const;
    
    // Scroll
    double getScrollX() const { return scrollX; }
    double getScrollY() const { return scrollY; }
    void resetScroll();
    
    // Movimiento relativo (para cámaras FPS)
    glm::vec2 getDelta() const { return delta; }
    
    // Callbacks
    void setButtonCallback(std::function<void(MouseButton, InputAction)> callback);
    void setScrollCallback(std::function<void(double, double)> callback);
    void setMotionCallback(std::function<void(double, double)> callback);
    
    // Estado interno
    void setButtonState(MouseButton button, bool pressed);
    void setPosition(double x, double y);
    void setScroll(double x, double y);
    void processFrame();
    
private:
    Mouse() = default;
    
    double posX = 0.0, posY = 0.0;
    double prevX = 0.0, prevY = 0.0;
    glm::vec2 delta{0.0f, 0.0f};
    
    std::vector<bool> currentButtons;
    std::vector<bool> previousButtons;
    std::vector<bool> justPressed;
    std::vector<bool> justReleased;
    
    double scrollX = 0.0, scrollY = 0.0;
    
    std::function<void(MouseButton, InputAction)> buttonCallback;
    std::function<void(double, double)> scrollCallback;
    std::function<void(double, double)> motionCallback;
};

class Gamepad {
public:
    struct Axis {
        enum Type {
            LeftX, LeftY, RightX, RightY,
            LeftTrigger, RightTrigger,
            Count
        };
    };
    
    struct Button {
        enum Type {
            A, B, X, Y,
            LeftBumper, RightBumper,
            Back, Start, Guide,
            LeftStick, RightStick,
            DPadUp, DPadDown, DPadLeft, DPadRight,
            Count
        };
    };
    
    static Gamepad& getInstance();
    
    bool isConnected() const { return connected; }
    bool isButtonPressed(Button::Type button) const;
    float getAxis(Axis::Type axis) const;
    
    void setConnectionCallback(std::function<void(bool)> callback);
    
    // Estado interno
    void setConnected(bool connected);
    void setButtonState(Button::Type button, bool pressed);
    void setAxisState(Axis::Type axis, float value);
    
private:
    Gamepad() = default;
    
    bool connected = false;
    std::vector<bool> buttons;
    std::vector<float> axes;
    
    std::function<void(bool)> connectionCallback;
};

class InputManager {
public:
    static InputManager& getInstance();
    
    void initialize();
    void cleanup();
    
    void update();
    
    // Modos de input
    void setInputMode(InputMode mode);
    InputMode getInputMode() const { return currentMode; }
    
    // Acciones mapeadas (input mapping system)
    void bindAction(const std::string& actionName, KeyCode key);
    void bindAction(const std::string& actionName, MouseButton button);
    bool isActionPressed(const std::string& actionName) const;
    
    // Ejes mapeados
    void bindAxis(const std::string& axisName, KeyCode positive, KeyCode negative);
    float getAxisValue(const std::string& axisName) const;
    
    // Vibración (gamepad)
    void setVibration(float leftMotor, float rightMotor, float duration);
    
private:
    InputManager() = default;
    ~InputManager();
    
    InputMode currentMode = InputMode::Normal;
    
    struct ActionBinding {
        std::vector<KeyCode> keys;
        std::vector<MouseButton> buttons;
    };
    
    struct AxisBinding {
        KeyCode positive = KeyCode::Unknown;
        KeyCode negative = KeyCode::Unknown;
    };
    
    std::unordered_map<std::string, ActionBinding> actionBindings;
    std::unordered_map<std::string, AxisBinding> axisBindings;
};

} // namespace FarmEngine
