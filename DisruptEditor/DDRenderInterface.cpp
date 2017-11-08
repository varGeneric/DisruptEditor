#include "DDRenderInterface.h"

#include <SDL_assert.h>
#include "stb_image_write.h"

RenderInterface::RenderInterface() {
	lines = loadShaders("res/gldd.xml");
	linesBuffer = createVertexBuffer(NULL, 0, BUFFER_STREAM);
	glEnable(GL_PROGRAM_POINT_SIZE);

	tex = loadShaders("res/tex.xml");
	texBuffer = createVertexBuffer(NULL, 0, BUFFER_STREAM);

	model = loadShaders("res/model.xml");

	glGenVertexArrays(1, &VertexArrayID);

	//Icon Framebuffer
	glGenTextures(1, &fbo_texture);
	glBindTexture(GL_TEXTURE_2D, fbo_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 512, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glGenTextures(1, &fbo_depth);
	glBindTexture(GL_TEXTURE_2D, fbo_depth);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 512, 512, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_texture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fbo_depth, 0);
	SDL_assert_release(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderInterface::drawPointList(const dd::DrawVertex *points, int count, bool depthEnabled) {
	updateVertexBuffer(points, count * sizeof(dd::DrawVertex), linesBuffer);

	lines.use();
	glUniformMatrix4fv(lines.uniforms["MVP"], 1, GL_FALSE, &VP[0][0]);
	// 1rst attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, linesBuffer.buffer_id);
	glVertexAttribPointer(
		0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		sizeof(float) * 7,  // stride
		(void*)0            // array buffer offset
	);

	// 2nd attribute buffer : colors
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(
		1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
		4,                                // size
		GL_FLOAT,                         // type
		GL_FALSE,                         // normalized?
		sizeof(float) * 7,                // stride
		(GLvoid*)(3 * sizeof(GLfloat))    // array buffer offset
	);

	// Draw the triangles
	glDrawArrays(GL_POINTS, 0, count);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
}

void RenderInterface::drawLineList(const dd::DrawVertex *points, int count, bool depthEnabled) {
	updateVertexBuffer(points, count * sizeof(dd::DrawVertex), linesBuffer);

	lines.use();
	glUniformMatrix4fv(lines.uniforms["MVP"], 1, GL_FALSE, &VP[0][0]);
	// 1rst attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, linesBuffer.buffer_id);
	glVertexAttribPointer(
		0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		sizeof(float) * 7,  // stride
		(void*)0            // array buffer offset
	);

	// 2nd attribute buffer : colors
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(
		1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
		4,                                // size
		GL_FLOAT,                         // type
		GL_FALSE,                         // normalized?
		sizeof(float) * 7,                // stride
		(GLvoid*)(3 * sizeof(GLfloat))    // array buffer offset
	);

	// Draw the triangles
	glDrawArrays(GL_LINES, 0, count);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
}

void RenderInterface::drawGlyphList(const dd::DrawVertex * glyphs, int count, dd::GlyphTextureHandle glyphTex) {
	updateVertexBuffer(glyphs, count * sizeof(dd::DrawVertex), texBuffer);

	tex.use();
	glUniform2f(tex.uniforms["windowSize"], windowSize.x, windowSize.y);
	glBindBuffer(GL_ARRAY_BUFFER, texBuffer.buffer_id);
	// 1rst attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
		2,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		sizeof(float) * 7,  // stride
		(void*)0            // array buffer offset
	);

	// 2nd attribute buffer : colors
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(
		1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
		2,                                // size
		GL_FLOAT,                         // type
		GL_FALSE,                         // normalized?
		sizeof(float) * 7,                // stride
		(GLvoid*)(2 * sizeof(GLfloat))    // array buffer offset
	);

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(
		2,                                // attribute. No particular reason for 1, but must match the layout in the shader.
		3,                                // size
		GL_FLOAT,                         // type
		GL_FALSE,                         // normalized?
		sizeof(float) * 7,                // stride
		(GLvoid*)(4 * sizeof(GLfloat))    // array buffer offset
	);

	// Draw the triangles
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, (GLuint)glyphTex);
	glDrawArrays(GL_TRIANGLES, 0, count);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
}

dd::GlyphTextureHandle RenderInterface::createGlyphTexture(int width, int height, const void * pixels) {
	GLuint textureId;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);

	return (dd::GlyphTextureHandle)textureId;
}

void RenderInterface::destroyGlyphTexture(dd::GlyphTextureHandle glyphTex) {
	glDeleteTextures(1, (GLuint*)&glyphTex);
}


void RenderInterface::saveFBO(const char *filename) {
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFinish();

	Vector<uint8_t> pixels(512 * 512 * 4);
	glReadPixels(0, 0, 512, 512, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

	stbi_write_png(filename, 512, 512, 4, pixels.data(), 4 * 512);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

RenderInterface& RenderInterface::instance() {
	static RenderInterface ri;
	return ri;
}
