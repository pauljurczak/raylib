/*******************************************************************************************
*
*   raylib Camera System - Camera Modes Setup and Control Functions
*
*   #define CAMERA_IMPLEMENTATION
*       Generates the implementation of the library into the included file.
*       If not defined, the library is in header only mode and can be included in other headers 
*       or source files without problems. But only ONE file should hold the implementation.
*
*   #define CAMERA_STANDALONE
*       If defined, the library can be used as standalone as a camera system but some
*       functions must be redefined to manage inputs accordingly.
*
*   NOTE: Memory footprint of this library is aproximately 112 bytes
*
*   Initial design by Marc Palau (2014)
*   Reviewed by Ramon Santamaria (2015-2016)
*
*   This software is provided "as-is", without any express or implied warranty. In no event
*   will the authors be held liable for any damages arising from the use of this software.
*
*   Permission is granted to anyone to use this software for any purpose, including commercial
*   applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*     1. The origin of this software must not be misrepresented; you must not claim that you
*     wrote the original software. If you use this software in a product, an acknowledgment
*     in the product documentation would be appreciated but is not required.
*
*     2. Altered source versions must be plainly marked as such, and must not be misrepresented
*     as being the original software.
*
*     3. This notice may not be removed or altered from any source distribution.
*
**********************************************************************************************/

#ifndef CAMERA_H
#define CAMERA_H

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Types and Structures Definition
// NOTE: Below types are required for CAMERA_STANDALONE usage
//----------------------------------------------------------------------------------
#if defined(CAMERA_STANDALONE)
    // Camera modes
    typedef enum { CAMERA_CUSTOM = 0, CAMERA_FREE, CAMERA_ORBITAL, CAMERA_FIRST_PERSON, CAMERA_THIRD_PERSON } CameraMode;

    // Vector2 type
    typedef struct Vector2 {
        float x;
        float y;
    } Vector2;

    // Vector3 type
    typedef struct Vector3 {
        float x;
        float y;
        float z;
    } Vector3;

    // Camera type, defines a camera position/orientation in 3d space
    typedef struct Camera {
        Vector3 position;
        Vector3 target;
        Vector3 up;
    } Camera;
#endif

#ifdef __cplusplus
extern "C" {            // Prevents name mangling of functions
#endif

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
#if defined(CAMERA_STANDALONE)
void SetCameraMode(int mode);                               // Set camera mode (multiple camera modes available)
void UpdateCamera(Camera *camera);                          // Update camera (player position is ignored)
void UpdateCameraPlayer(Camera *camera, Vector3 *position); // Update camera and player position (1st person and 3rd person cameras)

void SetCameraPosition(Vector3 position);                   // Set internal camera position
void SetCameraTarget(Vector3 target);                       // Set internal camera target
void SetCameraFovy(float fovy);                             // Set internal camera field-of-view-y

void SetCameraPanControl(int panKey);                       // Set camera pan key to combine with mouse movement (free camera)
void SetCameraAltControl(int altKey);                       // Set camera alt key to combine with mouse movement (free camera)
void SetCameraSmoothZoomControl(int szKey);                 // Set camera smooth zoom key to combine with mouse (free camera)

void SetCameraMoveControls(int frontKey, int backKey, 
                           int leftKey, int rightKey, 
                           int upKey, int downKey);         // Set camera move controls (1st person and 3rd person cameras)
void SetCameraMouseSensitivity(float sensitivity);          // Set camera mouse sensitivity (1st person and 3rd person cameras)
#endif

#ifdef __cplusplus
}
#endif

#endif // CAMERA_H


/***********************************************************************************
*
*   CAMERA IMPLEMENTATION
*
************************************************************************************/

#if defined(CAMERA_IMPLEMENTATION)

#include <math.h>               // Required for: sqrt(), sin(), cos()

#ifndef PI
    #define PI 3.14159265358979323846
#endif

#ifndef DEG2RAD
    #define DEG2RAD (PI/180.0f)
#endif

#ifndef RAD2DEG
    #define RAD2DEG (180.0f/PI)
#endif

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
// CAMERA_GENERIC
#define CAMERA_SCROLL_SENSITIVITY                       1.5f

