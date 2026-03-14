/**
 * @file camera.h
 * @brief 摄像机类的头文件
 * 
 * 该文件定义了Camera类，实现了一个完整的FPS风格摄像机系统。
 * 支持键盘移动、鼠标旋转和滚轮缩放，提供视图矩阵计算功能。
 * 
 * @author 谢兴国   1452492081@qq.com
 * @version 1.0
 * @date 2025-10-12
 * 
 * @note 基于欧拉角实现，俯仰角限制在±89度以防止万向锁
 */
#ifndef _H_CAMERA_H_
#define _H_CAMERA_H_

#ifdef PLATFORM_ANDROID
    #include <GLES3/gl3.h>
    #include <EGL/egl.h>
#else
    #include <glad/glad.h>
#endif
#include <glm/glm.hpp>


enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// Default camera values
const float YAW         = -90.0f;
const float PITCH       =  0.0f;
const float SPEED       =  2.5f;
const float SENSITIVITY =  0.1f;
const float ZOOM        =  45.0f;


// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
    // camera Attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    // euler Angles
    float Yaw;
    float Pitch;
    // camera options
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    // constructor with vectors
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH) ;//: Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)

    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) ;//: Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)

    glm::mat4 GetViewMatrix();


    void ProcessKeyboard(Camera_Movement direction, float deltaTime);

    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true);
   
    void ProcessMouseScroll(float yoffset);

private:
    void updateCameraVectors();
};
#endif
