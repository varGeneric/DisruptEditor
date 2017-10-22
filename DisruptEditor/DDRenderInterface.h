#pragma once

#include "Implementation.h"

class RenderInterface : public dd::RenderInterface {
public:
	RenderInterface();
	//
	// These are called by dd::flush() before any drawing and after drawing is finished.
	// User can override these to perform any common setup for subsequent draws and to
	// cleanup afterwards. By default, no-ops stubs are provided.
	//
	void beginDraw() {}
	void endDraw() {}

	//
	// Create/free the glyph bitmap texture used by the debug text drawing functions.
	// The debug renderer currently only creates one of those on startup.
	//
	// You're not required to implement these two if you don't care about debug text drawing.
	// Default no-op stubs are provided by default, which disable debug text rendering.
	//
	// Texture dimensions are in pixels, data format is always 8-bits per pixel (Grayscale/GL_RED).
	// The pixel values range from 255 for a pixel within a glyph to 0 for a transparent pixel.
	// If createGlyphTexture() returns null, the renderer will disable all text drawing functions.
	//
	dd::GlyphTextureHandle createGlyphTexture(int width, int height, const void * pixels);
	void destroyGlyphTexture(dd::GlyphTextureHandle glyphTex);

	//
	// Batch drawing methods for the primitives used by the debug renderer.
	// If you don't wish to support a given primitive type, don't override the method.
	//
	void drawPointList(const dd::DrawVertex * points, int count, bool depthEnabled);
	void drawLineList(const dd::DrawVertex * lines, int count, bool depthEnabled);
	void drawGlyphList(const dd::DrawVertex * glyphs, int count, dd::GlyphTextureHandle glyphTex);

	Shader lines, tex, model;
	VertexBuffer linesBuffer, texBuffer;

	mat4 VP;
	ivec2 windowSize;
};