// FREE_CAMERA
#define CAMERA_FREE_MOUSE_SENSITIVITY                   0.01f
#define CAMERA_FREE_DISTANCE_MIN_CLAMP                  0.3f
#define CAMERA_FREE_DISTANCE_MAX_CLAMP                  120.0f
#define CAMERA_FREE_MIN_CLAMP                           85.0f
#define CAMERA_FREE_MAX_CLAMP                          -85.0f
#define CAMERA_FREE_SMOOTH_ZOOM_SENSITIVITY             0.05f
#define CAMERA_FREE_PANNING_DIVIDER                     5.1f

// ORBITAL_CAMERA
#define CAMERA_ORBITAL_SPEED                            0.01f

// FIRST_PERSON
//#define CAMERA_FIRST_PERSON_MOUSE_SENSITIVITY           0.003f
#define CAMERA_FIRST_PERSON_FOCUS_DISTANCE              25.0f
#define CAMERA_FIRST_PERSON_MIN_CLAMP                   85.0f
#define CAMERA_FIRST_PERSON_MAX_CLAMP                  -85.0f

#define CAMERA_FIRST_PERSON_STEP_TRIGONOMETRIC_DIVIDER  5.0f
#define CAMERA_FIRST_PERSON_STEP_DIVIDER                30.0f
#define CAMERA_FIRST_PERSON_WAVING_DIVIDER              200.0f

#define CAMERA_FIRST_PERSON_HEIGHT_RELATIVE_EYES_POSITION  0.85f

// THIRD_PERSON
//#define CAMERA_THIRD_PERSON_MOUSE_SENSITIVITY           0.003f
#define CAMERA_THIRD_PERSON_DISTANCE_CLAMP              1.2f
#define CAMERA_THIRD_PERSON_MIN_CLAMP                   5.0f
#define CAMERA_THIRD_PERSON_MAX_CLAMP                  -85.0f
#define CAMERA_THIRD_PERSON_OFFSET                      (Vector3){ 0.4f, 0.0f, 0.0f }

// PLAYER (used by camera)
#define PLAYER_WIDTH                        0.4f
#define PLAYER_HEIGHT                       0.9f
#define PLAYER_DEPTH                        0.4f
#define PLAYER_MOVEMENT_DIVIDER             20.0f

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
// Camera move modes (first person and third person cameras)
typedef enum { MOVE_FRONT = 0, MOVE_LEFT, MOVE_BACK, MOVE_RIGHT, MOVE_UP, MOVE_DOWN } CameraMove;

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
static Vector2 cameraAngle = { 0.0f, 0.0f };
static float cameraTargetDistance = 5.0f;             // TODO: Remove! Use predefined camera->target to camera->position distance
static Vector2 cameraMousePosition = { 0.0f, 0.0f };
static Vector2 cameraMouseVariation = { 0.0f, 0.0f };

static int cameraMoveControl[6]  = { 'W', 'A', 'S', 'D', 'E', 'Q' };
static int cameraPanControlKey = 2;                   // raylib: MOUSE_MIDDLE_BUTTON
static int cameraAltControlKey = 342;                 // raylib: KEY_LEFT_ALT
static int cameraSmoothZoomControlKey = 341;          // raylib: KEY_LEFT_CONTROL

static int cameraMoveCounter = 0;                     // Used for 1st person swinging movement
static float cameraMouseSensitivity = 0.003f;         // How sensible is camera movement to mouse movement

static int cameraMode = CAMERA_CUSTOM;                // Current internal camera mode

//----------------------------------------------------------------------------------
// Module specific Functions Declaration
//----------------------------------------------------------------------------------
#if defined(CAMERA_STANDALONE)
// NOTE: Camera controls depend on some raylib input functions
// TODO: Set your own input functions (used in ProcessCamera())
static Vector2 GetMousePosition() { return (Vector2){ 0.0f, 0.0f }; }
static void SetMousePosition(Vector2 pos) {} 
static int IsMouseButtonDown(int button) { return 0;}
static int GetMouseWheelMove() { return 0; }
static int GetScreenWidth() { return 1280; }
static int GetScreenHeight() { return 720; }
static void ShowCursor() {}
static void HideCursor() {}
static int IsKeyDown(int key) { return 0; }
#endif

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------

