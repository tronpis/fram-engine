#include "Input.h"
#include <algorithm>

namespace FarmEngine {

// Keyboard implementation
Keyboard& Keyboard::getInstance() {
    static Keyboard instance;
    return instance;
}

bool Keyboard::isKeyPressed(KeyCode key) const {
    int index = static_cast<int>(key);
    if (index < 0 || index >= static_cast<int>(currentKeys.size())) {
        return false;
    }
    return currentKeys[index];
}

bool Keyboard::isKeyJustPressed(KeyCode key) const {
    int index = static_cast<int>(key);
    if (index < 0 || index >= static_cast<int>(justPressed.size())) {
        return false;
    }
    return justPressed[index];
}

bool Keyboard::isKeyJustReleased(KeyCode key) const {
    int index = static_cast<int>(key);
    if (index < 0 || index >= static_cast<int>(justReleased.size())) {
        return false;
    }
    return justReleased[index];
}

void Keyboard::setKeyCallback(std::function<void(KeyCode, InputAction)> callback) {
    keyCallback = std::move(callback);
}

void Keyboard::setKeyState(KeyCode key, bool pressed) {
    int index = static_cast<int>(key);
    if (index < 0 || index >= static_cast<int>(currentKeys.size())) {
        return;
    }
    
    bool previousState = currentKeys[index];
    currentKeys[index] = pressed;
    
    if (pressed && !previousState) {
        justPressed[index] = true;
    } else if (!pressed && previousState) {
        justReleased[index] = true;
    }
    
    if (keyCallback) {
        keyCallback(key, pressed ? InputAction::Press : InputAction::Release);
    }
}

void Keyboard::processFrame() {
    // Reset just pressed/released states
    std::fill(justPressed.begin(), justPressed.end(), false);
    std::fill(justReleased.begin(), justReleased.end(), false);
    
    // Update previous state
    previousKeys = currentKeys;
}

// Mouse implementation
Mouse& Mouse::getInstance() {
    static Mouse instance;
    return instance;
}

bool Mouse::isButtonPressed(MouseButton button) const {
    int index = static_cast<int>(button);
    if (index < 0 || index >= static_cast<int>(currentButtons.size())) {
        return false;
    }
    return currentButtons[index];
}

bool Mouse::isButtonJustPressed(MouseButton button) const {
    int index = static_cast<int>(button);
    if (index < 0 || index >= static_cast<int>(justPressed.size())) {
        return false;
    }
    return justPressed[index];
}

bool Mouse::isButtonJustReleased(MouseButton button) const {
    int index = static_cast<int>(button);
    if (index < 0 || index >= static_cast<int>(justReleased.size())) {
        return false;
    }
    return justReleased[index];
}

void Mouse::resetScroll() {
    scrollX = 0.0;
    scrollY = 0.0;
}

void Mouse::setButtonCallback(std::function<void(MouseButton, InputAction)> callback) {
    buttonCallback = std::move(callback);
}

void Mouse::setScrollCallback(std::function<void(double, double)> callback) {
    scrollCallback = std::move(callback);
}

void Mouse::setMotionCallback(std::function<void(double, double)> callback) {
    motionCallback = std::move(callback);
}

void Mouse::setButtonState(MouseButton button, bool pressed) {
    int index = static_cast<int>(button);
    if (index < 0 || index >= static_cast<int>(currentButtons.size())) {
        return;
    }
    
    bool previousState = currentButtons[index];
    currentButtons[index] = pressed;
    
    if (pressed && !previousState) {
        justPressed[index] = true;
    } else if (!pressed && previousState) {
        justReleased[index] = true;
    }
    
    if (buttonCallback) {
        buttonCallback(button, pressed ? InputAction::Press : InputAction::Release);
    }
}

void Mouse::setPosition(double x, double y) {
    posX = x;
    posY = y;
    delta.x = static_cast<float>(x - prevX);
    delta.y = static_cast<float>(y - prevY);
    prevX = x;
    prevY = y;
    
    if (motionCallback) {
        motionCallback(x, y);
    }
}

void Mouse::setScroll(double x, double y) {
    scrollX = x;
    scrollY = y;
    
    if (scrollCallback) {
        scrollCallback(x, y);
    }
}

void Mouse::processFrame() {
    // Reset just pressed/released states
    std::fill(justPressed.begin(), justPressed.end(), false);
    std::fill(justReleased.begin(), justReleased.end(), false);
    
    // Update previous state
    previousButtons = currentButtons;
    
    // Reset delta
    delta = glm::vec2(0.0f, 0.0f);
}

// Gamepad implementation
Gamepad& Gamepad::getInstance() {
    static Gamepad instance;
    return instance;
}

bool Gamepad::isButtonPressed(Button::Type button) const {
    int index = static_cast<int>(button);
    if (index < 0 || index >= static_cast<int>(buttons.size())) {
        return false;
    }
    return buttons[index];
}

float Gamepad::getAxis(Axis::Type axis) const {
    int index = static_cast<int>(axis);
    if (index < 0 || index >= static_cast<int>(axes.size())) {
        return 0.0f;
    }
    return axes[index];
}

void Gamepad::setConnectionCallback(std::function<void(bool)> callback) {
    connectionCallback = std::move(callback);
}

void Gamepad::setConnected(bool connected) {
    bool wasConnected = this->connected;
    this->connected = connected;
    
    if (connectionCallback && wasConnected != connected) {
        connectionCallback(connected);
    }
}

void Gamepad::setButtonState(Button::Type button, bool pressed) {
    int index = static_cast<int>(button);
    if (index < 0 || index >= static_cast<int>(buttons.size())) {
        return;
    }
    buttons[index] = pressed;
}

void Gamepad::setAxisState(Axis::Type axis, float value) {
    int index = static_cast<int>(axis);
    if (index < 0 || index >= static_cast<int>(axes.size())) {
        return;
    }
    axes[index] = value;
}

// InputManager implementation
InputManager& InputManager::getInstance() {
    static InputManager instance;
    return instance;
}

void InputManager::initialize() {
    // Initialize keyboard
    auto& keyboard = Keyboard::getInstance();
    
    // Initialize mouse
    auto& mouse = Mouse::getInstance();
    
    // Initialize gamepad
    auto& gamepad = Gamepad::getInstance();
}

void InputManager::cleanup() {
    // Cleanup if needed
}

void InputManager::update() {
    Keyboard::getInstance().processFrame();
    Mouse::getInstance().processFrame();
}

void InputManager::setInputMode(InputMode mode) {
    currentMode = mode;
}

void InputManager::bindAction(const std::string& actionName, KeyCode key) {
    actionBindings[actionName].keys.push_back(key);
}

void InputManager::bindAction(const std::string& actionName, MouseButton button) {
    actionBindings[actionName].buttons.push_back(button);
}

bool InputManager::isActionPressed(const std::string& actionName) const {
    auto it = actionBindings.find(actionName);
    if (it == actionBindings.end()) {
        return false;
    }
    
    const auto& binding = it->second;
    
    // Check keys
    for (KeyCode key : binding.keys) {
        if (Keyboard::getInstance().isKeyPressed(key)) {
            return true;
        }
    }
    
    // Check buttons
    for (MouseButton button : binding.buttons) {
        if (Mouse::getInstance().isButtonPressed(button)) {
            return true;
        }
    }
    
    return false;
}

void InputManager::bindAxis(const std::string& axisName, KeyCode positive, KeyCode negative) {
    axisBindings[axisName] = {positive, negative};
}

float InputManager::getAxisValue(const std::string& axisName) const {
    auto it = axisBindings.find(axisName);
    if (it == axisBindings.end()) {
        return 0.0f;
    }
    
    const auto& binding = it->second;
    float value = 0.0f;
    
    if (Keyboard::getInstance().isKeyPressed(binding.positive)) {
        value += 1.0f;
    }
    
    if (Keyboard::getInstance().isKeyPressed(binding.negative)) {
        value -= 1.0f;
    }
    
    return value;
}

void InputManager::setVibration(float leftMotor, float rightMotor, float duration) {
    // TODO: Implement gamepad vibration
    (void)leftMotor;
    (void)rightMotor;
    (void)duration;
}

InputManager::~InputManager() = default;

} // namespace FarmEngine
