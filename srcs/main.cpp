#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cmath>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Particle.hpp"
#include "Shader.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


glm::vec3 camera_pos = glm::vec3(0.0f, 2.0f, 10.0f);
glm::vec3 camera_front = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 camera_up = glm::vec3(0.0f, 1.0f, 0.0f);

float delta_time = 0.0f; // Time between current frame and last frame
float last_frame = 0.0f; // Time of last frame

float yaw = -90.0f, pitch = 0.0f;
float last_X = 400, last_Y = 300;
bool first_mouse = true;
float fov = 45.0f;

const int LAT_SEGMENTS = 10;
const int LON_SEGMENTS = 20;
const float PI = 3.14159265359f;

void process_input(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    float current_frame = glfwGetTime();
    delta_time = current_frame - last_frame;
    last_frame = current_frame;
    float camera_speed = 2.5f * delta_time;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera_pos += camera_speed * camera_front;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera_pos -= camera_speed * camera_front;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera_pos -= glm::normalize(glm::cross(camera_front, camera_up)) * camera_speed;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera_pos += glm::normalize(glm::cross(camera_front, camera_up)) * camera_speed;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        camera_pos += camera_speed * camera_up;
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        camera_pos -= camera_speed * camera_up;
    }
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (first_mouse) {
        last_X = xpos;
        last_Y = ypos;
        first_mouse = false;
    }

    float xoffset = xpos - last_X;
    float yoffset = last_Y - ypos;
    last_X = xpos;
    last_Y = ypos;

    const float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;
    if (pitch > 89.0f) {
        pitch = 89.0f;
    }
    if (pitch < -89.0f) {
        pitch = -89.0f;
    }

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw) * cos(glm::radians(pitch)));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    camera_front = glm::normalize(direction);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    fov -= (float)yoffset;
    if (fov < 1.0f) {
        fov = 1.0f;
    }
    if (fov > 45.0f) {
        fov = 45.0f;
    }
}

unsigned int load_texture(char const * path, bool repeat) {
    unsigned int texture_ID;
    glGenTextures(1, &texture_ID);

    int width, height, nr_components;
    unsigned char *data = stbi_load(path, &width, &height, &nr_components, 0);
    if (data) {
        GLenum format;
        if (nr_components == 1) {
            format = GL_RED;
        } else if (nr_components == 3) {
            format = GL_RGB;
        } else if (nr_components == 4) {
            format = GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, texture_ID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        if (repeat) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        } else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    } else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return texture_ID;
}

unsigned int load_cubemap(std::vector<std::string> faces) {
    unsigned int texture_ID;
    glGenTextures(1, &texture_ID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture_ID);

    int width, height, nr_channels;
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nr_channels, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        } else {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return texture_ID;
}

std::vector<float> generate_particle_vertices(float radius) {
    std::vector<float> vertices;

    for (int lat = 0; lat <= LAT_SEGMENTS; ++lat) {
        int fix_lat = lat - int(LAT_SEGMENTS / 2);
        float phi = fix_lat * PI / LAT_SEGMENTS;
        float sin_phi = sin(phi);
        float cos_phi = cos(phi);

        for (int lon = 0; lon <= LON_SEGMENTS; ++lon) {
            float theta = lon * 2 * PI / LON_SEGMENTS;
            float sin_theta = sin(theta);
            float cos_theta = cos(theta);

            float x = radius * cos_phi * cos_theta;
            float y = radius * sin_phi;
            float z = radius * cos_phi * sin_theta;

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        }
    }
    return vertices;
}