// Select camera mode (multiple camera modes available)
// TODO: Review hardcoded values when changing modes...
void SetCameraMode(int mode)
{
    if ((cameraMode == CAMERA_FIRST_PERSON) && (mode == CAMERA_FREE))
    {
        cameraTargetDistance = 5.0f;        // TODO: Review hardcode!
        cameraAngle.y = -40*DEG2RAD;
    }
    else if ((cameraMode == CAMERA_FIRST_PERSON) && (mode == CAMERA_ORBITAL))
    {
        cameraTargetDistance = 5.0f;        // TODO: Review hardcode!
        cameraAngle.y = -40*DEG2RAD;
    }
    else if ((cameraMode == CAMERA_CUSTOM) && (mode == CAMERA_FREE))
    {
        cameraTargetDistance = 10.0f;       // TODO: Review hardcode!
        cameraAngle.x = 45*DEG2RAD;
        cameraAngle.y = -40*DEG2RAD;
        
        ShowCursor();
    }
    else if ((cameraMode == CAMERA_CUSTOM) && (mode == CAMERA_ORBITAL))
    {
        //cameraTargetDistance = 10.0f;     // TODO: Review hardcode!
        cameraAngle.x = 225*DEG2RAD;
        cameraAngle.y = -40*DEG2RAD;
    }
    
    /*
    Vector3 v1 = internalCamera.position;
    Vector3 v2 = internalCamera.target;
    
    float dx = v2.x - v1.x;
    float dy = v2.y - v1.y;
    float dz = v2.z - v1.z;
    
    cameraTargetDistance = sqrt(dx*dx + dy*dy + dz*dz);
    */
    
    cameraMode = mode;
}

