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

#include <cmath>
#include "camera.h"

const float Camera::DEFAULT_ROTATION_SPEED = 0.3f;
const float Camera::DEFAULT_FOVX = 90.0f;
const float Camera::DEFAULT_ZNEAR = 0.1f;
const float Camera::DEFAULT_ZFAR = 1000.0f;

const D3DXVECTOR3 Camera::WORLD_XAXIS(1.0f, 0.0f, 0.0f);
const D3DXVECTOR3 Camera::WORLD_YAXIS(0.0f, 1.0f, 0.0f);
const D3DXVECTOR3 Camera::WORLD_ZAXIS(0.0f, 0.0f, 1.0f);

Camera::Camera()
{
    m_behavior = CAMERA_BEHAVIOR_FLIGHT;
    
    m_accumPitchDegrees = 0.0f;
    m_rotationSpeed = DEFAULT_ROTATION_SPEED;
    m_fovx = DEFAULT_FOVX;
    m_aspectRatio = 0.0f;
    m_znear = DEFAULT_ZNEAR;
    m_zfar = DEFAULT_ZFAR;
    
    m_eye = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    m_xAxis = D3DXVECTOR3(1.0f, 0.0f, 0.0f);
    m_yAxis = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
    m_zAxis = D3DXVECTOR3(0.0f, 0.0f, 1.0f);
    m_viewDir = D3DXVECTOR3(0.0f, 0.0f, 1.0f);
    
    m_acceleration = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    m_currentVelocity = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    m_velocity = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    
    D3DXMatrixIdentity(&m_viewMatrix);
    D3DXMatrixIdentity(&m_projMatrix);
}

Camera::~Camera()
{
}

void Camera::lookAt(const D3DXVECTOR3 &target)
{
    lookAt(m_eye, target, m_yAxis);
}

void Camera::lookAt(const D3DXVECTOR3 &eye, const D3DXVECTOR3 &target, const D3DXVECTOR3 &up)
{
    m_eye = eye;

    m_zAxis = target - eye;
    D3DXVec3Normalize(&m_zAxis, &m_zAxis);

    m_viewDir = m_zAxis;

    D3DXVec3Cross(&m_xAxis, &up, &m_zAxis);
    D3DXVec3Normalize(&m_xAxis, &m_xAxis);

    D3DXVec3Cross(&m_yAxis, &m_zAxis, &m_xAxis);
    D3DXVec3Normalize(&m_yAxis, &m_yAxis);
    D3DXVec3Normalize(&m_xAxis, &m_xAxis);

    D3DXMatrixIdentity(&m_viewMatrix);

    m_viewMatrix(0,0) = m_xAxis.x;
    m_viewMatrix(1,0) = m_xAxis.y;
    m_viewMatrix(2,0) = m_xAxis.z;
    m_viewMatrix(3,0) = -D3DXVec3Dot(&m_xAxis, &eye);

    m_viewMatrix(0,1) = m_yAxis.x;
    m_viewMatrix(1,1) = m_yAxis.y;
    m_viewMatrix(2,1) = m_yAxis.z;
    m_viewMatrix(3,1) = -D3DXVec3Dot(&m_yAxis, &eye);

    m_viewMatrix(0,2) = m_zAxis.x;
    m_viewMatrix(1,2) = m_zAxis.y;
    m_viewMatrix(2,2) = m_zAxis.z;
    m_viewMatrix(3,2) = -D3DXVec3Dot(&m_zAxis, &eye);

    // Extract the pitch angle from the view matrix.
    m_accumPitchDegrees = D3DXToDegree(-asinf(m_viewMatrix(1,2)));
}

