#pragma once
#include <GL/gl.h>

class Light {
public:
	GLenum lightID; // Light ID (GL_LIGHT0, GL_LIGHT1, ...)
	GLfloat position[4]; // Light position
	// Ambient light color is the intensity of the light on the object
	GLfloat ambient[4];
	// Diffuse light color is the color of the light
	GLfloat diffuse[4];
	// Specular light color is the color of the light when it reflects off the object
	GLfloat specular[4];

	// Constructor to initialize the light with default properties
	Light(GLenum lightId) {
		lightID = lightId;

		// Default position (directional light pointing along Z-axis)
		position[0] = 0.0f;
		position[1] = 0.0f;
		position[2] = 1.0f;
		position[3] = 0.0f; // 0.0f for directional light, 1.0f for point light

		// Default ambient light (low intensity)
		ambient[0] = 0.2f;
		ambient[1] = 0.2f;
		ambient[2] = 0.2f;
		ambient[3] = 1.0f;

		// Default diffuse light (white light)
		diffuse[0] = 1.0f;
		diffuse[1] = 1.0f;
		diffuse[2] = 1.0f;
		diffuse[3] = 1.0f;

		// Default specular light (white highlights)
		specular[0] = 1.0f;
		specular[1] = 1.0f;
		specular[2] = 1.0f;
		specular[3] = 1.0f;
	}

	// Set the position of the light
	void setPosition(GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
		position[0] = x;
		position[1] = y;
		position[2] = z;
		position[3] = w;
	}

	// Set the ambient light color
	void setAmbient(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
		ambient[0] = r;
		ambient[1] = g;
		ambient[2] = b;
		ambient[3] = a;
	}

	// Set the diffuse light color
	void setDiffuse(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
		diffuse[0] = r;
		diffuse[1] = g;
		diffuse[2] = b;
		diffuse[3] = a;
	}

	// Set the specular light color
	void setSpecular(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
		specular[0] = r;
		specular[1] = g;
		specular[2] = b;
		specular[3] = a;
	}

	// Apply the light properties to OpenGL
	void apply() const {
		glEnable(lightID); // Enable the light
		glLightfv(lightID, GL_POSITION, position); // Set position
		glLightfv(lightID, GL_AMBIENT, ambient);   // Set ambient color
		glLightfv(lightID, GL_DIFFUSE, diffuse);   // Set diffuse color
		glLightfv(lightID, GL_SPECULAR, specular); // Set specular color
	}

	// Disable the light
	void disable() const {
		glDisable(lightID);
	}
};
