//-----------------------------------------------------------------------------
// Copyright (c) 2006-2008 dhpoware. All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#if !defined(CAMERA_H)
#define CAMERA_H

#include <d3dx9.h>

//-----------------------------------------------------------------------------
// A general purpose 6DoF (six degrees of freedom) vector based camera.
//
// This camera class supports 2 different behaviors:
// first person mode and flight mode.
//
// First person mode only allows 5DOF (x axis movement, y axis movement, z axis
// movement, yaw, and pitch) and movement is always parallel to the world x-z
// (ground) plane.
//
// Flight mode supports 6DoF. This is the camera class' default behavior.
//
// This camera class allows the camera to be moved in 2 ways: using fixed
// step world units, and using a supplied velocity and acceleration. The former
// simply moves the camera by the specified amount. To move the camera in this
// way call one of the move() methods. The other way to move the camera
// calculates the camera's displacement based on the supplied velocity,
// acceleration, and elapsed time. To move the camera in this way call the
// updatePosition() method.
//-----------------------------------------------------------------------------

class Camera
{
public:
    enum CameraBehavior
    {
        CAMERA_BEHAVIOR_FIRST_PERSON,
        CAMERA_BEHAVIOR_FLIGHT
    };

    Camera();
    ~Camera();

    void lookAt(const D3DXVECTOR3 &target);
    void lookAt(const D3DXVECTOR3 &eye, const D3DXVECTOR3 &target, const D3DXVECTOR3 &up);
    void move(float dx, float dy, float dz);
    void move(const D3DXVECTOR3 &direction, const D3DXVECTOR3 &amount);
    void perspective(float fovx, float aspect, float znear, float zfar);
    void rotate(float headingDegrees, float pitchDegrees, float rollDegrees);
    void rotateSmoothly(float headingDegrees, float pitchDegrees, float rollDegrees);
    void updatePosition(const D3DXVECTOR3 &direction, float elapsedTimeSec);

    // Getter methods.

    const D3DXVECTOR3 &getAcceleration() const;
    CameraBehavior getBehavior() const;
    const D3DXVECTOR3 &getCurrentVelocity() const;
    const D3DXVECTOR3 &getPosition() const;
    float getRotationSpeed() const;
    const D3DXMATRIX &getProjectionMatrix() const;
    const D3DXVECTOR3 &getVelocity() const;
    const D3DXVECTOR3 &getViewDirection() const;
    const D3DXMATRIX &getViewMatrix() const;
    const D3DXVECTOR3 &getXAxis() const;
    const D3DXVECTOR3 &getYAxis() const;
    const D3DXVECTOR3 &getZAxis() const;
    
    // Setter methods.

    void setAcceleration(const D3DXVECTOR3 &acceleration);
    void setAcceleration(float x, float y, float z);
    void setBehavior(CameraBehavior behavior);
    void setCurrentVelocity(const D3DXVECTOR3 &currentVelocity);
    void setCurrentVelocity(float x, float y, float z);
    void setPosition(const D3DXVECTOR3 &eye);
    void setPosition(float x, float y, float z);
    void setRotationSpeed(float rotationSpeed);
    void setVelocity(const D3DXVECTOR3 &velocity);
    void setVelocity(float x, float y, float z);
        
private:
    void rotateFirstPerson(float headingDegrees, float pitchDegrees);
    void rotateFlight(float headingDegrees, float pitchDegrees, float rollDegrees);
    void updateVelocity(const D3DXVECTOR3 &direction, float elapsedTimeSec);
    void updateViewMatrix(bool orthogonalizeAxes);
    
    static const float DEFAULT_ROTATION_SPEED;
    static const float DEFAULT_FOVX;   
    static const float DEFAULT_ZFAR;
    static const float DEFAULT_ZNEAR;
    static const D3DXVECTOR3 WORLD_XAXIS;
    static const D3DXVECTOR3 WORLD_YAXIS;
    static const D3DXVECTOR3 WORLD_ZAXIS;

    CameraBehavior m_behavior;
    float m_accumPitchDegrees;
    float m_rotationSpeed;
    float m_fovx;
    float m_aspectRatio;
    float m_znear;
    float m_zfar;
    D3DXVECTOR3 m_eye;
    D3DXVECTOR3 m_xAxis;
    D3DXVECTOR3 m_yAxis;
    D3DXVECTOR3 m_zAxis;
    D3DXVECTOR3 m_viewDir;
    D3DXVECTOR3 m_acceleration;
    D3DXVECTOR3 m_currentVelocity;
    D3DXVECTOR3 m_velocity;
    D3DXMATRIX m_viewMatrix;
    D3DXMATRIX m_projMatrix;
};

//-----------------------------------------------------------------------------

inline const D3DXVECTOR3 &Camera::getAcceleration() const
{ return m_acceleration; }

inline Camera::CameraBehavior Camera::getBehavior() const
{ return m_behavior; }

inline const D3DXVECTOR3 &Camera::getCurrentVelocity() const
{ return m_currentVelocity; }

inline const D3DXVECTOR3 &Camera::getPosition() const
{ return m_eye; }

inline float Camera::getRotationSpeed() const
{ return m_rotationSpeed; }

inline const D3DXMATRIX &Camera::getProjectionMatrix() const
{ return m_projMatrix; }

inline const D3DXVECTOR3 &Camera::getVelocity() const
{ return m_velocity; }

inline const D3DXVECTOR3 &Camera::getViewDirection() const
{ return m_viewDir; }

inline const D3DXMATRIX &Camera::getViewMatrix() const
{ return m_viewMatrix; }

inline const D3DXVECTOR3 &Camera::getXAxis() const
{ return m_xAxis; }

inline const D3DXVECTOR3 &Camera::getYAxis() const
{ return m_yAxis; }

inline const D3DXVECTOR3 &Camera::getZAxis() const
{ return m_zAxis; }

#endif