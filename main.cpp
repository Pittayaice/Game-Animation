#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/animator.h>
#include <learnopengl/model_animation.h>

#include <iostream>

// Callback declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// Window
const unsigned int SCR_WIDTH = 1000;
const unsigned int SCR_HEIGHT = 700;

// Camera
Camera camera(glm::vec3(0.0f, 2.0f, 6.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Animation & Model
Animator* animator;
Animation* idleAnim;
Animation* walkAnim;
Animation* leftTurnAnim;
Animation* rightTurnAnim;
Animation* jumpAnim;
Animation* danceAnim;
Model* ourModel;

// Transform control
glm::vec3 modelPosition = glm::vec3(0.0f, -0.5f, 0.0f);
float modelRotation = 0.0f;
float moveSpeed = 2.0f;

// Animation state system
enum AnimationState {
    IDLE,
    WALKING,
    TURNING_LEFT,
    TURNING_RIGHT,
    JUMPING,
    DANCING
};

AnimationState currentState = IDLE;
Animation* currentAnim = nullptr;

// Turn animation control
float turnStartRotation = 0.0f;
float turnTargetRotation = 0.0f;
float turnProgress = 0.0f;
float turnDuration = 0.5f;

// Key press detection (prevent holding)
bool wasAPressed = false;
bool wasDPressed = false;
bool wasSpacePressed = false;
bool was1Pressed = false;

// Helper: switch animation safely
void switchAnimation(Animation* newAnim)
{
    if (animator && newAnim && newAnim != currentAnim)
    {
        animator->PlayAnimation(newAnim);
        currentAnim = newAnim;
    }
}

// Move forward in facing direction
void moveForward(float speed)
{
    modelPosition.x += sin(modelRotation) * speed;
    modelPosition.z += cos(modelRotation) * speed;
}

int main()
{
    // Initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Human Animation Control", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Load GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    stbi_set_flip_vertically_on_load(true);
    glEnable(GL_DEPTH_TEST);

    // Shader
    Shader ourShader("anim_model.vs", "anim_model.fs");

    // Load model and animations
    ourModel = new Model(FileSystem::getPath("resources/objects/human/Rumba Dancing.dae"));
    idleAnim = new Animation(FileSystem::getPath("resources/objects/human/Idle.dae"), ourModel);
    walkAnim = new Animation(FileSystem::getPath("resources/objects/human/Walking.dae"), ourModel);
    leftTurnAnim = new Animation(FileSystem::getPath("resources/objects/human/Left Turn.dae"), ourModel);
    rightTurnAnim = new Animation(FileSystem::getPath("resources/objects/human/Right Turn.dae"), ourModel);
    jumpAnim = new Animation(FileSystem::getPath("resources/objects/human/Forward Jump.dae"), ourModel);
    danceAnim = new Animation(FileSystem::getPath("resources/objects/human/Rumba Dancing.dae"), ourModel);

    // Start with idle
    animator = new Animator(idleAnim);
    currentAnim = idleAnim;
    currentState = IDLE;

    // Main render loop
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        animator->UpdateAnimation(deltaTime);

        glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        auto transforms = animator->GetFinalBoneMatrices();
        for (int i = 0; i < transforms.size(); ++i)
            ourShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, modelPosition);
        model = glm::rotate(model, modelRotation, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.5f));
        ourShader.setMat4("model", model);

        ourModel->Draw(ourShader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    delete animator;
    delete idleAnim;
    delete walkAnim;
    delete leftTurnAnim;
    delete rightTurnAnim;
    delete jumpAnim;
    delete danceAnim;
    delete ourModel;

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Get current key states
    bool aPressed = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
    bool dPressed = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
    bool spacePressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    bool onePressed = glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS;
    bool wPressed = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;

    // === TURN LEFT (A) - Single press ===
    if (aPressed && !wasAPressed && currentState != TURNING_LEFT && currentState != TURNING_RIGHT)
    {
        currentState = TURNING_LEFT;
        switchAnimation(leftTurnAnim);
        turnStartRotation = modelRotation;
        turnTargetRotation = modelRotation - glm::radians(90.0f);
        turnProgress = 0.0f;
    }
    wasAPressed = aPressed;

    // === TURN RIGHT (D) - Single press ===
    if (dPressed && !wasDPressed && currentState != TURNING_LEFT && currentState != TURNING_RIGHT)
    {
        currentState = TURNING_RIGHT;
        switchAnimation(rightTurnAnim);
        turnStartRotation = modelRotation;
        turnTargetRotation = modelRotation + glm::radians(90.0f);
        turnProgress = 0.0f;
    }
    wasDPressed = dPressed;

    // === UPDATE TURN ANIMATION ===
    if (currentState == TURNING_LEFT || currentState == TURNING_RIGHT)
    {
        turnProgress += deltaTime / turnDuration;

        if (turnProgress >= 1.0f)
        {
            // Turn complete
            modelRotation = turnTargetRotation;
            currentState = IDLE;
            switchAnimation(idleAnim);
            turnProgress = 0.0f;
        }
        else
        {
            // Smooth interpolation (ease-in-out)
            float t = turnProgress;
            t = t * t * (3.0f - 2.0f * t); // smoothstep
            modelRotation = turnStartRotation + (turnTargetRotation - turnStartRotation) * t;
        }
        return; // Block other inputs while turning
    }

    // === WALK FORWARD (W) ===
    if (wPressed)
    {
        moveForward(moveSpeed * deltaTime);
        if (currentState != WALKING && currentState != DANCING)
        {
            currentState = WALKING;
            switchAnimation(walkAnim);
        }
    }
    else if (currentState == WALKING)
    {
        currentState = IDLE;
        switchAnimation(idleAnim);
    }

    // === JUMP (Space) - Single press ===
    if (spacePressed && !wasSpacePressed && currentState != JUMPING && currentState != DANCING)
    {
        currentState = JUMPING;
        switchAnimation(jumpAnim);
        // Auto-return to idle after jump duration
    }
    wasSpacePressed = spacePressed;

    // === DANCE (1) - Toggle ===
    if (onePressed && !was1Pressed)
    {
        if (currentState != DANCING)
        {
            currentState = DANCING;
            switchAnimation(danceAnim);
        }
        else
        {
            currentState = IDLE;
            switchAnimation(idleAnim);
        }
    }
    was1Pressed = onePressed;

    // === RETURN TO IDLE ===
    if (currentState == JUMPING)
    {
        // Check if jump animation might be done (simple timer)
        static float jumpTimer = 0.0f;
        jumpTimer += deltaTime;
        if (jumpTimer > 1.0f) // Adjust based on your jump animation length
        {
            currentState = IDLE;
            switchAnimation(idleAnim);
            jumpTimer = 0.0f;
        }
    }

    // Auto-idle when no input (except dancing)
    if (currentState != DANCING &&
        currentState != TURNING_LEFT &&
        currentState != TURNING_RIGHT &&
        currentState != JUMPING &&
        !wPressed)
    {
        if (currentState != IDLE)
        {
            currentState = IDLE;
            switchAnimation(idleAnim);
        }
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}