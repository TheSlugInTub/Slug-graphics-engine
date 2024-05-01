#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "stb_image.h"
#include "stb_truetype.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Window.h"
#include "ShadowConfiguration.h"
#include "Shader.h"
#include "model.h"
#include "CameraClass.h"
#include <iostream>

void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window);
unsigned int loadTexture(char const* path);
void renderObject(Model OurModel, glm::mat4 model, Shader DefaultShader, unsigned int path);
void renderShadows(Model OurModel, glm::mat4 model, Shader ShadowShader, ShadowMapping shadowMapping);

const unsigned int SCR_WIDTH = 800 * 2;
const unsigned int SCR_HEIGHT = 600 * 2;

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

float near_plane = 1.0f;
float far_plane = 25.0f;

glm::vec3 lightPos = glm::vec3(0.0f, 2.0f, 0.0f);

int main()
{
    GLFWwindow* window = initializeWindow("SlugEngine", SCR_WIDTH, SCR_HEIGHT);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwSetCursorPosCallback(window, mouse_callback);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    Shader DefaultShader("vertex.shad", "fragment.shad");
    Shader ShadowShader("shadowvertex.shad", "shadowfragment.shad", "shadowcalculations.shad");
    unsigned int popCat = loadTexture("Slugarius.png");
    unsigned int woodTexture = loadTexture("wood.png");

    Model OurModel("cube.obj");
    Model OurSphere("ball.obj");

    ShadowMapping shadowMapping(800, 800);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(2.0f, -4.0f, 0.0));
        model = glm::scale(model, glm::vec3(10.0f, 1.0f, 10.0f));

        glm::mat4 model2 = glm::mat4(1.0f);
        model2 = glm::translate(model2, glm::vec3(5.0f, -2.0f, 1.0));
        model2 = glm::scale(model2, glm::vec3(1.0f, 1.0f, 1.0f));
        ShadowShader.setMat4("model", model2);

        renderShadows(OurModel, model, ShadowShader, shadowMapping);
        renderShadows(OurSphere, model2, ShadowShader, shadowMapping);

        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        DefaultShader.use();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        DefaultShader.setMat4("projection", projection);
        DefaultShader.setMat4("view", view);
        // set lighting uniforms
        DefaultShader.setVec3("lightPos", lightPos);
        DefaultShader.setVec3("viewPos", camera.Position);
        DefaultShader.setInt("shadows", true); // enable/disable shadows by pressing 'SPACE'
        DefaultShader.setFloat("far_plane", far_plane);
        DefaultShader.setFloat("lightIntensity", 1.5f);

        renderObject(OurModel, model, DefaultShader, woodTexture);
        renderObject(OurSphere, model2, DefaultShader, popCat);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

unsigned int loadTexture(char const* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

void renderShadows(Model OurModel, glm::mat4 model, Shader ShadowShader, ShadowMapping shadowMapping)
{
    ShadowShader.setMat4("model", model);
    shadowMapping.CreateDepthCubemap(lightPos, near_plane, far_plane);
    shadowMapping.RenderDepthCubemap(ShadowShader, OurModel);

    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void renderObject(Model OurModel, glm::mat4 model, Shader DefaultShader, unsigned int path)
{
    DefaultShader.setMat4("model", model);
    DefaultShader.setTexture2D("diffuseTexture", path, 0);
    OurModel.Draw(DefaultShader);
}