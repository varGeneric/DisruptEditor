#include "Camera.h"

#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

void Camera::update(float delta) {
	switch (type) {
		case FLYCAM:
		{
			const Uint8 *state = SDL_GetKeyboardState(NULL);
			bool moveForward = state[SDL_SCANCODE_W];
			bool moveBackward = state[SDL_SCANCODE_S];
			bool moveLeft = state[SDL_SCANCODE_A];
			bool moveRight = state[SDL_SCANCODE_D];
			bool moveUp = state[SDL_SCANCODE_R];
			bool moveDown = state[SDL_SCANCODE_F];

			float actualMoveSpeed = delta * 10.f;
			if (SDL_GetModState() & KMOD_LCTRL)
				actualMoveSpeed /= 10.f;
			if (SDL_GetModState() & KMOD_LSHIFT)
				actualMoveSpeed *= 20.f;

			float dx = sinf(phi) * cosf(theta) * actualMoveSpeed;
			float dy = cosf(phi) * actualMoveSpeed;
			float dz = sinf(phi) * sinf(theta) * actualMoveSpeed;

			vec3 movement;

			if (moveForward) {
				movement.x += dx;
				movement.y += dy;
				movement.z += dz;
			}
			if (moveBackward) {
				movement.x -= dx;
				movement.y -= dy;
				movement.z -= dz;
			}
			if (moveLeft) {
				movement.x += dz;
				//movement.y += dy;
				movement.z -= dx;
			}
			if (moveRight) {
				movement.x -= dz;
				//movement.y += dy;
				movement.z += dx;
			}

			location += movement;

			if (moveUp) location.y += actualMoveSpeed;
			if (moveDown) location.y -= actualMoveSpeed;

			float actualLookSpeed = delta * 10.f;

			int mouseX, mouseY;
			Uint32 mouseMask = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			if (!(mouseMask & SDL_BUTTON(SDL_BUTTON_MIDDLE)))
				actualLookSpeed = 0.f;

			lon += mouseX * actualLookSpeed;
			lat -= mouseY * actualLookSpeed;

			lat = max(-85, min(85, lat));
			phi = (90 - lat) * (3.141592f / 180.f);

			theta = (lon) * (3.141592f / 180.f);

			lookingAt.x = 100.f * sinf(phi) * cosf(theta);
			lookingAt.y = 100.f * cosf(phi);
			lookingAt.z = 100.f * sinf(phi) * sinf(theta);

			lookingAt += location;
			break;
		}
		case ORBIT:
			break;
	}
}