void Camera::move(float dx, float dy, float dz)
{
    // Moves the camera by dx world units to the left or right; dy
    // world units upwards or downwards; and dz world units forwards
    // or backwards.

    D3DXVECTOR3 eye = m_eye;
    D3DXVECTOR3 forwards;

    if (m_behavior == CAMERA_BEHAVIOR_FIRST_PERSON)
    {
        // Calculate the forwards direction. Can't just use the camera's local
        // z axis as doing so will cause the camera to move more slowly as the
        // camera's view approaches 90 degrees straight up and down.

        D3DXVec3Cross(&forwards, &m_xAxis, &WORLD_YAXIS);
        D3DXVec3Normalize(&forwards, &forwards);
    }
    else
    {
        forwards = m_viewDir;
    }

    eye += m_xAxis * dx;
    eye += WORLD_YAXIS * dy;
    eye += forwards * dz;

    setPosition(eye);
}

void Camera::move(const D3DXVECTOR3 &direction, const D3DXVECTOR3 &amount)
{
    // Moves the camera by the specified amount of world units in the specified
    // direction in world space.

    m_eye.x += direction.x * amount.x;
    m_eye.y += direction.y * amount.y;
    m_eye.z += direction.z * amount.z;

    updateViewMatrix(false);
}

void Camera::perspective(float fovx, float aspect, float znear, float zfar)
{
    // Construct a projection matrix based on the horizontal field of view
    // 'fovx' rather than the more traditional vertical field of view 'fovy'.

    float e = 1.0f / tanf(D3DXToRadian(fovx) / 2.0f);
    float aspectInv = 1.0f / aspect;
    float fovy = 2.0f * atanf(aspectInv / e);
    float xScale = 1.0f / tanf(0.5f * fovy);
    float yScale = xScale / aspectInv;

    m_projMatrix(0,0) = xScale;
    m_projMatrix(1,0) = 0.0f;
    m_projMatrix(2,0) = 0.0f;
    m_projMatrix(3,0) = 0.0f;

    m_projMatrix(0,1) = 0.0f;
    m_projMatrix(1,1) = yScale;
    m_projMatrix(2,1) = 0.0f;
    m_projMatrix(3,1) = 0.0f;

    m_projMatrix(0,2) = 0.0f;
    m_projMatrix(1,2) = 0.0f;
    m_projMatrix(2,2) = zfar / (zfar - znear);
    m_projMatrix(3,2) = -znear * zfar / (zfar - znear);

    m_projMatrix(0,3) = 0.0f;
    m_projMatrix(1,3) = 0.0f;
    m_projMatrix(2,3) = 1.0f;
    m_projMatrix(3,3) = 0.0f;

    m_fovx = fovx;
    m_aspectRatio = aspect;
    m_znear = znear;
    m_zfar = zfar;
}

void Camera::rotate(float headingDegrees, float pitchDegrees, float rollDegrees)
{
    // Rotates the camera based on its current behavior.
    // Note that not all behaviors support rolling.
    //
    // This Camera class follows the left-hand rotation rule.
    // Angles are measured clockwise when looking along the rotation
    // axis toward the origin. Since the Z axis is pointing into the
    // screen we need to negate rolls.

    rollDegrees = -rollDegrees;

    switch (m_behavior)
    {
    default:
        break;

    case CAMERA_BEHAVIOR_FIRST_PERSON:
        rotateFirstPerson(headingDegrees, pitchDegrees);
        break;

    case CAMERA_BEHAVIOR_FLIGHT:
        rotateFlight(headingDegrees, pitchDegrees, rollDegrees);
        break;
    }

    updateViewMatrix(true);
}

void Camera::rotateSmoothly(float headingDegrees, float pitchDegrees, float rollDegrees)
{
    // This method applies a scaling factor to the rotation angles prior to
    // using these rotation angles to rotate the camera. This method is usually
    // called when the camera is being rotated using an input device (such as a
    // mouse or a joystick). 

    headingDegrees *= m_rotationSpeed;
    pitchDegrees *= m_rotationSpeed;
    rollDegrees *= m_rotationSpeed;

    rotate(headingDegrees, pitchDegrees, rollDegrees);
}