// Update camera depending on selected mode
// NOTE: Camera controls depend on some raylib functions:
//       Mouse: GetMousePosition(), SetMousePosition(), IsMouseButtonDown(), GetMouseWheelMove()
//       System: GetScreenWidth(), GetScreenHeight(), ShowCursor(), HideCursor()
//       Keys:  IsKeyDown()
void UpdateCamera(Camera *camera)
{
    /*
    if (cameraMode != CAMERA_CUSTOM)
    {
        
    }
    */
    // Mouse movement detection
    Vector2 mousePosition = GetMousePosition();
    int mouseWheelMove = GetMouseWheelMove();
    int panKey = IsMouseButtonDown(cameraPanControlKey);    // bool value
    
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    if ((cameraMode != CAMERA_FREE) && (cameraMode != CAMERA_ORBITAL))
    {
        HideCursor();

        if (mousePosition.x < screenHeight/3) SetMousePosition((Vector2){ screenWidth - screenHeight/3, mousePosition.y});
        else if (mousePosition.y < screenHeight/3) SetMousePosition((Vector2){ mousePosition.x, screenHeight - screenHeight/3});
        else if (mousePosition.x > screenWidth - screenHeight/3) SetMousePosition((Vector2) { screenHeight/3, mousePosition.y});
        else if (mousePosition.y > screenHeight - screenHeight/3) SetMousePosition((Vector2){ mousePosition.x, screenHeight/3});
        else
        {
            cameraMouseVariation.x = mousePosition.x - cameraMousePosition.x;
            cameraMouseVariation.y = mousePosition.y - cameraMousePosition.y;
        }
    }
    else
    {
        ShowCursor();

        cameraMouseVariation.x = mousePosition.x - cameraMousePosition.x;
        cameraMouseVariation.y = mousePosition.y - cameraMousePosition.y;
    }

	// NOTE: We GetMousePosition() again because it can be modified by a previous SetMousePosition() call
	// If using directly mousePosition variable we have problems on CAMERA_FIRST_PERSON and CAMERA_THIRD_PERSON
    cameraMousePosition = GetMousePosition();

    // Support for multiple automatic camera modes
    switch (cameraMode)
    {
        case CAMERA_FREE:
        {
            // Camera zoom
            if ((cameraTargetDistance < CAMERA_FREE_DISTANCE_MAX_CLAMP) && (mouseWheelMove < 0))
            {
                cameraTargetDistance -= (mouseWheelMove*CAMERA_SCROLL_SENSITIVITY);

                if (cameraTargetDistance > CAMERA_FREE_DISTANCE_MAX_CLAMP) cameraTargetDistance = CAMERA_FREE_DISTANCE_MAX_CLAMP;
            }
            // Camera looking down
            else if ((camera->position.y > camera->target.y) && (cameraTargetDistance == CAMERA_FREE_DISTANCE_MAX_CLAMP) && (mouseWheelMove < 0))
            {
                camera->target.x += mouseWheelMove*(camera->target.x - camera->position.x)*CAMERA_SCROLL_SENSITIVITY/cameraTargetDistance;
                camera->target.y += mouseWheelMove*(camera->target.y - camera->position.y)*CAMERA_SCROLL_SENSITIVITY/cameraTargetDistance;
                camera->target.z += mouseWheelMove*(camera->target.z - camera->position.z)*CAMERA_SCROLL_SENSITIVITY/cameraTargetDistance;
            }
            else if ((camera->position.y > camera->target.y) && (camera->target.y >= 0))
            {
                camera->target.x += mouseWheelMove*(camera->target.x - camera->position.x)*CAMERA_SCROLL_SENSITIVITY/cameraTargetDistance;
                camera->target.y += mouseWheelMove*(camera->target.y - camera->position.y)*CAMERA_SCROLL_SENSITIVITY/cameraTargetDistance;
                camera->target.z += mouseWheelMove*(camera->target.z - camera->position.z)*CAMERA_SCROLL_SENSITIVITY/cameraTargetDistance;

                // if (camera->target.y < 0) camera->target.y = -0.001;
            }
            else if ((camera->position.y > camera->target.y) && (camera->target.y < 0) && (mouseWheelMove > 0))
            {
                cameraTargetDistance -= (mouseWheelMove*CAMERA_SCROLL_SENSITIVITY);
                if (cameraTargetDistance < CAMERA_FREE_DISTANCE_MIN_CLAMP) cameraTargetDistance = CAMERA_FREE_DISTANCE_MIN_CLAMP;
            }
            // Camera looking up
            else if ((camera->position.y < camera->target.y) && (cameraTargetDistance == CAMERA_FREE_DISTANCE_MAX_CLAMP) && (mouseWheelMove < 0))
            {
                camera->target.x += mouseWheelMove*(camera->target.x - camera->position.x)*CAMERA_SCROLL_SENSITIVITY/cameraTargetDistance;
                camera->target.y += mouseWheelMove*(camera->target.y - camera->position.y)*CAMERA_SCROLL_SENSITIVITY/cameraTargetDistance;
                camera->target.z += mouseWheelMove*(camera->target.z - camera->position.z)*CAMERA_SCROLL_SENSITIVITY/cameraTargetDistance;
            }
            else if ((camera->position.y < camera->target.y) && (camera->target.y <= 0))
            {
                camera->target.x += mouseWheelMove*(camera->target.x - camera->position.x)*CAMERA_SCROLL_SENSITIVITY/cameraTargetDistance;
                camera->target.y += mouseWheelMove*(camera->target.y - camera->position.y)*CAMERA_SCROLL_SENSITIVITY/cameraTargetDistance;
                camera->target.z += mouseWheelMove*(camera->target.z - camera->position.z)*CAMERA_SCROLL_SENSITIVITY/cameraTargetDistance;

                // if (camera->target.y > 0) camera->target.y = 0.001;
            }
            else if ((camera->position.y < camera->target.y) && (camera->target.y > 0) && (mouseWheelMove > 0))
            {
                cameraTargetDistance -= (mouseWheelMove*CAMERA_SCROLL_SENSITIVITY);
                if (cameraTargetDistance < CAMERA_FREE_DISTANCE_MIN_CLAMP) cameraTargetDistance = CAMERA_FREE_DISTANCE_MIN_CLAMP;
            }

            // Inputs
            if (IsKeyDown(cameraAltControlKey))
            {
                if (IsKeyDown(cameraSmoothZoomControlKey))
                {
                    // Camera smooth zoom
                    if (panKey) cameraTargetDistance += (cameraMouseVariation.y*CAMERA_FREE_SMOOTH_ZOOM_SENSITIVITY);
                }
                // Camera orientation calculation
                else if (panKey)
                {
                    // Camera orientation calculation
                    // Get the mouse sensitivity
                    cameraAngle.x += cameraMouseVariation.x*-CAMERA_FREE_MOUSE_SENSITIVITY;
                    cameraAngle.y += cameraMouseVariation.y*-CAMERA_FREE_MOUSE_SENSITIVITY;

                    // Angle clamp
                    if (cameraAngle.y > CAMERA_FREE_MIN_CLAMP*DEG2RAD) cameraAngle.y = CAMERA_FREE_MIN_CLAMP*DEG2RAD;
                    else if (cameraAngle.y < CAMERA_FREE_MAX_CLAMP*DEG2RAD) cameraAngle.y = CAMERA_FREE_MAX_CLAMP*DEG2RAD;
                }
            }
            // Paning
            else if (panKey)
            {
                camera->target.x += ((cameraMouseVariation.x*-CAMERA_FREE_MOUSE_SENSITIVITY)*cos(cameraAngle.x) + (cameraMouseVariation.y*CAMERA_FREE_MOUSE_SENSITIVITY)*sin(cameraAngle.x)*sin(cameraAngle.y))*(cameraTargetDistance/CAMERA_FREE_PANNING_DIVIDER);
                camera->target.y += ((cameraMouseVariation.y*CAMERA_FREE_MOUSE_SENSITIVITY)*cos(cameraAngle.y))*(cameraTargetDistance/CAMERA_FREE_PANNING_DIVIDER);
                camera->target.z += ((cameraMouseVariation.x*CAMERA_FREE_MOUSE_SENSITIVITY)*sin(cameraAngle.x) + (cameraMouseVariation.y*CAMERA_FREE_MOUSE_SENSITIVITY)*cos(cameraAngle.x)*sin(cameraAngle.y))*(cameraTargetDistance/CAMERA_FREE_PANNING_DIVIDER);
            }

            // Focus to center
            // TODO: Move this function out of this module?
            if (IsKeyDown('Z')) camera->target = (Vector3){ 0.0f, 0.0f, 0.0f };

            // Camera position update
            camera->position.x = sin(cameraAngle.x)*cameraTargetDistance*cos(cameraAngle.y) + camera->target.x;

            if (cameraAngle.y <= 0.0f) camera->position.y = sin(cameraAngle.y)*cameraTargetDistance*sin(cameraAngle.y) + camera->target.y;
            else camera->position.y = -sin(cameraAngle.y)*cameraTargetDistance*sin(cameraAngle.y) + camera->target.y;

            camera->position.z = cos(cameraAngle.x)*cameraTargetDistance*cos(cameraAngle.y) + camera->target.z;

        } break;
        case CAMERA_ORBITAL:
        {
            cameraAngle.x += CAMERA_ORBITAL_SPEED;

            // Camera zoom
            cameraTargetDistance -= (mouseWheelMove*CAMERA_SCROLL_SENSITIVITY);
            
            // Camera distance clamp
            if (cameraTargetDistance < CAMERA_THIRD_PERSON_DISTANCE_CLAMP) cameraTargetDistance = CAMERA_THIRD_PERSON_DISTANCE_CLAMP;

            // Focus to center
            if (IsKeyDown('Z')) camera->target = (Vector3){ 0.0f, 0.0f, 0.0f };

            // Camera position update
            camera->position.x = sin(cameraAngle.x)*cameraTargetDistance*cos(cameraAngle.y) + camera->target.x;

            if (cameraAngle.y <= 0.0f) camera->position.y = sin(cameraAngle.y)*cameraTargetDistance*sin(cameraAngle.y) + camera->target.y;
            else camera->position.y = -sin(cameraAngle.y)*cameraTargetDistance*sin(cameraAngle.y) + camera->target.y;

            camera->position.z = cos(cameraAngle.x)*cameraTargetDistance*cos(cameraAngle.y) + camera->target.z;

        } break;
        case CAMERA_FIRST_PERSON:
        case CAMERA_THIRD_PERSON:
        {
            bool isMoving = false;

            // Keyboard inputs
            if (IsKeyDown(cameraMoveControl[MOVE_FRONT]))
            {
                camera->position.x -= sin(cameraAngle.x)/PLAYER_MOVEMENT_DIVIDER;
                camera->position.y += sin(cameraAngle.y)/PLAYER_MOVEMENT_DIVIDER;
                camera->position.z -= cos(cameraAngle.x)/PLAYER_MOVEMENT_DIVIDER;

                isMoving = true;
            }
            else if (IsKeyDown(cameraMoveControl[MOVE_BACK]))
            {
                camera->position.x += sin(cameraAngle.x)/PLAYER_MOVEMENT_DIVIDER;
                camera->position.y -= sin(cameraAngle.y)/PLAYER_MOVEMENT_DIVIDER;
                camera->position.z += cos(cameraAngle.x)/PLAYER_MOVEMENT_DIVIDER;

                isMoving = true;
            }

            if (IsKeyDown(cameraMoveControl[MOVE_LEFT]))
            {
                camera->position.x -= cos(cameraAngle.x)/PLAYER_MOVEMENT_DIVIDER;
                camera->position.z += sin(cameraAngle.x)/PLAYER_MOVEMENT_DIVIDER;

                isMoving = true;
            }
            else if (IsKeyDown(cameraMoveControl[MOVE_RIGHT]))
            {
                camera->position.x += cos(cameraAngle.x)/PLAYER_MOVEMENT_DIVIDER;
                camera->position.z -= sin(cameraAngle.x)/PLAYER_MOVEMENT_DIVIDER;

                isMoving = true;
            }

            if (IsKeyDown(cameraMoveControl[MOVE_UP])) camera->position.y += 1.0f/PLAYER_MOVEMENT_DIVIDER;
            else if (IsKeyDown(cameraMoveControl[MOVE_DOWN])) camera->position.y -= 1.0f/PLAYER_MOVEMENT_DIVIDER;

            if (cameraMode == CAMERA_THIRD_PERSON)
            {
                // Camera orientation calculation
                cameraAngle.x += cameraMouseVariation.x*-cameraMouseSensitivity;
                cameraAngle.y += cameraMouseVariation.y*-cameraMouseSensitivity;

                // Angle clamp
                if (cameraAngle.y > CAMERA_THIRD_PERSON_MIN_CLAMP*DEG2RAD) cameraAngle.y = CAMERA_THIRD_PERSON_MIN_CLAMP*DEG2RAD;
                else if (cameraAngle.y < CAMERA_THIRD_PERSON_MAX_CLAMP*DEG2RAD) cameraAngle.y = CAMERA_THIRD_PERSON_MAX_CLAMP*DEG2RAD;

                // Camera zoom
                cameraTargetDistance -= (mouseWheelMove*CAMERA_SCROLL_SENSITIVITY);

                // Camera distance clamp
                if (cameraTargetDistance < CAMERA_THIRD_PERSON_DISTANCE_CLAMP) cameraTargetDistance = CAMERA_THIRD_PERSON_DISTANCE_CLAMP;

                // Camera is always looking at player
                camera->target.x = camera->position.x + CAMERA_THIRD_PERSON_OFFSET.x*cos(cameraAngle.x) + CAMERA_THIRD_PERSON_OFFSET.z*sin(cameraAngle.x);
                camera->target.y = camera->position.y + PLAYER_HEIGHT*CAMERA_FIRST_PERSON_HEIGHT_RELATIVE_EYES_POSITION + CAMERA_THIRD_PERSON_OFFSET.y;
                camera->target.z = camera->position.z + CAMERA_THIRD_PERSON_OFFSET.z*sin(cameraAngle.x) - CAMERA_THIRD_PERSON_OFFSET.x*sin(cameraAngle.x);

                // Camera position update
                camera->position.x = sin(cameraAngle.x)*cameraTargetDistance*cos(cameraAngle.y) + camera->target.x;

                if (cameraAngle.y <= 0.0f) camera->position.y = sin(cameraAngle.y)*cameraTargetDistance*sin(cameraAngle.y) + camera->target.y;
                else camera->position.y = -sin(cameraAngle.y)*cameraTargetDistance*sin(cameraAngle.y) + camera->target.y;

                camera->position.z = cos(cameraAngle.x)*cameraTargetDistance*cos(cameraAngle.y) + camera->target.z;
            }
            else    // CAMERA_FIRST_PERSON
            {
                if (isMoving) cameraMoveCounter++;

                // Camera orientation calculation
                cameraAngle.x += (cameraMouseVariation.x*-cameraMouseSensitivity);
                cameraAngle.y += (cameraMouseVariation.y*-cameraMouseSensitivity);

                // Angle clamp
                if (cameraAngle.y > CAMERA_FIRST_PERSON_MIN_CLAMP*DEG2RAD) cameraAngle.y = CAMERA_FIRST_PERSON_MIN_CLAMP*DEG2RAD;
                else if (cameraAngle.y < CAMERA_FIRST_PERSON_MAX_CLAMP*DEG2RAD) cameraAngle.y = CAMERA_FIRST_PERSON_MAX_CLAMP*DEG2RAD;

                // Camera is always looking at player
                camera->target.x = camera->position.x - sin(cameraAngle.x)*CAMERA_FIRST_PERSON_FOCUS_DISTANCE;
                camera->target.y = camera->position.y + sin(cameraAngle.y)*CAMERA_FIRST_PERSON_FOCUS_DISTANCE;
                camera->target.z = camera->position.z - cos(cameraAngle.x)*CAMERA_FIRST_PERSON_FOCUS_DISTANCE;

                // Camera position update
                //camera->position.y = (playerPosition.y + PLAYER_HEIGHT*CAMERA_FIRST_PERSON_HEIGHT_RELATIVE_EYES_POSITION) 
                // - sin(cameraMoveCounter/CAMERA_FIRST_PERSON_STEP_TRIGONOMETRIC_DIVIDER)/CAMERA_FIRST_PERSON_STEP_DIVIDER;
                
                // TODO: Review limits, avoid moving under the ground (y = 0.0f) and over the 'eyes position', weird movement (rounding issues...)
                camera->position.y -= sin(cameraMoveCounter/CAMERA_FIRST_PERSON_STEP_TRIGONOMETRIC_DIVIDER)/CAMERA_FIRST_PERSON_STEP_DIVIDER;

                camera->up.x = sin(cameraMoveCounter/(CAMERA_FIRST_PERSON_STEP_TRIGONOMETRIC_DIVIDER*2))/CAMERA_FIRST_PERSON_WAVING_DIVIDER;
                camera->up.z = -sin(cameraMoveCounter/(CAMERA_FIRST_PERSON_STEP_TRIGONOMETRIC_DIVIDER*2))/CAMERA_FIRST_PERSON_WAVING_DIVIDER;
            }
        } break;
        default: break;
    }
}