int main(int argc, char *argv[]) {
    // Initialize window
    if (!glfwInit()) {
        std::cout << "Error: Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    int window_w = 1920;
    int window_h = 1080;
    std::string window_title = "ImpactX";
    GLFWwindow* window = glfwCreateWindow(window_w, window_h, window_title.c_str(), NULL, NULL);
    if (window == NULL) {
        std::cout << "Error: Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Error: Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return -1;
    }

    Shader particle_shader("../shaders/particle.vs", "../shaders/particle.fs");
    Shader space_box_shader("../shaders/space_box.vs", "../shaders/space_box.fs");

    float space_box_vertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f,  1.0f
    };

    float particle_radius = 0.02f;
    std::vector<float> particle_vertices = generate_particle_vertices(particle_radius);
    glm::vec3 center_pos_1(0.0f);
    glm::vec3 center_pos_2(3.0f, 3.0f, 3.7f);
    float planet_radius = 0.7f;
    int particle_num_1 = 50000;
    int particle_num_2 = 50000;
    int particle_num = particle_num_1 + particle_num_2;
    glm::vec3 initial_velocity_1 = glm::vec3(0.25f);
    glm::vec3 initial_velocity_2 = glm::vec3(-0.25f);
    float mass = 1.0f;
    int threads = 256;
    Particle particles(center_pos_1, center_pos_2, planet_radius, particle_num_1,
        particle_num_2, initial_velocity_1, initial_velocity_2, mass, particle_radius, threads);

    // Initialize window
    glViewport(0, 0, window_w, window_h);

    // particle VAO
    unsigned int particle_VAO, particle_VBO;
    glGenVertexArrays(1, &particle_VAO);
    glGenBuffers(1, &particle_VBO);

    // Store instance data in an array buffer
    unsigned int instance_VBO;
    glGenBuffers(1, &instance_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, instance_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * particle_num, particles.get_particle_position().data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    unsigned int instance_color_VBO;
    glGenBuffers(1, &instance_color_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, instance_color_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * particle_num, particles.get_particle_color().data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(particle_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, particle_VBO);
    glBufferData(GL_ARRAY_BUFFER, particle_vertices.size() * sizeof(float), particle_vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Particle instance data
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, instance_VBO);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexAttribDivisor(1, 1); // Tell OpenGL this is per-instance data

    // set instance data(color)
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, instance_color_VBO);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexAttribDivisor(2, 1); // tell OpenGL this is an instanced vertex attribute
    glBindVertexArray(0);

    // Space box VAO
    unsigned int skybox_VAO, skybox_VBO;
    glGenVertexArrays(1, &skybox_VAO);
    glGenBuffers(1, &skybox_VBO);
    glBindVertexArray(skybox_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, skybox_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(space_box_vertices), &space_box_vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);

    unsigned int cubeTexture = load_texture("../textures/awesomeface.png", true);

    std::vector<std::string> faces
    {
        "../textures/space_box/right.png",
        "../textures/space_box/left.png",
        "../textures/space_box/top.png",
        "../textures/space_box/bottom.png",
        "../textures/space_box/front.png",
        "../textures/space_box/back.png"
    };
    unsigned int cubemap_texture = load_cubemap(faces);

    double last_time = glfwGetTime();
    double fps_last_time = glfwGetTime();
    int frame_num = 0;
    space_box_shader.use();
    space_box_shader.setInt("spacebox", 0);
    while (!glfwWindowShouldClose(window)) {
        process_input(window);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        double current_time = glfwGetTime();
        double delta = current_time - last_time;
        particles.update_particle(delta);
        last_time = glfwGetTime();

        glBindBuffer(GL_ARRAY_BUFFER, instance_VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * particle_num, particles.get_particle_position().data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ARRAY_BUFFER, instance_color_VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * particle_num, particles.get_particle_color().data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // particle
        particle_shader.use();
        glm::mat4 view = glm::lookAt(camera_pos, camera_pos + camera_front, camera_up);
        glm::mat4 projection = glm::perspective(glm::radians(fov), float(window_w) / float(window_h), 0.1f, 100.0f);
        glBindVertexArray(particle_VAO);
        unsigned int particle_view = glGetUniformLocation(particle_shader.ID, "view");
        unsigned int particle_proj = glGetUniformLocation(particle_shader.ID, "projection");
        glUniformMatrix4fv(particle_view, 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(particle_proj, 1, GL_FALSE, &projection[0][0]);
        glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, particle_vertices.size() / 3, particle_num);

        // Draw skybox as last
        glDepthFunc(GL_LEQUAL);
        space_box_shader.use();
        view = glm::mat3(glm::lookAt(camera_pos, camera_pos + camera_front, camera_up)); // Remove translation from the view matrix
        unsigned int viewLoc = glGetUniformLocation(space_box_shader.ID, "view");
        unsigned int projLoc = glGetUniformLocation(space_box_shader.ID, "projection");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);
        // Skybox cube
        glBindVertexArray(skybox_VAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_texture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LEQUAL);

        glfwSwapBuffers(window);
        glfwPollEvents();

        // Measure FPS
        double fps_current_time = glfwGetTime();
        double fps_delta = fps_current_time - fps_last_time;
        frame_num += 1;
        if (fps_delta >= 1.0) {
            int fps = int(double(frame_num) / fps_delta);
            std::stringstream ss;
            ss << window_title.c_str() << " [" << fps << " FPS]";
            glfwSetWindowTitle(window, ss.str().c_str());
            frame_num = 0;
            fps_last_time = fps_current_time;
        }
    }

    glfwTerminate();
    return 0;
}