void Camera::updatePosition(const D3DXVECTOR3 &direction, float elapsedTimeSec)
{
    // Moves the camera using Newton's second law of motion. Unit mass is
    // assumed here to somewhat simplify the calculations. The direction vector
    // is in the range [-1,1].

    if (D3DXVec3LengthSq(&m_currentVelocity) != 0.0f)
    {
        // Only move the camera if the velocity vector is not of zero length.
        // Doing this guards against the camera slowly creeping around due to
        // floating point rounding errors.

        D3DXVECTOR3 displacement = (m_currentVelocity * elapsedTimeSec) +
            (0.5f * m_acceleration * elapsedTimeSec * elapsedTimeSec);

        // Floating point rounding errors will slowly accumulate and cause the
        // camera to move along each axis. To prevent any unintended movement
        // the displacement vector is clamped to zero for each direction that
        // the camera isn't moving in. Note that the updateVelocity() method
        // will slowly decelerate the camera's velocity back to a stationary
        // state when the camera is no longer moving along that direction. To
        // account for this the camera's current velocity is also checked.

        if (direction.x == 0.0f && fabsf(m_currentVelocity.x) < 1e-6f)
            displacement.x = 0.0f;

        if (direction.y == 0.0f && fabsf(m_currentVelocity.y) < 1e-6f)
            displacement.y = 0.0f;

        if (direction.z == 0.0f && fabsf(m_currentVelocity.z) < 1e-6f)
            displacement.z = 0.0f;

        move(displacement.x, displacement.y, displacement.z);
    }

    // Continuously update the camera's velocity vector even if the camera
    // hasn't moved during this call. When the camera is no longer being moved
    // the camera is decelerating back to its stationary state.

    updateVelocity(direction, elapsedTimeSec);
}

void Camera::setAcceleration(const D3DXVECTOR3 &acceleration)
{
    m_acceleration = acceleration;
}

void Camera::setAcceleration(float x, float y, float z)
{
    m_acceleration.x = x;
    m_acceleration.y = y;
    m_acceleration.z = z;
}

void Camera::setBehavior(CameraBehavior behavior)
{
    if (m_behavior == CAMERA_BEHAVIOR_FLIGHT && behavior == CAMERA_BEHAVIOR_FIRST_PERSON)
    {
        // Moving from flight behavior to first person behavior.
        // Need to ignore camera roll, but retain existing pitch and heading.

        lookAt(m_eye, m_eye + m_zAxis, WORLD_YAXIS);
    }

    m_behavior = behavior;
}

void Camera::setCurrentVelocity(const D3DXVECTOR3 &currentVelocity)
{
    m_currentVelocity = currentVelocity;
}

void Camera::setCurrentVelocity(float x, float y, float z)
{
    m_currentVelocity.x = x;
    m_currentVelocity.y = y;
    m_currentVelocity.z = z;
}

void Camera::setPosition(const D3DXVECTOR3 &eye)
{
    m_eye = eye;

    updateViewMatrix(false);
}

void Camera::setPosition(float x, float y, float z)
{
    m_eye.x = x;
    m_eye.y = y;
    m_eye.z = z;

    updateViewMatrix(false);
}

void Camera::setRotationSpeed(float rotationSpeed)
{
    m_rotationSpeed = rotationSpeed;
}

void Camera::setVelocity(const D3DXVECTOR3 &velocity)
{
    m_velocity = velocity;
}

void Camera::setVelocity(float x, float y, float z)
{
    m_velocity.x = x;
    m_velocity.y = y;
    m_velocity.z = z;
}