/*
// Set internal camera position
void SetCameraPosition(Vector3 position)
{
    internalCamera.position = position;
    
    Vector3 v1 = internalCamera.position;
    Vector3 v2 = internalCamera.target;
    
    float dx = v2.x - v1.x;
    float dy = v2.y - v1.y;
    float dz = v2.z - v1.z;
    
    cameraTargetDistance = sqrt(dx*dx + dy*dy + dz*dz);
}

// Set internal camera target
void SetCameraTarget(Vector3 target)
{
    internalCamera.target = target;
    
    Vector3 v1 = internalCamera.position;
    Vector3 v2 = internalCamera.target;
    
    float dx = v2.x - v1.x;
    float dy = v2.y - v1.y;
    float dz = v2.z - v1.z;
    
    cameraTargetDistance = sqrt(dx*dx + dy*dy + dz*dz);
}
*/
// Set camera pan key to combine with mouse movement (free camera)
void SetCameraPanControl(int panKey)
{
    cameraPanControlKey = panKey;
}

// Set camera alt key to combine with mouse movement (free camera)
void SetCameraAltControl(int altKey)
{
    cameraAltControlKey = altKey;
}

// Set camera smooth zoom key to combine with mouse (free camera)
void SetCameraSmoothZoomControl(int szKey)
{
    cameraSmoothZoomControlKey = szKey;
}

// Set camera move controls (1st person and 3rd person cameras)
void SetCameraMoveControls(int frontKey, int backKey, int leftKey, int rightKey, int upKey, int downKey)
{
    cameraMoveControl[MOVE_FRONT] = frontKey;
    cameraMoveControl[MOVE_LEFT] = leftKey;
    cameraMoveControl[MOVE_BACK] = backKey;
    cameraMoveControl[MOVE_RIGHT] = rightKey;
    cameraMoveControl[MOVE_UP] = upKey;
    cameraMoveControl[MOVE_DOWN] = downKey;
}

// Set camera mouse sensitivity (1st person and 3rd person cameras)
void SetCameraMouseSensitivity(float sensitivity)
{
    cameraMouseSensitivity = (sensitivity/10000.0f);
}

#endif // CAMERA_IMPLEMENTATION