void Camera::rotateFirstPerson(float headingDegrees, float pitchDegrees)
{
    m_accumPitchDegrees += pitchDegrees;

    if (m_accumPitchDegrees > 90.0f)
    {
        pitchDegrees = 90.0f - (m_accumPitchDegrees - pitchDegrees);
        m_accumPitchDegrees = 90.0f;
    }

    if (m_accumPitchDegrees < -90.0f)
    {
        pitchDegrees = -90.0f - (m_accumPitchDegrees - pitchDegrees);
        m_accumPitchDegrees = -90.0f;
    }

    float heading = D3DXToRadian(headingDegrees);
    float pitch = D3DXToRadian(pitchDegrees);
    
    D3DXMATRIX rotMtx;
    D3DXVECTOR4 result;

    // Rotate camera's existing x and z axes about the world y axis.
    if (heading != 0.0f)
    {
        D3DXMatrixRotationY(&rotMtx, heading);
        
        D3DXVec3Transform(&result, &m_xAxis, &rotMtx);
        m_xAxis = D3DXVECTOR3(result.x, result.y, result.z);

        D3DXVec3Transform(&result, &m_zAxis, &rotMtx);
        m_zAxis = D3DXVECTOR3(result.x, result.y, result.z);
    }

    // Rotate camera's existing y and z axes about its existing x axis.
    if (pitch != 0.0f)
    {
        D3DXMatrixRotationAxis(&rotMtx, &m_xAxis, pitch);
        
        D3DXVec3Transform(&result, &m_yAxis, &rotMtx);
        m_yAxis = D3DXVECTOR3(result.x, result.y, result.z);
        
        D3DXVec3Transform(&result, &m_zAxis, &rotMtx);
        m_zAxis = D3DXVECTOR3(result.x, result.y, result.z);
    }
}

void Camera::rotateFlight(float headingDegrees, float pitchDegrees, float rollDegrees)
{
    float heading = D3DXToRadian(headingDegrees);
    float pitch = D3DXToRadian(pitchDegrees);
    float roll = D3DXToRadian(rollDegrees);

    D3DXMATRIX rotMtx;
    D3DXVECTOR4 result;

    // Rotate camera's existing x and z axes about its existing y axis.
    if (heading != 0.0f)
    {
        D3DXMatrixRotationAxis(&rotMtx, &m_yAxis, heading);
        
        D3DXVec3Transform(&result, &m_xAxis, &rotMtx);
        m_xAxis = D3DXVECTOR3(result.x, result.y, result.z);
        
        D3DXVec3Transform(&result, &m_zAxis, &rotMtx);
        m_zAxis = D3DXVECTOR3(result.x, result.y, result.z);
    }

    // Rotate camera's existing y and z axes about its existing x axis.
    if (pitch != 0.0f)
    {
        D3DXMatrixRotationAxis(&rotMtx, &m_xAxis, pitch);

        D3DXVec3Transform(&result, &m_yAxis, &rotMtx);
        m_yAxis = D3DXVECTOR3(result.x, result.y, result.z);
        
        D3DXVec3Transform(&result, &m_zAxis, &rotMtx);
        m_zAxis = D3DXVECTOR3(result.x, result.y, result.z);
    }

    // Rotate camera's existing x and y axes about its existing z axis.
    if (roll != 0.0f)
    {
        D3DXMatrixRotationAxis(&rotMtx, &m_zAxis, roll);
        
        D3DXVec3Transform(&result, &m_xAxis, &rotMtx);
        m_xAxis = D3DXVECTOR3(result.x, result.y, result.z);
        
        D3DXVec3Transform(&result, &m_yAxis, &rotMtx);
        m_yAxis = D3DXVECTOR3(result.x, result.y, result.z);
    }
}

void Camera::updateVelocity(const D3DXVECTOR3 &direction, float elapsedTimeSec)
{
    // Updates the camera's velocity based on the supplied movement direction
    // and the elapsed time (since this method was last called). The movement
    // direction is the in the range [-1,1].

    if (direction.x != 0.0f)
    {
        // Camera is moving along the x axis.
        // Linearly accelerate up to the camera's max speed.

        m_currentVelocity.x += direction.x * m_acceleration.x * elapsedTimeSec;

        if (m_currentVelocity.x > m_velocity.x)
            m_currentVelocity.x = m_velocity.x;
        else if (m_currentVelocity.x < -m_velocity.x)
            m_currentVelocity.x = -m_velocity.x;
    }
    else
    {
        // Camera is no longer moving along the x axis.
        // Linearly decelerate back to stationary state.

        if (m_currentVelocity.x > 0.0f)
        {
            if ((m_currentVelocity.x -= m_acceleration.x * elapsedTimeSec) < 0.0f)
                m_currentVelocity.x = 0.0f;
        }
        else
        {
            if ((m_currentVelocity.x += m_acceleration.x * elapsedTimeSec) > 0.0f)
                m_currentVelocity.x = 0.0f;
        }
    }

    if (direction.y != 0.0f)
    {
        // Camera is moving along the y axis.
        // Linearly accelerate up to the camera's max speed.

        m_currentVelocity.y += direction.y * m_acceleration.y * elapsedTimeSec;

        if (m_currentVelocity.y > m_velocity.y)
            m_currentVelocity.y = m_velocity.y;
        else if (m_currentVelocity.y < -m_velocity.y)
            m_currentVelocity.y = -m_velocity.y;
    }
    else
    {
        // Camera is no longer moving along the y axis.
        // Linearly decelerate back to stationary state.

        if (m_currentVelocity.y > 0.0f)
        {
            if ((m_currentVelocity.y -= m_acceleration.y * elapsedTimeSec) < 0.0f)
                m_currentVelocity.y = 0.0f;
        }
        else
        {
            if ((m_currentVelocity.y += m_acceleration.y * elapsedTimeSec) > 0.0f)
                m_currentVelocity.y = 0.0f;
        }
    }

    if (direction.z != 0.0f)
    {
        // Camera is moving along the z axis.
        // Linearly accelerate up to the camera's max speed.

        m_currentVelocity.z += direction.z * m_acceleration.z * elapsedTimeSec;

        if (m_currentVelocity.z > m_velocity.z)
            m_currentVelocity.z = m_velocity.z;
        else if (m_currentVelocity.z < -m_velocity.z)
            m_currentVelocity.z = -m_velocity.z;
    }
    else
    {
        // Camera is no longer moving along the z axis.
        // Linearly decelerate back to stationary state.

        if (m_currentVelocity.z > 0.0f)
        {
            if ((m_currentVelocity.z -= m_acceleration.z * elapsedTimeSec) < 0.0f)
                m_currentVelocity.z = 0.0f;
        }
        else
        {
            if ((m_currentVelocity.z += m_acceleration.z * elapsedTimeSec) > 0.0f)
                m_currentVelocity.z = 0.0f;
        }
    }
}

void Camera::updateViewMatrix(bool orthogonalizeAxes)
{
    if (orthogonalizeAxes)
    {
        // Regenerate the camera's local axes to orthogonalize them.
        
        D3DXVec3Normalize(&m_zAxis, &m_zAxis);
        
        D3DXVec3Cross(&m_yAxis, &m_zAxis, &m_xAxis);
        D3DXVec3Normalize(&m_yAxis, &m_yAxis);
        
        D3DXVec3Cross(&m_xAxis, &m_yAxis, &m_zAxis);
        D3DXVec3Normalize(&m_xAxis, &m_xAxis);

        m_viewDir = m_zAxis;
    }

    // Reconstruct the view matrix.

    m_viewMatrix(0,0) = m_xAxis.x;
    m_viewMatrix(1,0) = m_xAxis.y;
    m_viewMatrix(2,0) = m_xAxis.z;
    m_viewMatrix(3,0) = -D3DXVec3Dot(&m_xAxis, &m_eye);

    m_viewMatrix(0,1) = m_yAxis.x;
    m_viewMatrix(1,1) = m_yAxis.y;
    m_viewMatrix(2,1) = m_yAxis.z;
    m_viewMatrix(3,1) = -D3DXVec3Dot(&m_yAxis, &m_eye);

    m_viewMatrix(0,2) = m_zAxis.x;
    m_viewMatrix(1,2) = m_zAxis.y;
    m_viewMatrix(2,2) = m_zAxis.z;
    m_viewMatrix(3,2) = -D3DXVec3Dot(&m_zAxis, &m_eye);

    m_viewMatrix(0,3) = 0.0f;
    m_viewMatrix(1,3) = 0.0f;
    m_viewMatrix(2,3) = 0.0f;
    m_viewMatrix(3,3) = 1.0f;
}